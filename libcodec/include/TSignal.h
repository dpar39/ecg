#ifndef ECGSignalH
#define ECGSignalH

#include "BWCoder.h"
#include "CommonDef.h"
#include <cmath>
#include <string>
#include <vector>

//This class is the core of the Program
class Signal {
private:
    void ForwardLinearPredictionBlock(short* x, int len);
    void InverseLinearPredictionBlock(short* x, int len);
    void ForwardSPTransformBlock(short* x, int len, short*& s, short*& d);
    short* InverseSPTransformBlock(short* s, short* d, int len);

    BYTE* CompressLead(short* s, int s_len, int& enc_len);
    short* ExpandLead(BYTE*& stream, int& s_len);

public:
    //FIELDS
    char FileName[256]; //Full file name of the signal

    std::vector<int16_t> m_channels[4];

    short* ch[4]; //Array of channels(leads) that keep the signal
    //X, Y, Z leads and Vcm (common mode noise)
    double m_samplingFreqHz; //sampling frequency.  (Not used in this program)
    unsigned m_numSamples; //Number of samples in every lead
    unsigned m_numChannels; //Number of channels in use
    double percent_to_save; //Percent of the record to be saved.
    int SaveSpecifiedChannel; //0 -> Save all channels, 1 -> Save channel 1 2 -> Save channel 2

    //First order linear prediction coefficient (a)
    double a;

    //Parameters for BWT compression
    BWC bw, bwS, bwD; //bwS and bwD are the compression parameters
    //for the low pass and high pass coeff of the S+P transform

    //Linear Prediction Method
    //Set 1 for differentiator,
    //Set 2 for linear prediction with a = 0.9865 (Arnavut´s proposed coeff.)
    //Set 3 to estimate best (a) from the aoutocorrelation functions (MMSE)
    //Set 4 to predict using a Weighted Least Squared Error( WLSE)
    int LinearPredictionMethod;

    //Use the S+P Transform
    bool UseSplusPTransform;
    //Create two threads to compress aproximants and details
    bool CreateTwoThreads;

    //S+P Predictors
    int a_left, a_center, a_right, beta;
    double common_denominator;

    Signal(); // default constructor
    ~Signal();

    //METHODS ----- INTERFACE FUNCTIONS  ****
    void readECG(const std::string& ecgFilePath);
    double saveECG(const char* file_name);
};
#endif
