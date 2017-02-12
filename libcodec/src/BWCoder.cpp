//---------------------------------------------------------------------------
#include "BWCoder.h"
#include "CommonDef.h"
#include "IFStage.h"
#include "ECStage.h"

#include <time.h>
#include <algorithm>

/////////////////////////////////////////////////////////
//       Burrows-Wheeler Transform Functions           //
/////////////////////////////////////////////////////////
void BWCEncode(BWC& block)
{
    clock_t start, bwt_end, ivr_end, enc_end;
    BYTE* pstream; //pointer to write
    short* bwt_seq;
    int* F;
    short* sptr; //pointers to sweep
    int k; //counter to sweep
    short min, max, range, alphaLen;
    int est_stream_len;

    start = clock();
    if (block.symbols == nullptr)
    {
        block.stream = nullptr;
        block.output_len = 0;
        return;
    }
    if (block.Convert2UIntBeforeBWT)
    {
        bias2unbias(block.symbols, block.input_len);
    }

    min_max_element(block.symbols, block.input_len, min, max); //min and max symbol
    range = max - min + 1; //range of the symbols
    alphaLen = range; //number of symbols

    //Count Symbols and adjust data from 0
    F = new int[range];
    reset_vector(F, range);
    sptr = block.symbols;
    k = block.input_len;
    while (k--)
        ++F[*sptr++ -= min];

    est_stream_len = block.input_len * 1.5;
    pstream = block.stream = new BYTE[est_stream_len];
#if 0
        reset_vector(block.stream, est_stream_len); //For arithmetic coding
#endif

    //Burrows-Wheeler transform...
    //Calculate BWT by the Bentley-Sedgewick Quick Sort Method
    bwt_seq = ForwardBWT(block);
    WriteUnsigned(block.m_bwtIndex, 3, pstream);
    WriteInt(min, 2, pstream);
    bwt_end = clock();

    //write the frecuency count of symbols in stream
    unsigned F_RLELen = range;
    int* F_RLE = RLE0(F, F_RLELen);
    WriteUnsigned(F_RLELen, 2, pstream);
    EntropyEncode(F_RLE, F_RLELen, block.E2Level, pstream);
    delete []F_RLE;

    //Inversion Frequency Stage with symbol sort
    if (block.SortMode == 3) //Determine best sort mode
        block.SortMode = BestSortMode(F, range);
    int* T = GetForwardPermutation(F, alphaLen, block.SortMode);
    int* if_seq = IFEncode(bwt_seq, F, T, alphaLen);
    block.alphaLen = alphaLen;
    unsigned if_seq_len = block.input_len - F[alphaLen - 1];
    delete []bwt_seq;
    delete []F;
    delete[] T;
    ivr_end = clock();

    //Entropy Encode Stage
    EntropyEncode(if_seq, if_seq_len, block.E2Level, pstream);
    delete []if_seq;

    block.output_len = pstream - block.stream;
    enc_end = clock();
    //Elapsed Time information
    block.bwc_time = ((double)enc_end - start) / CLK_TCK;
    block.bwt_time = ((double)bwt_end - start) / CLK_TCK;
    block.ivr_time = ((double)ivr_end - bwt_end) / CLK_TCK;
    block.enc_time = ((double)enc_end - ivr_end) / CLK_TCK;
}

void BWCDecode(BWC& block)
{
    clock_t start, bwt_end, ivr_end, dec_end;
    start = clock();

    BYTE* pstream = block.stream;
    unsigned if_seq_len;
    short* bwt_seq = nullptr;
    int* F = nullptr;

    register int k;
    register short* sptr; //cicle variables

    block.m_bwtIndex = ReadUnsigned(3, pstream);
    short min = ReadInt(2, pstream);
    short range = ReadUnsigned(2, pstream);

    //Decode the Frequency Table
    int* F_RLE = new int[range];
    EntropyDecode(F_RLE, range, block.E2Level, pstream);
    unsigned F_RLELen = range;
    F = RLD0(F_RLE, F_RLELen);
    delete []F_RLE;
    range = F_RLELen;

    short alphaLen = range;
    unsigned seq_len = sum(F, range);
    block.output_len = seq_len;

    int* T = GetInversePermutation(F, alphaLen, block.SortMode);
    if_seq_len = seq_len - F[alphaLen - 1];

    //Decode the IF sequence from the stream
    int* if_seq = new int[if_seq_len];
    EntropyDecode(if_seq, if_seq_len, block.E2Level, pstream);
    block.input_len = pstream - block.stream;
    dec_end = clock();

    // Inversion Frequency Stage
    if (block.SortMode == DESCENDING)
        bwt_seq = IFDecode_D(if_seq, F, T, alphaLen);
    else
        bwt_seq = IFDecode_A(if_seq, F, T, alphaLen);
    delete []if_seq;
    delete []F;
    delete []T;
    ivr_end = clock();

    // Burrows-Wheeler inverse transform...
    block.symbols = InverseBWT(bwt_seq, seq_len, block.m_bwtIndex, min);
    delete []bwt_seq;
    if (block.Convert2UIntBeforeBWT)
        unbias2bias(block.symbols, seq_len);
    bwt_end = clock();

    block.bwc_time = (double)((double)bwt_end - start) / CLK_TCK;
    block.bwt_time = (double)((double)bwt_end - ivr_end) / CLK_TCK;
    block.ivr_time = (double)((double)ivr_end - dec_end) / CLK_TCK;
    block.enc_time = (double)((double)dec_end - start) / CLK_TCK;
}

