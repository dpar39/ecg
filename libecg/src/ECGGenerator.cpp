#include "ECGSignalGenerator.h"
#include <fstream>
#include <map>
#include <sstream>
#include <iostream>


// static
ECGSignalGeneratorSPtr ECGSignalGenerator::fromDaqViewFile(const std::string &ecgFileName)
{
    auto instance = std::make_shared<ECGSignalGenerator>();

    auto io$FileName = ecgFileName + ".IO$";
    auto iotFileName = ecgFileName + ".IOT";

    try 
    {
        // Read metadata for ECG record
        std::ifstream io$(io$FileName, std::ios::in);

        std::vector<double> resol_uV;

        std::string line;
        while (io$.good())
        {
            std::getline(io$, line);
            if (line == "*FREQUENCY")
            {
                io$ >> instance->m_samplingRateHz;
                continue;
            }

            if (line == "*CHANNELS")
            {
                while (io$.good())
                {
                    getline(io$, line);
                    if (line.empty() || line[0] == '*')
                    {
                        continue;
                    }

                    int chNum;
                    double resolution_uV;
                    char chName[12];
                    char chUnits[12];
                    sscanf(line.c_str(), "%d,%f,%s,%s", &chNum, &resolution_uV, chName, chUnits);
                    resol_uV.push_back(resolution_uV);
                }
            }
        }


        std::ifstream iotStream(iotFileName, std::ios::binary | std::ios::in);
        iotStream.seekg(0, iotStream.end);
        auto byteLength = iotStream.tellg();
        iotStream.seekg(0, iotStream.beg);

        auto numValues = byteLength / sizeof(int16_t);

        std::vector<int16_t> rawData(numValues);
        iotStream.read(reinterpret_cast<char *> (rawData.data()), byteLength);

        instance->m_numChannels = resol_uV.size();


        switch (instance->numChannels())
        {
        case 1:
            for (auto it = rawData.cbegin(); it != rawData.cend(); it += 1)
            {
                instance->m_samples.emplace_back(it[0] / resol_uV[0],
                    NAN, NAN, NAN);
            }
            break;
        case 3:
            for (auto it = rawData.cbegin(); it != rawData.cend(); it += 3)
            {
                instance->m_samples.emplace_back(it[0] / resol_uV[0],
                    it[1] / resol_uV[1], it[2] / resol_uV[2], NAN);
            }
            break;
        case 4:
            for (auto it = rawData.cbegin(); it != rawData.cend(); it += 4)
            {
                instance->m_samples.emplace_back(it[0] / resol_uV[0],
                    it[1] / resol_uV[1], it[2] / resol_uV[2], it[3] / resol_uV[3]);
            }
            break;
        default:
            return nullptr;
        }
    }
    catch(const std::runtime_error &e)
    {
        std::cout << "Unable to load file: " << e.what() << std::endl;
        return nullptr;
    }
    return instance;
}



void ECGSignalGenerator::play(double playSpeed)
{
    
}

void ECGSignalGenerator::stop()
{
    
}

void ECGSignalGenerator::connectSampleEvent(ECGSampleEvent evnt)
{
    
}

void ECGSignalGenerator::disconnectSampleEvent(ECGSampleEvent evnt)
{
    
}