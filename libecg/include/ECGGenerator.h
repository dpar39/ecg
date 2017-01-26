#pragma once

#include "common.h"

#include "IECGSignalGenerator.h"
#include <vector>

FORWARD_DECL(ECGSignalGenerator)

class ECGSignalGenerator : public IECGSignalGenerator
{
public:

    size_t numChannels() const override { return m_numChannels; }

    double recordLengthSec() const { return m_samples.size() / m_samplingRateHz; }

    double sampleRateHz() const { return m_samplingRateHz; }

    static ECGSignalGeneratorSPtr fromDaqViewFile(const std::string &ecgFileName);

    void play(double playSpeed) override;

    void stop() override;

    void connectSampleEvent(ECGSampleEvent evnt) override;

    void disconnectSampleEvent(ECGSampleEvent evnt) override;

private:

    std::vector<ECGSample> m_samples;

    size_t m_numChannels;

    double m_samplingRateHz;
};