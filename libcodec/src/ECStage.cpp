/////////////////////////////////////
// Entropy Coding Implementation   //
/////////////////////////////////////

#include "ECStage.h"
#include "CommonDef.h"
#include <cmath>

///////////////////////////////////////////////////////
// Interfase Functions for the Entropy Coding Stage  //
///////////////////////////////////////////////////////
void EntropyEncode(int *v, unsigned vLen, int E2Level, BYTE *&stream)
{
    BYTE *e, *e2, *m1, *m2, *m3, *m0;
    e = new BYTE[12 * vLen];
    e2 = e + vLen;
    m1 = e2 + vLen;
    m2 = m1 + vLen;
    m3 = m2 + vLen;
    m0 = m3 + vLen;

    unsigned j;
    int _e, _b;
    BYTE maxE2 = 0;
    unsigned m1Len = 0, m2Len = 0, m3Len = 0,
         m0Len = 0, e2Len = 0;

    for (j = 0; j < vLen; j++)
    {
    if (!(e[j] = bin2exp(v[j], _b)))
        continue;
    else if (e[j] == 1) // this  group has 2 possible symbols
        m1[m1Len++] = _b;
    else if (e[j] == 2) // this  group has 4 possible symbols
        m2[m2Len++] = _b;
    else //this  group has 8 or more possible symbols
    {
        m3[m3Len++] = _b & 0x07;
        if (e[j] > 3) //more than 3 bits word
        {
        _b >>= 3; //To get the msbits in val
        _e = 3;
        while (_e < e[j])
        {
            m0[m0Len++] = _b & 0x01;
            _b >>= 1;
            ++_e;
        }
        }
    }
    if ((_e = e[j] - E2Level) >= 0)
    {
        e[j] = E2Level;
        e2[e2Len++] = _e;
        if (_e > maxE2)
        maxE2 = _e;
    }
    }

    //Write header
    int h = 0;
    h |= bytes(m1Len);
    h <<= 2;
    h |= bytes(m2Len);
    h <<= 2;
    h |= bytes(m3Len);
    h <<= 2;
    h |= bytes(m0Len);
    h <<= 2;
    h |= bytes(e2Len);
    h <<= 5;
    h |= maxE2;

    WriteUnsigned(h, 2, stream);
    WriteUnsigned(e2Len, bytes(e2Len) + 1, stream);
    WriteUnsigned(m0Len, bytes(m0Len) + 1, stream);
    WriteUnsigned(m3Len, bytes(m3Len) + 1, stream);
    WriteUnsigned(m2Len, bytes(m2Len) + 1, stream);
    WriteUnsigned(m1Len, bytes(m1Len) + 1, stream);

    MODELS
    TCODER coder;
    coder.StartEncoding(stream, 0); //The first byte has the alphabet of 2 level model.
    coder.EncodeSequence(e, vLen, eModel);
    if (maxE2)
    {
        coder.EncodeSequence(e2, e2Len, e2Model);
    }
    coder.EncodeSequence(m1, m1Len, Model1);
    coder.EncodeSequence(m2, m2Len, Model2);
    coder.EncodeSequence(m3, m3Len, Model3);
    coder.EncodeSequence(m0, m0Len, Model0);
    stream += coder.DoneEncoding();
    delete[] e;
}

