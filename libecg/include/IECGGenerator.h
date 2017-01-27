#pragma once
#include <functional>

FORWARD_DECL(IECGGenerator)

typedef std::function<void(const ECGSample &)> ECGSampleCallback;
typedef std::shared_ptr<ECGSampleCallback> ECGSampleCallbackSPtr;

class IECGGenerator
{
public:


    /*!@brief Starts playing the ECG signal
     *!@param[in] playSpeed Ratio to real time sampling rate. Passing a negative value generates the samples as fast as possible! */
    virtual void play(double playSpeed) = 0;

    /*!@brief Returns whether the generator is currently reproducing the ECG Signal !*/
    virtual bool isPlaying() const = 0;

    /*!@brief Stops playing the ECG signal if currently playing !*/
    virtual void stop() = 0;

    /*!@brief Connects the event fired when a new ECG sample is available !*/
    virtual ECGSampleCallbackSPtr connectSampleEvent(ECGSampleCallback &evnt) = 0;

    /*!@brief Disconnects a callback invoked every time a new ECG sample is available. If such callback doesn't exist nothing happens !*/
    virtual void disconnectSampleEvent(ECGSampleCallbackSPtr registration) = 0;

public:
    virtual ~IECGGenerator() = default;
};