short* ForwardBWT(BWC& blck)
{
    short* bwt_seq = new short[blck.input_len];
    blck.symbols[blck.input_len] = -1;

    blck.m_bwtIndex = blck.input_len;
    short** suffixes = new short*[blck.input_len + 1];
    for (register unsigned i = 0; i <= blck.input_len; ++i)
        suffixes[i] = blck.symbols + i;

    /* Sort the suffixes using Bentley-Sedgewick Faster ternary quick sort version */
    ssort2((short **)suffixes, blck.input_len + 1, 0);

    /* if TESTING Calculate AML of the sequence */
    if (!blck.aml) blck.aml = calculate_aml(suffixes, blck.input_len);

    /* Copy the last Col of sorted matrix to the original symbol string */
    short* p = bwt_seq;
    for (unsigned j = 0; j <= blck.input_len; j++)
        if (suffixes[j] == blck.symbols)
            blck.m_bwtIndex = j;
        else
            *p++ = *(suffixes[j] - 1);

    delete []suffixes;
    return bwt_seq;
}

short* InverseBWT(short* bwt_seq, unsigned Len, unsigned index, short offset)
{
    register int k, tmp, I = 0;
    short* rec_seq = new short[Len];
    int* P = new int[Len];
    int* C = new int[ALPHA_LEN];
    int *pc = C, *pp = P;
    short* pl = bwt_seq; //Sweep variables
    reset_vector(C, ALPHA_LEN);

    k = Len;
    while (k--)
        *pp++ = C[*pl++]++;

    int sum = 1;
    k = ALPHA_LEN;
    while (--k)
        *pc++ = (sum += *pc) - *pc;

    k = Len;
    while (k--)
    {
        if ((I = P[I] + C[tmp = bwt_seq[I]]) > index) --I;
        rec_seq[k] = tmp + offset;
    }

    delete []C;
    delete []P;
    return rec_seq;
}

int* GetForwardPermutation(int* F, short& range, int sort_mode)
{
    //Get the exsiting symbols frequencies
    unsigned n_of_symbol, i, j;
    int *k, *l, *end = F + range;
    int* F_copy = new int[range];
    reset_vector(F_copy, range);
    for (k = F , l = F_copy; k < end; ++k)
        if (*k)
            *l++ = *k;

    n_of_symbol = l - F_copy; //Number of existing symbols
    //Sort frequencies (if necessary)
    if (sort_mode != NO_SORT) //Ascending or descending order
    {
        std::sort(F_copy, l);
        if (sort_mode == DESCENDING)
            flipud(F_copy, n_of_symbol);
    }
    // Construct the permutation sequence
    int* T = new int[range];
    for (i = 0; i < range; i++)
    {
        j = 0;
        while (F_copy[i] != F[j])
            j++;
        F[j] = -1;
        T[j] = i;
    }
    copy_vector(F, F_copy, range);
    delete []F_copy;
    range = n_of_symbol;
    return T;
}

int* GetInversePermutation(int* F, short& range, int sort_mode)
{
    //Get the exsiting symbols frequencies
    unsigned n_of_symbol, i, j;
    int *k, *l, *end = F + range;
    int* F_copy = new int[range];
    reset_vector(F_copy, range);
    for (k = F , l = F_copy; k < end; ++k)
    {
        if (*k)
            *l++ = *k;
    }

    n_of_symbol = l - F_copy; //Number of existing symbols
    //Sort frequencies (if necessary)
    if (sort_mode != NO_SORT) //Ascending or descending order
    {
        std::sort(F_copy, l);
        if (sort_mode == DESCENDING)
            flipud(F_copy, n_of_symbol);
    }
    // Construct the permutation sequence
    int* T = new int[range];
    for (i = 0; i < range; i++)
    {
        j = 0;
        while (F_copy[i] != F[j])
            j++;
        F[j] = -1;
        T[i] = j;
    }
    copy_vector(F, F_copy, range);
    range = n_of_symbol;
    delete []F_copy;
    return T;
}

