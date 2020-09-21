//---------------------------------------------------------------------------
#include <cmath>
#include <fstream>

#include "TSignal.h"
#include "CommonDef.h"
#include "BWCoder.h"

#include <filesystem>

// Thread Function to encode/decode data...
DWORD WINAPI ThreadProc( LPVOID lpParam )
{
    BWC *bwp = (BWC *)lpParam;
    if(bwp->stream)
        BWCDecode(*bwp);
    else
        BWCEncode(*bwp);
    return 0;
}
Signal::Signal()
{
    ch[0] = ch[1] = ch[2] = ch[3] = nullptr;
    m_numSamples = 0;
    SaveSpecifiedChannel = 0;
    LinearPredictionMethod = 1;
    UseSplusPTransform = true;
    CreateTwoThreads = false;
    percent_to_save = 1; //Save the whole file

    a_left = -1;
    a_center = 5;
    a_right = 6;
    beta = -3;
    common_denominator = 16.0;

    bw.E2Level = 4;
    bw.aml = 1;
    bw.SortMode = DESCENDING;
    bw.Convert2UIntBeforeBWT = false;
}
Signal::~Signal()
{
    if(ch[0]) delete []ch[0];
    if(ch[1]) delete []ch[1];
    if(ch[2]) delete []ch[2];
    if(ch[3]) delete []ch[3];
}
///////////////////////////////////////////////////////////////////////////
//   Read ECG Signal from file in .DAT format or .ECG compressed format  //
///////////////////////////////////////////////////////////////////////////
void   Signal::readECG(const std::string& ecgFilePath)
{
    using namespace std;
    for (unsigned j = 0; j < 4; j++)
    {
        if (ch[j])
        {
            delete[] ch[j];
            ch[j] = nullptr;
        }
    }

    m_numSamples = 0;
    auto extension = ecgFilePath.substr(ecgFilePath.find_last_of(".") + 1);

    using namespace std;

    std::ifstream fptr(ecgFilePath, std::ios::in | std::ios::binary);
    fptr.unsetf(std::ios::skipws);
    fptr.seekg(std::ios::end);
    auto fileSize = fptr.tellg();
    fptr.seekg(std::ios::beg);

    vector<uint8_t> data;
    data.reserve(fileSize);
    data.assign(istream_iterator<char>(fptr), istream_iterator<char>());

    if (extension == "DAT")
    {
        // MIT-BIH
        m_numChannels = 2;
        m_numSamples = fileSize / 3;  // 3 bytes for 2 samples, 1 for each channel

        for (unsigned i = 0; i < m_numChannels; i++)
            ch[i] = new short[m_numSamples];

        int i = 0;
        for (auto it = data.begin(); it != data.end(); it += 3)
        {
            ch[1][i] = static_cast<int16_t>(it[2]) + ((it[1] & 0xF0) << 4) - 1024;
            ch[0][i] = static_cast<int16_t>(it[0]) + ((it[1] & 0x0F) << 8) - 1024;
            ++i;
        }
    }
    else if (extension == "ECG")
    {
        m_numChannels = 0;

        auto dataPtr = const_cast<BYTE*>(data.data());
        auto dataBeg = dataPtr;

        int dec_len = 0;
        int samples;

        while (dataPtr - dataBeg < fileSize)
        {
            ch[m_numChannels++] = ExpandLead(dataPtr, samples);
        }
        m_numSamples = samples;
    }
}
///////////////////////////////////////////////////////////////////////////
//   Write ECG Signal to file in .DAT format or .ECG compressed format   //
///////////////////////////////////////////////////////////////////////////
double Signal::saveECG(const std::string & file_name)
{
    char file_extension[10];
    strncpy(file_extension, file_name + strlen(file_name) - 3,  3); file_extension[3] = '\0';
    strupr( file_extension );
    int samples_to_save = m_numSamples*percent_to_save;

    try
    {
        if( !strcmp(file_extension, "DAT" ))
        {
            FILE *fptr = fopen(file_name, "wb");

            BYTE *data = new BYTE[3*samples_to_save], *data_begin = data;
            int sample0, sample1;
            short *ch0 = ch[0], *ch1 = ch[1];
            int i = samples_to_save;
            while(i--)
            {
                data[0] = (sample0 = (*ch0++) + 1024)&0x00FF;
                data[2] = (sample1 = (*ch1++) + 1024)&0x00FF;
                data[1] = ((sample1>>8)<<4) + (sample0>>8);
                data += 3;
            }
            fwrite(data_begin, 1, 3*samples_to_save, fptr);
            fclose(fptr);
            delete []data_begin;
            return 1.0;   //No compression in DAT format
        }
        else if( !strcmp(file_extension, "ECG" ))
        {
            std::ofstream fptr(file_name, ios::binary);
            m_numChannels = 2;

            if(!SaveSpecifiedChannel)
            {
                for(unsigned i = 0; i < m_numChannels; i++)
                {
                     int signal_enc_len = 0;
                     BYTE * signal_enc = CompressLead(ch[i], samples_to_save, signal_enc_len);
                     fwrite(signal_enc, 1, signal_enc_len, fptr);
                     delete []signal_enc;
                }
            }
            else
            {
                int signal_enc_len = 0;
                BYTE * signal_enc = CompressLead(ch[SaveSpecifiedChannel - 1], samples_to_save, signal_enc_len);
                fwrite(signal_enc, 1, signal_enc_len, fptr);
                delete []signal_enc;
            }

            double filesize = fptr.ftell();

            if (SaveSpecifiedChannel)
                return samples_to_save*1.5/filesize;      //1 byte and a half for each sample in each channel
            else
                return samples_to_save*m_numChannels*1.5/filesize;
        }
    }
    catch (...)
    {
        throw "Can't save in the specified location. Check access rights are correct.";
    }
    return -1;
}
void   Signal::ForwardLinearPredictionBlock(short *x, int len)
{
    if     (LinearPredictionMethod == 1)
        ForwardDPCM(x, len);
    else if(LinearPredictionMethod == 2)
        diff(x, len, a = 0.9865);
    else if(LinearPredictionMethod == 3)
        diff(x, len, a = lpc0(x, len));
}
void   Signal::InverseLinearPredictionBlock(short *x, int len)
{
    if     (LinearPredictionMethod == 1)
        InverseDPCM(x, len);
    else if(LinearPredictionMethod == 2)
        integrate(x, len, a = 0.9865);
    else if(LinearPredictionMethod == 3)
        integrate(x, len, a);
}
BYTE * Signal::CompressLead(short *s, int s_len, int &enc_len)
{
    short *aprx = nullptr, *dtls = nullptr;       //Aproximant and details after decomposition
    int aprx_len = s_len, dtls_len = 0;     //Apriximant and details array lengths
    BYTE *aprx_enc = nullptr, *dtls_enc = nullptr;//Aproximant and details streams
    int aprx_enc_len = 0, dtls_enc_len = 0; //Apriximant and details array lengths

    BYTE *stream = nullptr, *stream_ptr = nullptr;//Overall enooded lead

    short *x = new short[s_len+2];     //Copy of the original signal
    copy_vector(x, s, s_len);

    if(LinearPredictionMethod)
        ForwardLinearPredictionBlock(x, s_len);
    if(UseSplusPTransform)
    {
        ForwardSPTransformBlock(x, s_len, aprx, dtls);
        delete []x;
        aprx_len = dtls_len = s_len/2;

        bwS = bw; bwD = bw;
        bwS.input_len =  aprx_len;
        bwS.symbols = aprx;
        bwD.input_len =  dtls_len;
        bwD.symbols = dtls;
        bwS.stream = 0;
        bwD.stream = 0;

        if(CreateTwoThreads)
        {
            HANDLE hThread[2];
            DWORD dwThreadId[2];
            hThread[0] = CreateThread(nullptr, 0,ThreadProc, &bwS, 0, &dwThreadId[0]);
            if (hThread[0] == nullptr) ExitProcess(1);
            hThread[1] = CreateThread(nullptr, 0,ThreadProc, &bwD, 0, &dwThreadId[0]);
            if (hThread[1] == nullptr) ExitProcess(1);
            WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
            CloseHandle(hThread[0]); CloseHandle(hThread[1]);
        }
        else
        {
            BWCEncode(bwD);
            BWCEncode(bwS);
        }
        aprx_enc = bwS.stream;
        aprx_enc_len = bwS.output_len;
        delete []aprx;
        dtls_enc = bwD.stream;
        dtls_enc_len = bwD.output_len;
        delete []dtls;

        //Encode in a single stream
        enc_len =  3 + aprx_enc_len + dtls_enc_len;
        stream = stream_ptr = new BYTE[enc_len];
        WriteUnsigned(aprx_enc_len, 3, stream_ptr);
        memcpy(stream_ptr, aprx_enc, aprx_enc_len);
        stream_ptr += aprx_enc_len; delete [] aprx_enc;
        memcpy(stream_ptr, dtls_enc, dtls_enc_len);
        stream_ptr += dtls_enc_len; delete [] dtls_enc;
    }
    else
    {
        bw.input_len =  s_len;
        bw.symbols = x;
        BWCEncode(bw);
        stream = bw.stream;
        enc_len = bw.output_len;
        delete []x;
    }
    return stream;
}
short* Signal::ExpandLead(BYTE *&stream, int &s_len)
{
    short *aprx, *dtls, *x;                   //Approximant and details after decomposition
    int aprx_len = 0, dtls_len = 0;           //Approximant and details array lengths
    BYTE *aprx_enc, *dtls_enc;	              //Approximant and details streams
    int aprx_dec_len = 0, dtls_dec_len = 0;   //Approximant and details array lengths
    int level;

    if(UseSplusPTransform)
    {
        bwS = bw; bwD = bw;
        aprx_dec_len = ReadUnsigned(3,stream);
        bwS.stream = stream;
        bwD.stream = stream + aprx_dec_len;

        if(CreateTwoThreads)
        {
            HANDLE hThread[2];
            DWORD dwThreadId[2];
            hThread[0] = CreateThread(nullptr, 0,ThreadProc, &bwS, 0, &dwThreadId[0]);
            if (hThread[0] == nullptr) ExitProcess(1);
            hThread[1] = CreateThread(nullptr, 0,ThreadProc, &bwD, 0, &dwThreadId[0]);
            if (hThread[1] == nullptr) ExitProcess(1);
            WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
            CloseHandle(hThread[0]); CloseHandle(hThread[1]);
        }
        else
        {
            BWCDecode(bwS);
            BWCDecode(bwD);
        }
        stream += bwS.input_len + bwD.input_len;

        aprx = bwS.symbols;
        aprx_len = bwS.output_len;
        dtls = bwD.symbols;
        dtls_len = bwD.output_len;

        s_len = aprx_len + dtls_len;
        x = InverseSPTransformBlock(aprx, dtls, s_len);
        delete []aprx; delete []dtls;
    }
    else
    {
        bw.stream = stream;
        BWCDecode(bw);
        x = bw.symbols;
        s_len = bw.output_len;
        stream += bw.input_len;
    }
    if(LinearPredictionMethod)
        InverseLinearPredictionBlock(x, s_len);
    return x;
}
void   Signal::ForwardSPTransformBlock(short *x, int len, short *&s, short *&d)
{
    int half = len/2;
    d = new short[half+2];     //details
    s = new short[half+2];     //approximants
    short *pd, *ps, *pd1, *px; //sweep pointers

    short *d1 = new short[half+1];
    d1[half] = 0;  int k;

    k = half; pd1 = d1; ps = s; px = x;
    while(k--)
    {
        *pd1   = *(px+1) - *px; //detail
        *ps++  = *px + floor(*pd1++/2.0); //aproximant
         px   += 2;
    }

    ps = s; pd = d; pd1 = d1;
    short dsl1 = 0, ds0 = 0 - s[0], dsp1 = s[0] - s[1];
    k = half;  s[half] = 0;  ps++;
    while(k--)
    {
        *pd++ = *pd1++ + floor((a_left*dsl1 + a_center*ds0 +
                a_right*dsp1 - beta*(*(pd1+1)))/common_denominator);
        dsl1 = ds0;
        ds0  = dsp1;
        dsp1 = *ps++ - *(ps+1);
    }
    delete []d1;
}

