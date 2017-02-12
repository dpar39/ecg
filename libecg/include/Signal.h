
#pragma once
#include <memory>

#include <vector>

#include "common.h"
#include "IECGSignal.h"


FORWARD_DECL(ECGSignal)


class ECGSignal : public IECGSignal
{
public:

    size_t numChannels() const override { return m_numChannels; }

    double recordLengthSec() const override { return m_samples.size() / m_samplingRateHz; }

    double sampleRateHz() const override { return m_samplingRateHz; }

    static ECGSignalSPtr fromDaqViewFile(const std::string &ecgFileName);

    ECGSampleIter cbegin() const override { return m_samples.cbegin(); };

    ECGSampleIter cend() const override { return  m_samples.cend(); };
private:
    std::vector<ECGSample> m_samples;

    size_t m_numChannels;

    double m_samplingRateHz;
};
