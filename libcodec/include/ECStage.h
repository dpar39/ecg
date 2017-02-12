//---------------------------------------------------------------------------

#ifndef ECStageH
#define ECStageH
#include "CommonDef.h"
#if 1
	#define TCODER TRangeCoder
#else
	#define TCODER TArithmeticCoder
#endif
#define MODELS	TCModel  eModel(E2Level +1, 350, 150, 100, 3, 3); \
				TCModel e2Model(maxE2 + 1, 650, 150, 40, 5, 2);   \
				TCModel  Model1(2, 15);						      \
				TCModel  Model2(4, 10);							  \
				TCModel  Model3(8, 35, 10, 5, 3, 3);			  \
				TCModel  Model0(2, 100);	
#define FREQ_MAX 32768/2 //32*1024

#define RANGE_CODER_BIT_MAX ((DWORD)32)
#define RANGE_CODER_BITS_PER_BYTE 8
#define RANGE_CODER_TOP_VALUE ((DWORD)(1<<(RANGE_CODER_BIT_MAX-1)))
#define RANGE_CODER_BOTTOM_VALUE  ((DWORD)(RANGE_CODER_TOP_VALUE >> RANGE_CODER_BITS_PER_BYTE))
#define RANGE_CODER_SHIFT_BITS ((DWORD)(RANGE_CODER_BIT_MAX - 9))
#define RANGE_CODER_RANGE_MASK ((DWORD)(0xFF << RANGE_CODER_SHIFT_BITS))
#define RANGE_CODER_EXTRA_BITS ((DWORD)(((RANGE_CODER_BIT_MAX - 2)%8) + 1))
#define RANGE_CODER_BYTE_RESET 3

#define N   16
#define N_1 (N-1)
#define bitout(t,toffset,bit) ((t)[(toffset)++>>3]|=(bit)<<(((toffset)&7)))
#define bitin(t,toffset)	  (((1<<((toffset)&7))&((t)[(toffset)++>>3]))?1:0)
#define bitget(v,bitNumber)   (((v)&(1<<((bitNumber)-1)))?1:0)
#define bitunset(v,bitNumber) ((v)&=~(1<<(bitNumber)))
#define bit__set(v,bitNumber) ((v)|=(1<<(bitNumber)))

///////////////////////////////////////////////////////
// Interfase Functions for the Entropy Coding Stage  //
///////////////////////////////////////////////////////
void       EntropyEncode(int *v, unsigned vLen, int E2Level, BYTE *&stream);
void       EntropyDecode(int *v, unsigned vLen, int E2Level, BYTE *&stream);
int		 * RLE0(int *s , unsigned &sLen);
int		 * RLD0(int *sr, unsigned &sLen);

///////////////////////////////////////////////////////
// Classes Definitions for the Entropy Coding Stage  //
///////////////////////////////////////////////////////
class TCModel
{
	protected:
		int m_alphabet_len, //alphabet size
			m_index_len;
		DWORD m_Cache,
		m_context_1,        //Current context 1
		m_context_n,        //Current context n
		m_context_1_mask,   //Context 1 mask
		m_context_n_mask,   //Context n mask
		m_c_1_len,          //Len of context 1
		m_c_n_len;          //Len of context n
		int m_context_bit_shift,   //Number of bits to shift
			m_n,     		//Order n
			m_increment_0,  //Increment order 0
			m_increment_1,  //Increment order 1
			m_increment_n;  //Increment order n

		int  *m_context_freqs_0,
			**m_context_freqs_1,
			**m_context_freqs_n,
			 *m_freqs;

		int  m_cum_context_0,
			 *m_cum_context_1,
			 *m_cum_context_n;

		int *m_block;
		void TCModel::SetCurrentFrequencies();
	public:
		int *m_cum_freqs;    //cumulated frequencies

		TCModel(int n_of_symbols, int inc_0, int inc_1, int inc_n,
				int context_bit_shift, int order);
		TCModel(int n_of_symbols, int inc);
		~TCModel();
		void Reset();
		void Update(int index);
};
class TRangeCoder {
	protected:
		BYTE *m_output,
			 *m_input;
		int m_out_pos,
			m_in_pos;
		DWORD m_low,
			  m_range;
		BYTE  m_buffer;
		int   m_bytes2follow;
	public:
		TRangeCoder() {m_output = m_input = nullptr;};
		void StartEncoding(BYTE *output, unsigned out_pos);
		void RenormalizeEncoding();
		void EncodeSequence(BYTE *seq, unsigned seq_len, TCModel &model);
		int  DoneEncoding();

		void StartDecoding(BYTE *input, unsigned in_pos);
		void RenormalizeDecoding();
		void DecodeSequence(BYTE *seq, unsigned seq_len, TCModel &model);
		int  DoneDecoding();
};
class TArithmeticCoder {
	protected:
		BYTE    *m_output,
			    *m_input;
		int	     m_out_pos,
			     m_in_pos;
		
		int code_index; 

	    DWORD dec_low,               
		      dec_up,
              E3_count,
              dec_low_new,
			  dec_tag,
			  dec_tag_new;		
	public:
		TArithmeticCoder() {m_output = m_input = nullptr;};
		void StartEncoding(BYTE *output, unsigned out_pos);;
		void EncodeSequence(BYTE *seq, unsigned seq_len, TCModel &model);
		int  DoneEncoding();

		void StartDecoding(BYTE *input, unsigned in_pos);
		void DecodeSequence(BYTE *seq, unsigned seq_len, TCModel &model);
		int  DoneDecoding();
};
inline int   bin2exp(int x, int &ba)
{
	if(!x) { ba = 0; return 0; }
	else
	{
		int e = 1, sum = 2;
		while(sum < x) {   ++e; sum += 1<<e; }
		ba = x - (sum - (1<<e) + 1);
		return e;
	}
}
inline int   exp2bin(int e, int  ba)
{
	return e?(ba + (1<<e) - 1):0;
}
inline int   bytes(int x)
{
	if(x <= 0x000000FF)
		return 0;
	else if(x <= 0x0000FFFF)
		return 1;
	else if(x <= 0x00FFFFFF)
		return 2;
	else
		return 3;
}
#endif