short *Signal::InverseSPTransformBlock(short *s, short *d, int len)
{
    int half = len/2;
    short *x = new short[len];
    short *pd, *ps, *px; //sweep pointers
    int k;

    short dsl1 = s[half-3]-s[half-2], ds0 = s[half-2]-s[half-1], dsp1 = s[half-1] - 0;
    d[half-1] -= floor((a_left*dsl1 + a_center*ds0 + a_right*dsp1)/common_denominator);;

    k = half-3;
    pd = d + half - 2; //last-but-one detail
    ps = s + half - 4;
    while(k--)
    {
        dsp1 = ds0;
        ds0  = dsl1;
        dsl1 = *ps-- - *(ps+1);
        *pd-- -= floor((a_left*dsl1 + a_center*ds0  + a_right*dsp1 - beta*(*(pd+1)))/common_denominator);
    }
    d[1] -= floor((a_left*(0-s[0]) + a_center*(s[0]-s[1]) + a_right*(s[1]-s[2]) - beta*d[2])/common_denominator);
    d[0] -= floor((                  a_center*(    -s[0]) + a_right*(s[0]-s[1]) - beta*d[1])/common_denominator);

    k = half; px = x; pd = d; ps = s;
    while(k--)
    {
        *px    = *ps++ - floor(*pd/2.0);
        *(px+1)= *px + *pd++;
         px   += 2;
    }
    return x;
}
/*
BYTE * Signal::EncodeDetails(short *dtls, int dtls_len, int &dtls_enc_len)
{
    bias2unbias(dtls, dtls_len);
    int *symbols = new int[dtls_len];
    copy_vector(symbols, dtls, dtls_len);
    unsigned dtls_enc_len_u;
    BYTE *dtls_enc = EntropyEncode(symbols, dtls_len, dtls_enc_len_u, 4);
    delete []symbols;
    dtls_enc_len = dtls_enc_len_u;
    return dtls_enc;
}
short *Signal::DecodeDetails(BYTE *stream, int dtls_len, int &dtls_dec_len)
{
    unsigned dtls_dec_len_u;
    int *symbols = EntropyDecode(stream, dtls_len,dtls_dec_len_u, 4);
    dtls_dec_len = dtls_dec_len_u;
    short *dtls = new short[dtls_len];
    copy_vector(dtls, symbols, dtls_len);
    delete []symbols;
    unbias2bias(dtls, dtls_len);
    return dtls;
}

/* int dtls_dec_len = 0; dtls_len = aprx_len;
            dtls = DecodeDetails(stream + dec_len, dtls_len, dtls_dec_len);
            dec_len +=  dtls_dec_len;  */
