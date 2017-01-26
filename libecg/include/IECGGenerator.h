#pragma once
#include <functional>

FORWARD_DECL(IECGSignalGenerator)

typedef std::function<void(const ECGSample &)> ECGSampleEvent;

class IECGSignalGenerator
{
public:
    /*!@brief ECG record length in seconds !*/
    virtual double recordLengthSec() const = 0;

    /*!@brief Number of channels in ECG record !*/
    virtual size_t numChannels() const = 0;

    /*!@brief ECG signal sampling rate in Hz !*/
    virtual double sampleRateHz() const = 0;

    /*!@brief Starts playing the ECG signal
     *!@param[in] playSpeed Ratio to real time sampling rate. Passing a negative value generates the samples as fast as possible! */
    virtual void play(double playSpeed) = 0;

    /*!@brief Stops playing the ECG signal if currently playing !*/
    virtual void stop() = 0;

    /*!@brief Connects the event fired when a new ECG sample is available !*/
    virtual void connectSampleEvent(ECGSampleEvent evnt) = 0;

    /*!@brief Disconnects a callback invoked every time a new ECG sample is available. If such callback doesn't exist nothing happens !*/
    virtual void disconnectSampleEvent(ECGSampleEvent evnt) = 0;

public:
    virtual ~IECGSignalGenerator() = default;
};