void EntropyDecode(int *v, unsigned vLen, int E2Level, BYTE *&stream)
{
    unsigned m1Len, m2Len, m3Len, m0Len, e2Len;
    BYTE *e2, *m1, *m2, *m3, *m0;
    unsigned j;
    BYTE maxE2, _e, _ee;
    int b;
    BYTE *e = new BYTE[vLen];

    //Read Header
    int h = ReadUnsigned(2, stream);
    maxE2 = h & 31;
    h >>= 5;
    e2Len = ReadUnsigned((h & 3) + 1, stream);
    h >>= 2;
    m0Len = ReadUnsigned((h & 3) + 1, stream);
    h >>= 2;
    m3Len = ReadUnsigned((h & 3) + 1, stream);
    h >>= 2;
    m2Len = ReadUnsigned((h & 3) + 1, stream);
    h >>= 2;
    m1Len = ReadUnsigned((h & 3) + 1, stream);

    MODELS

    TCODER coder;
    coder.StartDecoding(stream, 0);
    coder.DecodeSequence(e, vLen, eModel);
    if (e2Len)
    e2 = new BYTE[e2Len];
    coder.DecodeSequence(e2, e2Len, e2Model);
    if (m1Len)
    m1 = new BYTE[m1Len];
    coder.DecodeSequence(m1, m1Len, Model1);
    if (m2Len)
    m2 = new BYTE[m2Len];
    coder.DecodeSequence(m2, m2Len, Model2);
    if (m3Len)
    m3 = new BYTE[m3Len];
    coder.DecodeSequence(m3, m3Len, Model3);
    if (m0Len)
    m0 = new BYTE[m0Len];
    coder.DecodeSequence(m0, m0Len, Model0);
    stream += coder.DoneDecoding();

    m1Len = m2Len = m3Len = m0Len = e2Len = 0;

    for (j = 0; j < vLen; j++)
    {
    _e = e[j];
    if (_e == E2Level && maxE2)
        _e += e2[e2Len++];
    if (_e == 0)
        v[j] = 0;
    else if (_e == 1) //  2 possible levels
        v[j] = exp2bin(_e, m1[m1Len++]);
    else if (_e == 2) // 4 possible levels
        v[j] = exp2bin(_e, m2[m2Len++]);
    else // 8 posible levels or more
    {
        b = m3[m3Len++];
        if (_e > 3) //more than 3 bits word
        {
        _ee = 3;
        while (_ee < _e)
        {
            b += m0[m0Len++] << _ee;
            _ee++;
        }
        }
        v[j] = exp2bin(_e, b);
    }
    } //end of for...
    delete[] e;
    if (e2Len)
    delete[] e2;
    if (m1Len)
    delete[] m1;
    if (m2Len)
    delete[] m2;
    if (m3Len)
    delete[] m3;
    if (m0Len)
    delete[] m0;
}

int *RLE0(int *s, unsigned &sLen)
{
    unsigned j = 0, jr = 0;
    unsigned run;
    int *sr = new int[sLen * 3 / 2 + 1]; //worst case

    while (j < sLen)
    {
    run = 0;
    while (!s[j])
    {
        ++run;
        ++j;
    }
    if (run)
    {
        sr[jr++] = 0;
        sr[jr++] = run - 1;
    }
    else
        sr[jr++] = s[j++];
    }
    sLen = jr; //the length of the run
    return sr;
}