int BestSortMode(int* F, short& range)
{
    short alphaLen = 0;
    unsigned k;

    for (k = 0; k < range; k++)
        if (F[k])
            ++alphaLen; //number of symbols that really appear on the sequence
    double Favg = sum(F, range) / (double)alphaLen;

    unsigned G = 0;
    for (k = 0; k < range; k++)
        if (F[k] >= 2 * Favg)
            ++G;

    range = alphaLen; //return the number of symbols that really appear on the sequence
    if (100.0 * G / (double)alphaLen < 10.0)
        return DESCENDING; // i.e. descending sort mode
    else
        return ASCENDING; //i.e. ascending sort mode
}

int EstimateStreamLength(int* F, int range, int seq_len)
{
    double entropy = 0, p, dou_seq_len = seq_len;
    for (unsigned k = 0; k < range; k++)
        if (F[k])
        {
            p = F[k] / dou_seq_len;
            entropy += -p * log(p);
        }
    return 500 + entropy / log(2.0) * dou_seq_len / 8 * 1.50; //giving a margin
}

//To calculate de Forward BWT  (Bentley-Sedgewick Method)
#define min(a, b) ((a)<=(b) ? (a) : (b))
#define i2c(i) x[i][depth]
// ssort2 -- Faster Version of Multikey Quicksort
void vecswap2(short** a, short** b, int n)
{
    while (n-- > 0)
    {
        short* t = *a;
        *a++ = *b;
        *b++ = t;
    }
}

#define swap2(a, b) { t = *(a); *(a) = *(b); *(b) = t; }
#define ptr2short(i) (*(*(i) + depth))

short** med3func(short** a, short** b, short** c, int depth)
{
    int va, vb, vc;
    if ((va = ptr2short(a)) == (vb = ptr2short(b)))
        return a;
    if ((vc = ptr2short(c)) == va || vc == vb)
        return c;
    return va < vb ?
               (vb < vc ? b : (va < vc ? c : a))
               : (vb > vc ? b : (va < vc ? a : c));
}

#define med3(a, b, c) med3func(a, b, c, depth)

void inssort(short** a, int n, int d)
{
    short **pi, **pj, *s, *t;
    for (pi = a + 1; --n > 0; pi++)
        for (pj = pi; pj > a; pj--)
        {
            // Inline strcmp: break if *(pj-1) <= *pj
            for (s = *(pj - 1) + d , t = *pj + d; *s == *t; s++ , t++);
            if (*s <= *t)
                break;
            swap2(pj, pj-1);
        }
}

void ssort2(short** a, int n, int depth)
{
    int d, r, partval;
    short **pa, **pb, **pc, **pd, **pl, **pm, **pn, *t;
    if (n < 10)
    {
        inssort(a, n, depth);
        return;
    }
    pl = a;
    pm = a + (n / 2);
    pn = a + (n - 1);
    if (n > 30)
    { // On big arrays, pseudomedian of 9
        d = (n / 8);
        pl = med3(pl, pl+d, pl+2*d);
        pm = med3(pm-d, pm, pm+d);
        pn = med3(pn-2*d, pn-d, pn);
    }
    pm = med3(pl, pm, pn);
    swap2(a, pm);
    partval = ptr2short(a);
    pa = pb = a + 1;
    pc = pd = a + n - 1;
    for (;;)
    {
        while (pb <= pc && (r = ptr2short(pb) - partval) <= 0)
        {
            if (r == 0)
            {
                swap2(pa, pb);
                pa++;
            }
            pb++;
        }
        while (pb <= pc && (r = ptr2short(pc) - partval) >= 0)
        {
            if (r == 0)
            {
                swap2(pc, pd);
                pd--;
            }
            pc--;
        }
        if (pb > pc) break;
        swap2(pb, pc);
        pb++;
        pc--;
    }
    pn = a + n;
    r = min(pa-a, pb-pa);
    vecswap2(a, pb - r, r);
    r = min(pd-pc, pn-pd-1);
    vecswap2(pb, pn - r, r);
    if ((r = pb - pa) > 1)
        ssort2(a, r, depth);
    //the smallest symbol is -1 and it's only one on the sequence
    if (ptr2short(a + r) != -1)
        ssort2(a + r, pa - a + pn - pd - 1, depth + 1);
    if ((r = pd - pc) > 1)
        ssort2(a + n - r, r, depth);
}
