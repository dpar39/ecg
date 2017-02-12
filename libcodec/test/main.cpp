/*
   This program compresses the MIT-BIH >Arrhythmia database.
*/
#include "BWCoder.h"
#include "TSignal.h"
#include "CommonDef.h"

#include <windows.h>
#include <time.h>
#include <iostream>

#include <data/config.h>

#include <boost/filesystem.hpp>

#define MITDB_SIZE 48
#define DBBEGIN 0
#define CLK_TCK (double)CLOCKS_PER_SEC

using namespace std;
//////////////////////////////////////////
//   MIT-BIH Arrhythmia Database files  //
//////////////////////////////////////////


Signal S; //Class to keep the signal

///////////////////////////
//  Experiments Results  //
///////////////////////////
//Compression Rates
double CR[MITDB_SIZE];
//Compression/Decompression Times
double T[MITDB_SIZE],
       BWC_T[MITDB_SIZE], BWT_T[MITDB_SIZE],
       IVR_T[MITDB_SIZE], ECD_T[MITDB_SIZE];
//Average Match Lengths of the sequences in the BWC
double AML[MITDB_SIZE];
//Alphabet size
double AS[MITDB_SIZE];

///////////////////////////////////
//  Using S+P Transform Measures //
///////////////////////////////////
//  Stream sizes of S+P low pass and high pass coeff.
double SS_S[MITDB_SIZE], SS_D[MITDB_SIZE];
//Compression/Decompression Times on S+P low pass and high pass coeff.
double BWC_T_S[MITDB_SIZE], BWT_T_S[MITDB_SIZE],
       IVR_T_S[MITDB_SIZE], ECD_T_S[MITDB_SIZE],
       BWC_T_D[MITDB_SIZE], BWT_T_D[MITDB_SIZE],
       IVR_T_D[MITDB_SIZE], ECD_T_D[MITDB_SIZE];

//Average Match Lengths of S+P low pass and high pass coeff.
double AML_S[MITDB_SIZE], AML_D[MITDB_SIZE];
//Alphabet size of S+P low pass and high pass coeff.
double AS_S[MITDB_SIZE], AS_D[MITDB_SIZE];



