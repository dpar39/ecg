//---------------------------------------------------------------------------

#pragma once

#include "CommonDef.h"
#define NO_SORT    0
#define DESCENDING 1
#define ASCENDING  2

#define ALPHA_LEN 4096

//////////////////////////////////////////////////
//    Main Interfase of the BWC (Enco & Deco)   //
//////////////////////////////////////////////////
struct BWC
{
    double bwc_time, bwt_time, ivr_time, enc_time;
    unsigned input_len, output_len;
    bool Convert2UIntBeforeBWT;
    double aml; //if aml == 0 the aml is calculated on the sequence
    int alphaLen;
    int SortMode; //Sort mode of the symbols by thier frequency of occurrence
    int E2Level;
    unsigned m_bwtIndex;



    short* symbols;
    BYTE* stream;
 };

void BWCEncode(BWC& block);
void BWCDecode(BWC& block);

//////////////////////////////////////////////
//    Internal Subroutines of the BWC       //
//////////////////////////////////////////////
short* ForwardBWT(BWC& blck);
short* InverseBWT(short* bwt_seq, unsigned Len, unsigned index, short offset);
int BestSortMode(int* F, short& range);
int* GetForwardPermutation(int* F, short& range, int sort_mode);
int* GetInversePermutation(int* F, short& range, int sort_mode);
int EstimateStreamLength(int* F, int range, int seq_len);
void ssort2(short** a, int n, int depth);


