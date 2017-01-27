#pragma once
#include <vector>

#include "common.h"

#include "IECGGenerator.h"
#include "IECGSignal.h"

#include <future>


FORWARD_DECL(ECGGenerator)

class ECGGenerator : public IECGGenerator
{
public:

    explicit ECGGenerator(const IECGSignalSPtr& ecgSignal)
        : m_ecgSignal(ecgSignal)
    {
    }


    void play(double playSpeed) override;

    bool isPlaying() const override;;


    void stop() override;

    ECGSampleCallbackSPtr connectSampleEvent(ECGSampleCallback& evnt) override;

    void disconnectSampleEvent(ECGSampleCallbackSPtr registration) override;

private:
    IECGSignalSPtr m_ecgSignal;

    std::vector<std::shared_ptr<ECGSampleCallback>> m_observers;

    std::future<void> m_playFuture;

    bool m_isPlaying;

    void onSampleGenerated(const ECGSample &sample);

    std::mutex m_mutex;
};