int *RLD0(int *sr, unsigned &sLen)
{
    unsigned j = 0, jr = 0;
    unsigned run;
    unsigned est_len = 0;

    //Get the length of the Run Length Decoded sequence
    while (jr < sLen)
    {
    ++est_len;
    if (!sr[jr])
    {
        ++jr;
        est_len += sr[jr];
    }
    ++jr;
    }

    int *s = new int[est_len];
    jr = 0;
    while (jr < sLen)
    {
    if (!sr[jr])
    {
        ++jr;
        run = sr[jr++] + 1;
        while (run--)
        s[j++] = 0;
    }
    else
        s[j++] = sr[jr++];
    }
    sLen = j;
    return s;
}
////////////////////////////////////////////////////////
//       TCModel Class Implementation                 //
////////////////////////////////////////////////////////
TCModel::TCModel(int n_of_symbols, int inc)
{
    m_alphabet_len = n_of_symbols;
    m_index_len = n_of_symbols + 2;
    m_n = 0;
    m_increment_0 = inc;

    m_cum_freqs = new int[m_index_len];
    m_freqs = new int[m_index_len];
    Reset();
}
TCModel::TCModel(int n_of_symbols, int inc_0, int inc_1, int inc_n, int context_bit_shift, int order)
{
    m_alphabet_len = n_of_symbols;
    m_index_len = n_of_symbols + 2;
    m_increment_0 = inc_0;
    m_increment_1 = inc_1;
    m_increment_n = inc_n;

    m_context_bit_shift = context_bit_shift;
    m_n = order;
    m_c_1_len = 1 << m_context_bit_shift;
    m_c_n_len = 1 << (m_context_bit_shift * m_n);
    m_context_1_mask = m_c_1_len - 1;
    m_context_n_mask = m_c_n_len - 1;

    //Get the main block for frequency context storage
    m_block = new int[m_index_len * (1 + m_c_1_len + m_c_n_len)];

    m_context_freqs_0 = m_block;
    m_context_freqs_1 = new int *[m_c_1_len];
    m_context_freqs_n = new int *[m_c_n_len];

    int *p = m_block + m_index_len;
    for (int i = 0; i < m_c_1_len; ++i, p += m_index_len)
    m_context_freqs_1[i] = p;
    for (int i = 0; i < m_c_n_len; ++i, p += m_index_len)
    m_context_freqs_n[i] = p;

    //Sum of frequencies for every context
    m_cum_context_1 = new int[m_c_1_len];
    m_cum_context_n = new int[m_c_n_len];
    m_cum_freqs = new int[m_index_len];
    Reset();
}
void TCModel::Reset()
{
    int i, order, cum_freq;
    if (m_n)
    {
        m_context_freqs_0[0] = 0;
        for (order = 0; order <= m_context_1_mask; ++order)
            m_context_freqs_1[order][0] = 0;
        for (order = 0; order <= m_context_n_mask; ++order)
            m_context_freqs_n[order][0] = 0;

        for (i = 1; i <= m_alphabet_len; ++i)
            m_context_freqs_0[i] = m_increment_0;
        for (order = 0; order <= m_context_1_mask; ++order)
            for (i = 1; i <= m_alphabet_len; ++i)
            m_context_freqs_1[order][i] = m_increment_1;
        for (order = 0; order <= m_context_n_mask; ++order)
            for (i = 1; i <= m_alphabet_len; ++i)
            m_context_freqs_n[order][i] = m_increment_n;

        // Calculate cumulated frequency count of order 0
        m_cum_context_0 = m_alphabet_len * m_increment_0;

        // Calculate cumulated frequency count of order 1 for every possible context
        for (order = 0; order <= m_context_1_mask; ++order)
            m_cum_context_1[order] = m_alphabet_len * m_increment_1;

        // Calculate cumulated frequency count of order n for every possible context
        for (order = 0; order <= m_context_n_mask; ++order)
            m_cum_context_n[order] = m_alphabet_len * m_increment_n;

        //Initialize old data and context
        m_Cache = 0;
        m_context_1 = 0;
        m_context_n = 0;

        // Set current frequency array
        SetCurrentFrequencies();
    }
    else //Reset No context model
    {
        m_freqs[0] = 0; // frequency of 0 must be 0
        //initialize freqs array
        for (i = 1; i <= m_alphabet_len; i++)
            m_freqs[i] = (m_alphabet_len + 1 - i) * m_increment_0;
        //initializr cum_freqs array
        cum_freq = 0;
        for (i = m_alphabet_len; i >= 0; i--)
        {
            m_cum_freqs[i] = cum_freq;
            cum_freq += m_freqs[i];
        }
    }
}
void TCModel::Update(int index)
{
    int i, cum_freq;
    int *ptr;

    if (m_n)
    {
    //Update Context 0
    m_context_freqs_0[index] += m_increment_0;
    m_cum_context_0 += m_increment_0;
    //Rescale if cumulated frequency passes FREQ_MAX
    if (m_cum_context_0 >= FREQ_MAX)
    {
        cum_freq = 0;
        for (i = m_alphabet_len; i >= 0; --i)
        {
            m_context_freqs_0[i] >>= 1;
            ++m_context_freqs_0[i];
            m_cum_context_0 = cum_freq;
            cum_freq += m_context_freqs_0[i];
        }
    }

    //Update Context 1
    m_context_freqs_1[m_context_1][index] += m_increment_1;
    m_cum_context_1[m_context_1] += m_increment_1;
    //Rescale if cumulated frequency passes FREQ_MAX
    if (m_cum_context_1[m_context_1] >= FREQ_MAX)
    {
        cum_freq = 0;
        for (i = m_alphabet_len; i >= 0; --i)
        {
        m_context_freqs_1[m_context_1][i] >>= 1;
        ++m_context_freqs_1[m_context_1][i];
        m_cum_context_1[m_context_1] = cum_freq;
        cum_freq += m_context_freqs_1[m_context_1][i];
        }
    }

    //Update Context n
    m_context_freqs_n[m_context_n][index] += m_increment_n;
    m_cum_context_n[m_context_n] += m_increment_n;
    //Rescale if cumulated frequency passes FREQ_MAX
    if (m_cum_context_n[m_context_n] >= FREQ_MAX)
    {
        cum_freq = 0;
        for (i = m_alphabet_len; i >= 0; --i)
        {
        m_context_freqs_n[m_context_n][i] >>= 1;
        ++m_context_freqs_n[m_context_n][i];
        m_cum_context_n[m_context_n] = cum_freq;
        cum_freq += m_context_freqs_n[m_context_n][i];
        }
    }
    //Update Contexts
    m_Cache = ((m_Cache << m_context_bit_shift) | (index - 1));
    m_context_1 = m_Cache & m_context_1_mask;
    m_context_n = m_Cache & m_context_n_mask;

    //Set Current Frequencies
    cum_freq = 0;
    int *s_0 = m_context_freqs_0 + m_alphabet_len;
    int *s_1 = m_context_freqs_1[m_context_1] + m_alphabet_len;
    int *s_n = m_context_freqs_n[m_context_n] + m_alphabet_len;

    for (int *ptr = m_cum_freqs + m_alphabet_len; ptr >= m_cum_freqs;)
    {
        *ptr-- = cum_freq;
        cum_freq += *s_0-- + *s_1-- + *s_n-- + 1;
    }
    }
    else
    {
    //increment frequency
    m_freqs[index] += m_increment_0;
    //Update frequencies on top
    while (index)
        m_cum_freqs[--index] += m_increment_0;
    //Rescale if cumulated frequency passes FREQ_MAX
    if (*m_cum_freqs >= FREQ_MAX)
    {
        cum_freq = 0;
        for (i = m_alphabet_len; i >= 0; --i)
        {
        m_freqs[i] >>= 1;
        ++m_freqs[i];
        m_cum_freqs[i] = cum_freq;
        cum_freq += m_freqs[i];
        }
    }
    }
}
void TCModel::SetCurrentFrequencies()
{
    int cum_freq = 0;
    int *s_0 = m_context_freqs_0 + m_alphabet_len;
    int *s_1 = m_context_freqs_1[m_context_1] + m_alphabet_len;
    int *s_n = m_context_freqs_n[m_context_n] + m_alphabet_len;
    for (int *ptr = m_cum_freqs + m_alphabet_len; ptr >= m_cum_freqs;)
    {
    *ptr-- = cum_freq;
    cum_freq += *s_0-- + *s_1-- + *s_n-- + 1;
    }
}
TCModel::~TCModel()
{
    delete[] m_cum_freqs;
    if (m_n)
    {
    delete[] m_block;
    delete[] m_context_freqs_1;
    delete[] m_context_freqs_n;
    delete[] m_cum_context_1;
    delete[] m_cum_context_n;
    }
    else
    delete[] m_freqs;
}
////////////////////////////////////////////////////////
//       RangeCoder Funtions  (Encoding)              //
////////////////////////////////////////////////////////
void TRangeCoder::StartEncoding(BYTE *output, unsigned out_pos)
{
    m_output = output;
    m_out_pos = out_pos;
    m_low = 0;
    m_range = RANGE_CODER_TOP_VALUE;
    m_buffer = 0;
    m_bytes2follow = 0;
}
void TRangeCoder::RenormalizeEncoding()
{
    while (m_range <= RANGE_CODER_BOTTOM_VALUE)
    {
    if (m_low < (0xff << RANGE_CODER_SHIFT_BITS)) // no carry possible --> output
    {
        m_output[m_out_pos++] = m_buffer;
        for (; m_bytes2follow; --m_bytes2follow)
        m_output[m_out_pos++] = 0xFF;
        m_buffer = m_low >> RANGE_CODER_SHIFT_BITS;
    }
    else if (m_low & RANGE_CODER_TOP_VALUE) // carry now, no future carry
    {
        m_output[m_out_pos++] = m_buffer + 1;
        for (; m_bytes2follow; --m_bytes2follow)
        m_output[m_out_pos++] = 0x00;
        m_buffer = m_low >> RANGE_CODER_SHIFT_BITS;
    }
    else // passes on a potential carry
        ++m_bytes2follow;
    m_range <<= 8;
    m_low = (m_low << 8) & (RANGE_CODER_TOP_VALUE - 1);
    }
}
void TRangeCoder::EncodeSequence(BYTE *seq, unsigned seq_len, TCModel &model)
{
    int index;
    DWORD range, low;
    while (seq_len--)
    {
    index = *seq++ + 1;
    if (m_range <= RANGE_CODER_BOTTOM_VALUE)
        RenormalizeEncoding();

    range = m_range / (DWORD)*model.m_cum_freqs;
    if (model.m_cum_freqs[index - 1] < *model.m_cum_freqs)
    {
        low = range * (DWORD)model.m_cum_freqs[index];
        m_low += low;
        m_range = range * (DWORD)(model.m_cum_freqs[index - 1] - model.m_cum_freqs[index]);
    }
    else
    {
        low = range * (DWORD)model.m_cum_freqs[index];
        m_low += low;
        m_range -= low;
    }
    model.Update(index);
    }
}
int TRangeCoder::DoneEncoding()
{
    DWORD temp;
    if (m_range <= RANGE_CODER_BOTTOM_VALUE)
    RenormalizeEncoding(); //Now we have a normalized state

    temp = (m_low >> RANGE_CODER_SHIFT_BITS) + 1;
    if (temp > 0xFF) // we have a carry
    {
    m_output[m_out_pos++] = m_buffer + 1;
    for (; m_bytes2follow; --m_bytes2follow)
        m_output[m_out_pos++] = 0x00;
    }
    else // no carry
    {
    m_output[m_out_pos++] = m_buffer;
    for (; m_bytes2follow; --m_bytes2follow)
        m_output[m_out_pos++] = 0xFF;
    }
    m_output[m_out_pos++] = temp & 0xFF;
    m_output[m_out_pos++] = 0;
    return m_out_pos;
}
////////////////////////////////////////////////////////
//       RangeCoder Funtions  (Decoding)              //
////////////////////////////////////////////////////////
void TRangeCoder::StartDecoding(BYTE *input, unsigned in_pos)
{
    m_input = input;
    m_in_pos = in_pos + 1;
    m_buffer = m_input[m_in_pos++];
    m_low = m_buffer >> (RANGE_CODER_BITS_PER_BYTE - RANGE_CODER_EXTRA_BITS);
    m_range = 1 << RANGE_CODER_EXTRA_BITS;
    m_bytes2follow = 0;
}
void TRangeCoder::RenormalizeDecoding()
{
    while (m_range <= RANGE_CODER_BOTTOM_VALUE)
    {
    m_low = (m_low << RANGE_CODER_BITS_PER_BYTE) | ((m_buffer << RANGE_CODER_EXTRA_BITS) & 0xFF);
    m_buffer = m_input[m_in_pos++];
    m_low |= (m_buffer >> (RANGE_CODER_BITS_PER_BYTE - RANGE_CODER_EXTRA_BITS));
    m_range <<= RANGE_CODER_BITS_PER_BYTE;
    }
}
void TRangeCoder::DecodeSequence(BYTE *seq, unsigned seq_len, TCModel &model)
{
    DWORD range, Temp, Cum, D, cum_freq_0;
    int index;

    while (seq_len--)
    {
    if (m_range <= RANGE_CODER_BOTTOM_VALUE)
        RenormalizeDecoding();
    cum_freq_0 = (DWORD) * (model.m_cum_freqs);

    range = m_range / cum_freq_0;
    Temp = m_low / range;
    if (Temp >= cum_freq_0)
        Cum = cum_freq_0 - 1;
    else
        Cum = Temp;

    // Calculating index for cumulated frequencies
    index = 1;
    int *ss = model.m_cum_freqs + 1;
    while (*ss++ > Cum)
        ++index;

    *seq++ = index - 1;
    Cum = model.m_cum_freqs[index];

    Temp = range * Cum;
    m_low -= Temp;
    D = (DWORD)(model.m_cum_freqs[index - 1]) - Cum;
    if (Cum + D < cum_freq_0)
        m_range = range * D;
    else
        m_range = m_range - Temp;
    model.Update(index);
    }
}
int TRangeCoder::DoneDecoding()
{
    if (m_range <= RANGE_CODER_BOTTOM_VALUE)
    RenormalizeDecoding();
    m_in_pos++;
    return m_in_pos - RANGE_CODER_BYTE_RESET;
}
////////////////////////////////////////////////////////
//       Arithmetic Coder Funtions  (Encoding)        //
////////////////////////////////////////////////////////
void TArithmeticCoder::StartEncoding(BYTE *output, unsigned out_pos)
{
    m_out_pos = out_pos;
    m_output = output + out_pos;
    dec_low = 0;
    dec_up = (1 << N) - 1;
    E3_count = 0;
    code_index = 0;
}
void TArithmeticCoder::EncodeSequence(BYTE *seq, unsigned seq_len, TCModel &model)
{
    int index, b;
    while (seq_len--)
    {
    index = *seq++ + 1;

    dec_low_new = dec_low + (dec_up - dec_low + 1) * model.m_cum_freqs[index] / (model.m_cum_freqs[0]);
    dec_up = dec_low + (dec_up - dec_low + 1) * model.m_cum_freqs[index - 1] / (model.m_cum_freqs[0]) - 1;
    dec_low = dec_low_new;

    // Check for E1, E2 or E3 conditions and keep looping as long as they occur.
    while ((bitget(dec_low, N) == bitget(dec_up, N)) || (bitget(dec_low, N_1) && !bitget(dec_up, N_1)))
    { // If it is an E1 or E2 condition,
        if (bitget(dec_low, N) == bitget(dec_up, N))
        {
        if (b = bitget(dec_low, N))
            bitout(m_output, code_index, 1);
        else
            ++code_index;

        dec_low <<= 1;
        dec_up <<= 1;
        ++dec_up;

        // Check if E3_count is non-zero and transmit appropriate bits
        if (!b)
            while (E3_count--)
            bitout(m_output, code_index, 1);
        else
            code_index += E3_count;
        E3_count = 0;

        // Reduce to N for next loop
        bitunset(dec_low, N);
        bitunset(dec_up, N);
        }
        // Else if it is an E3 condition
        else if ((bitget(dec_low, N_1)) && !(bitget(dec_up, N_1)))
        {
        dec_low <<= 1;
        dec_up <<= 1;
        ++dec_up;

        bitunset(dec_low, N);
        bitunset(dec_up, N);

        dec_low ^= 1 << N_1;
        dec_up ^= 1 << N_1;

        ++E3_count;
        } //if
    }     //while
    model.Update(index);
    } //while
}
int TArithmeticCoder::DoneEncoding()
{
    int i, b;
    if (!E3_count)
    { // Just transmit the final value of the lower bound bin_low
    i = N;
    while (i)
    {
        if (bitget(dec_low, i))
        bitout(m_output, code_index, 1);
        else
        ++code_index;
        --i;
    }
    }
    else
    {
    // Transmit the MSB of bin_low.
    if (b = bitget(dec_low, N))
        bitout(m_output, code_index, b);
    else
        ++code_index;

    // Then transmit complement of b (MSB of bin_low), E3_count times.
    if (!b)
        while (E3_count--)
        bitout(m_output, code_index, 1);
    else
        code_index += E3_count;
    E3_count = 0;

    // Then transmit the remaining bits of bin_low
    i = N_1;
    while (i)
    {
        if (bitget(dec_low, i))
        bitout(m_output, code_index, 1);
        else
        ++code_index;
        --i;
    }
    }
    m_out_pos += floor(code_index / 8.0);
    return m_out_pos;
}
////////////////////////////////////////////////////////
//       Arithmetic Coder Funtions  (Decoding)        //
////////////////////////////////////////////////////////
void TArithmeticCoder::StartDecoding(BYTE *input, unsigned in_pos)
{
    // Initialize the lower and upper bounds.
    dec_low = 0;
    dec_up = (1 << N) - 1;
    E3_count = 0;
    m_input = input + in_pos;
    code_index = 0;
    m_in_pos = in_pos;

    // Read the first N number of bits into a temporary tag bin_tag
    dec_tag = 0;
    int i = N;
    while (i)
    {
    if (bitin(m_input, code_index))
        dec_tag += 1 << (i - 1);
    --i;
    }
}
void TArithmeticCoder::DecodeSequence(BYTE *seq, unsigned seq_len, TCModel &model)
{
    int index;
    while (seq_len--)
    {
    dec_tag_new = ((dec_tag - dec_low + 1) * model.m_cum_freqs[0] - 1) / (dec_up - dec_low + 1);

    index = 1;
    while (model.m_cum_freqs[index] > dec_tag_new)
        ++index;

    dec_low_new = dec_low + ((dec_up - dec_low + 1) * model.m_cum_freqs[index] / model.m_cum_freqs[0]);
    dec_up = dec_low + ((dec_up - dec_low + 1) * model.m_cum_freqs[index - 1] / model.m_cum_freqs[0]) - 1;
    dec_low = dec_low_new;

    // Check for E1, E2 or E3 conditions and keep looping as long as they occur.
    while ((bitget(dec_low, N) == bitget(dec_up, N)) || (bitget(dec_low, N_1) && !bitget(dec_up, N_1)))
    {
        if (bitget(dec_low, N) == bitget(dec_up, N))
        {
        dec_low <<= 1;
        dec_up <<= 1;
        dec_up++;

        dec_tag <<= 1;
        dec_tag += bitin(m_input, code_index);

        bitunset(dec_low, N);
        bitunset(dec_up, N);
        bitunset(dec_tag, N);
        }
        // Else if it is an E3 condition
        else if ((bitget(dec_low, N_1)) && !(bitget(dec_up, N_1)))
        {
        //Left shifts and update
        dec_low <<= 1;
        dec_up <<= 1;
        ++dec_up;

        // Left shift and read in code
        dec_tag <<= 1;
        dec_tag += bitin(m_input, code_index);

        // Reduce to N for next loop
        bitunset(dec_low, N);
        bitunset(dec_up, N);
        bitunset(dec_tag, N);

        // Complement the new MSB of dec_low, dec_up and dec_tag
        dec_low ^= 1 << N_1;
        dec_up ^= 1 << N_1;
        dec_tag ^= 1 << N_1;
        }
    }
    *seq++ = index - 1;
    model.Update(index);
    }
}
int TArithmeticCoder::DoneDecoding()
{
    m_in_pos += floor(code_index / 8.0);
    return m_in_pos;
}