//Main Function
int main(int argc, char* argv[])
{
    namespace fs = boost::filesystem;
    using std::cout;
    using std::cin;

    std::vector<std::string> ecgRecords = { "100","101","102","103","104","105","106","107","108","109",
        "111","112","113","114","115","116","117","118","119","121","122","123","124",
        "200","201","202","203","205","207","208","209","210","212","213","214","215",
        "217","219","220","221","222","223","228","230","231","232","233","234" };

    string mitbihdir = resolvePath("data/MIT-BIH");
    
    const string compressDir = "compress";
    const string expandDir = "expand";

    if (!fs::exists(compressDir))
    {
        fs::create_directories(compressDir);
    }
    if (!fs::exists(expandDir))
    {
        fs::create_directories(expandDir);
    }



    //Parameters for Compression/Expansion
    bool CalculateAML = false;
    char answer;
    double hold;

    cout << "----------------------------------------------------------" << endl;
    cout << " This Program outputs reported measures compressing the   " << endl;
    cout << " 48 ECG registers of the MIT-BIH Arrhythmia Database.     " << endl;
    cout << "----------------------------------------------------------" << endl << endl;

    cout << "First, make sure the original 48 ECG registers are located" << endl;
    cout << "in a directory named MITDB that is in this executable path." << endl;
    cout << "Two directories named COMPRESS and EXPAND will be created " << endl;
    cout << "in this program's path to store compressed and expanded files." << endl << endl;

START:
    cout << "	Select which channel you want to compress:               " << endl;
    cout << "    0. Both Channels                                      " << endl;
    cout << "    1. Channel 1                                          " << endl;
    cout << "    2. Channel 2                                          " << endl;
    cout << "Channel: ";
    cin >> S.SaveSpecifiedChannel;
    cout << endl << endl;

    cout << "Specify the First-Order Linear Prediction algorithm you want to apply to the signals: " << endl;
    cout << "    0. No Prediction                                                                  " << endl;
    cout << "    1. DPCM (Differential Pulse Code Modulation)                                      " << endl;
    cout << "    2. Linear Prediction with a = 0.9865 (Proposed by Arnavut)                        " << endl;
    cout << "    3. Linear Prediction calculating <a> from the autocorrelation of the sequence     " << endl;
    cout << "LP Algorithm: ";
    cin >> S.LinearPredictionMethod;
    cout << endl << endl;

    cout << "Apply S+P Transform? (y/n): ";
    cin >> answer;
    cout << endl << endl;
    S.UseSplusPTransform = answer == 'y' || answer == 'Y' || answer == '1';

    if (S.UseSplusPTransform)
    {
        cout << "Use independent Threads to Encode/Decode approximants and details? (y/n): ";
        cin >> answer;
        cout << endl << endl;
        S.CreateTwoThreads = answer == 'y' || answer == 'Y' || answer == '1';
    }
    if (S.SaveSpecifiedChannel)
    {
        cout << "Calculate Average Match Length AML on the sequences? (y/n): ";
        cin >> answer;
        cout << endl << endl;
        CalculateAML = answer == 'y' || answer == 'Y' || answer == '1';
    }
    cout << "Convert to unsigned integers before BWC? (y/n): ";
    cin >> answer;
    cout << endl << endl;
    S.bw.Convert2UIntBeforeBWT = answer == 'y' || answer == 'Y' || answer == '1';

    cout << "Select Sort Mode of the symbols (by their frequency of occurrence) before IF stage:" << endl;
    cout << "   0. No Sort                                                                      " << endl;
    cout << "   1. Descending Sort                                                              " << endl;
    cout << "   2. Ascending Sort                                                               " << endl;
    cout << "Sort Mode: ";
    cin >> S.bw.SortMode;
    cout << endl << endl;

    cout << "Percent of the file to be saved (%): ";
    cin >> S.percent_to_save;
    cout << endl << endl;
    S.percent_to_save /= 100.0;

    if (CalculateAML)
    {
        S.bw.aml = 0;
    }

    char ecg_filename[128], sfilen[8];

    // COMPRESSION OF THE MITDB_SIZE REGISTERS
    cout << ">>>>>> Compressing ECG signals... Please wait..." << endl;
    clock_t tic;

    int i = 0;
    for (const auto &ecgFile : ecgRecords)
    {
        cout << ">> Compressing ECG Signal: " << ecgFile << ".DAT" << endl;

        auto ecgFilePath = (fs::path(mitbihdir) / (ecgFile + ".DAT")).string();
        auto ecgFilePathCompressed = (fs::path(compressDir) / (ecgFile + ".ecg")).string();

        tic = clock();
        S.readECG(ecgFilePath);
      
        CR[i] = S.saveECG(ecgFilePathCompressed.c_str());

        T[i] = (clock() - tic) / CLK_TCK;

        BWC_T[i] = S.bw.bwc_time;
        BWT_T[i] = S.bw.bwt_time;
        IVR_T[i] = S.bw.ivr_time;
        ECD_T[i] = S.bw.enc_time;

        cout << "     Compression Rate: " << CR[i] << endl;
        cout << "     Compression Time: " << T[i] << " seconds." << endl;

        if (S.SaveSpecifiedChannel)
        {
            if (S.UseSplusPTransform)
            {
                SS_S[i] = S.bwS.output_len;
                SS_D[i] = S.bwD.output_len;
                BWC_T_S[i] = S.bwS.bwc_time;
                BWC_T_D[i] = S.bwD.bwc_time;
                cout << "     Low pass stream compression size: " << 100 * SS_S[i] / (SS_S[i] + SS_D[i]) << " %." << endl;
                cout << "     Low pass stream compression time: " << 100 * BWC_T_S[i] / (BWC_T_S[i] + BWC_T_D[i]) << " %." << endl;
                cout << "     Alphabet Size of low  pass BWT coefficients: " << (AS_S[i] = S.bwS.alphaLen) << " symbols." << endl;
                cout << "     Alphabet Size of high pass BWT coefficients: " << (AS_D[i] = S.bwD.alphaLen) << " symbols." << endl;

                BWC_T_S[i] = S.bwS.bwc_time;
                BWC_T_D[i] = S.bwD.bwc_time;
                BWT_T_S[i] = S.bwS.bwt_time;
                BWT_T_D[i] = S.bwD.bwt_time;
                IVR_T_S[i] = S.bwS.ivr_time;
                IVR_T_D[i] = S.bwD.ivr_time;
                ECD_T_S[i] = S.bwS.enc_time;
                ECD_T_D[i] = S.bwD.enc_time;
                if (CalculateAML)
                {
                    cout << "     AML of low pass/high pass coeff. seq: " << (AML_S[i] = S.bwS.aml) << "/" << (AML_D[i] = S.bwD.aml) << " symbols." << endl;
                    S.bwS.aml = 0;
                    S.bwS.aml = 0;
                }
            }
            else
            {
                cout << "     Alphabet Size of the BWT sequence: " << (AS[i] = S.bw.alphaLen) << " symbols." << endl;
                if (CalculateAML)
                {
                    cout << "     AML of the BWT sequence: " << (AML[i] = S.bw.aml) << " symbols." << endl;
                    S.bw.aml = 0;
                }
            }
        }
    }

    //Report Compression Results....
    cout << endl;
    cout << "----------------------------------------------------------" << endl;
    cout << "            +++++ Compression Results +++++               " << endl;
    cout << "----------------------------------------------------------" << endl;
    cout << endl;
    cout << "  >> Compression Rate = " << mean(CR,MITDB_SIZE) << " +/- " << std_dev(CR, MITDB_SIZE) << endl;
    cout << "  >> Compression Time = " << sum(T,MITDB_SIZE) << " seconds (" << mean(T,MITDB_SIZE) << " +/- " << std_dev(T, MITDB_SIZE) << " seconds per file)" << endl;
    cout << endl;
    if (S.SaveSpecifiedChannel)
    {
        cout << "  BWC Statistics: " << endl;
        if (S.UseSplusPTransform)
        {
            cout << "  >> Low pass stream compression size: " << 100 * sum(SS_S,MITDB_SIZE) / (sum(SS_S,MITDB_SIZE) + sum(SS_D,MITDB_SIZE)) << " %." << endl;
            cout << "  >> Low pass stream compression time: " << 100 * sum(BWC_T_S,MITDB_SIZE) / (sum(BWC_T_S,MITDB_SIZE) + sum(BWC_T_D,MITDB_SIZE)) << " %." << endl;
            cout << "  >> Alphabet Size of the low  pass BWT sequences = " << mean(AS_S,MITDB_SIZE) << " +/- " << std_dev(AS_S, MITDB_SIZE) << " symbols" << endl;
            cout << "  >> Alphabet Size of the high pass BWT sequences = " << mean(AS_D,MITDB_SIZE) << " +/- " << std_dev(AS_D, MITDB_SIZE) << " symbols" << endl;
            if (CalculateAML)
            {
                cout << "  >> AML of the low  pass BWT sequence = " << mean(AML_S,MITDB_SIZE) << " +/- " << std_dev(AML_S, MITDB_SIZE) << " symbols" << endl;
                cout << "  >> AML of the high pass BWT sequence = " << mean(AML_D,MITDB_SIZE) << " +/- " << std_dev(AML_D, MITDB_SIZE) << " symbols" << endl;
            }
            cout << "  >> BWC Low  pass encoding time: " << sum(BWC_T_S, MITDB_SIZE) << " seconds" << endl;
            cout << "     (" << 100 * sum(BWT_T_S, MITDB_SIZE) / sum(BWC_T_S, MITDB_SIZE) << "% on BWT Stage, " << 100 * sum(IVR_T_S, MITDB_SIZE) / sum(BWC_T_S, MITDB_SIZE)
                << "% on IF Stage and " << 100 * sum(ECD_T_S, MITDB_SIZE) / sum(BWC_T_S, MITDB_SIZE) << "% on EC Stage)" << endl;
            cout << "  >> BWC High pass encoding time: " << sum(BWC_T_D, MITDB_SIZE) << " seconds" << endl;
            cout << "     (" << 100 * sum(BWT_T_D, MITDB_SIZE) / sum(BWC_T_D, MITDB_SIZE) << "% on BWT Stage, " << 100 * sum(IVR_T_D, MITDB_SIZE) / sum(BWC_T_D, MITDB_SIZE)
                << "% on IF Stage and " << 100 * sum(ECD_T_D, MITDB_SIZE) / sum(BWC_T_D, MITDB_SIZE) << "% on EC Stage)" << endl;
        }
        else
        {
            cout << "  >> Alphabet Size of the BWT sequences = " << mean(AS,MITDB_SIZE) << " +/- " << std_dev(AS, MITDB_SIZE) << " symbols" << endl;
            if (CalculateAML)
                cout << "  >> AML of the BWT sequence = " << mean(AML,MITDB_SIZE) << " +/- " << std_dev(AML, MITDB_SIZE) << " symbols" << endl;
            cout << "  >> BWC Encoding Time: " << sum(BWC_T, MITDB_SIZE) << " seconds" << endl;
            cout << "     (" << 100 * sum(BWT_T, MITDB_SIZE) / sum(BWC_T, MITDB_SIZE) << "% on BWT Stage, " << 100 * sum(IVR_T, MITDB_SIZE) / sum(BWC_T, MITDB_SIZE)
                << "% on IF Stage and " << 100 * sum(ECD_T, MITDB_SIZE) / sum(BWC_T, MITDB_SIZE) << "% on EC Stage)" << endl;
        }
    }
    cout << "----------------------------------------------------------" << endl;
    cout << endl;
    cout << "Do you want to SKIP decoding? (y/n): ";
    cin >> answer;
    cout << endl;
    if (answer == 'y' || answer == 'Y' || answer == '1')
        goto END;
    cout << endl;

    // DECOMPRESSION OF THE MITDB_SIZE REGISTERS
    cout << ">>>>>> Decompressing... Please wait..." << endl;
    i = 0;
    for (const auto &ecgFile : ecgRecords)
    {
        tic = clock();
        cout << ">> Decompressing ECG Signal: " << ecgFile << ".ECG" << endl;

        auto ecgFilePathCompressed = (fs::path(compressDir) / (ecgFile + ".ecg")).string();
        auto ecgFilePath = (fs::path(expandDir) / (ecgFile + ".DAT")).string();

        S.readECG(ecg_filename);

        if (!S.SaveSpecifiedChannel) //Note that DAT format is for two-channel signals
        {
            S.saveECG(ecg_filename);
        }

        T[i] = (clock() - tic) / CLK_TCK;
        BWC_T[i] = S.bw.bwc_time;
        BWT_T[i] = S.bw.bwt_time;
        IVR_T[i] = S.bw.ivr_time;
        ECD_T[i] = S.bw.enc_time;

        BWC_T_S[i] = S.bwS.bwc_time;
        BWC_T_D[i] = S.bwD.bwc_time;
        BWT_T_S[i] = S.bwS.bwt_time;
        BWT_T_D[i] = S.bwD.bwt_time;
        IVR_T_S[i] = S.bwS.ivr_time;
        IVR_T_D[i] = S.bwD.ivr_time;
        ECD_T_S[i] = S.bwS.enc_time;
        ECD_T_D[i] = S.bwD.enc_time;
        cout << "     Decompression Time: " << T[i] << " seconds." << endl;
    }

    //Report Deompression Results....
    cout << endl;
    cout << "----------------------------------------------------------" << endl;
    cout << "          +++++ Decompression Results  +++++              " << endl;
    cout << "----------------------------------------------------------" << endl;
    cout << endl;
    cout << "  >> Decompression Time = " << sum(T,MITDB_SIZE) << " seconds (" << mean(T,MITDB_SIZE) << " +/- " << std_dev(T, MITDB_SIZE) << " seconds per file)" << endl;
    cout << endl;
    if (S.SaveSpecifiedChannel)
    {
        cout << "  BWC Statistics: " << endl;
        if (S.UseSplusPTransform)
        {
            cout << "  >> BWC Decoding Time of low  pass coefficients: " << sum(BWC_T_S, MITDB_SIZE) << " seconds" << endl;
            cout << "     (" << 100 * sum(BWT_T_S, MITDB_SIZE) / sum(BWC_T_S, MITDB_SIZE) << "% on BWT Stage, " << 100 * sum(IVR_T_S, MITDB_SIZE) / sum(BWC_T_S, MITDB_SIZE)
                << "% on IF Stage and " << 100 * sum(ECD_T_S, MITDB_SIZE) / sum(BWC_T_S, MITDB_SIZE) << "% on EC Stage)" << endl;
            cout << "  >> BWC Decoding Time of high pass coefficients: " << sum(BWC_T_D, MITDB_SIZE) << " seconds" << endl;
            cout << "     (" << 100 * sum(BWT_T_D, MITDB_SIZE) / sum(BWC_T_D, MITDB_SIZE) << "% on BWT Stage, " << 100 * sum(IVR_T_D, MITDB_SIZE) / sum(BWC_T_D, MITDB_SIZE)
                << "% on IF Stage and " << 100 * sum(ECD_T_D, MITDB_SIZE) / sum(BWC_T_D, MITDB_SIZE) << "% on EC Stage)" << endl;
        }
        else
        {
            cout << "  >> BWC Decoding Time: " << sum(BWC_T, MITDB_SIZE) << " seconds" << endl;
            cout << "     (" << 100 * sum(BWT_T, MITDB_SIZE) / sum(BWC_T, MITDB_SIZE) << "% on BWT Stage, " << 100 * sum(IVR_T, MITDB_SIZE) / sum(BWC_T, MITDB_SIZE)
                << "% on IF Stage and " << 100 * sum(ECD_T, MITDB_SIZE) / sum(BWC_T, MITDB_SIZE) << "% on EC Stage)" << endl;
        }
    }
    cout << "----------------------------------------------------------" << endl;
    cout << endl;
    if (S.SaveSpecifiedChannel == 0)
    {
        cout << "Do you want to compare original and recovered files? (y/n): ";
        cin >> answer;
        cout << endl << endl;
        if (answer == 'y' || answer == 'Y' || answer == '1')
        {
            char f_ori[128], f_rec[128];
            for (const auto &ecgFile : ecgRecords)
            {
                cout << ">> Comparing original and recovered version of the file " << ecgFile << ".DAT ..." << endl;

                auto ecgOriginalFilePath = (fs::path(mitbihdir) / (ecgFile + ".DAT")).string();
                auto ecgDecodedFilePath = (fs::path(expandDir) / (ecgFile + ".DAT")).string();

                compare_files(ecgOriginalFilePath.c_str(), ecgDecodedFilePath.c_str()); //see definition in CommonDef.cpp
            }
        }
    }
END:
    cout << "Do you want to test with another options? (y/n):";
    cin >> answer;
    cout << endl;
    if (answer == 'y' || answer == 'Y' || answer == '1')
        goto START;
    return 0;
}
