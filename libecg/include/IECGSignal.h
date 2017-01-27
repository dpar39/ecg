#pragma once

#include "common.h"

FORWARD_DECL(IECGSignal)

class IECGSignal
{
public:
    virtual ~IECGSignal() = default;

    /*!@brief ECG record length in seconds !*/
    virtual double recordLengthSec() const = 0;

    /*!@brief Number of channels in ECG record !*/
    virtual size_t numChannels() const = 0;

    /*!@brief ECG signal sampling rate in Hz !*/
    virtual double sampleRateHz() const = 0;

    virtual ECGSampleIter cbegin() const = 0;

    virtual ECGSampleIter cend() const = 0;
};
