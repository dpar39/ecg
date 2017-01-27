#pragma once
#include <vector>

#include "common.h"

#include "IECGGenerator.h"
#include "IECGSignal.h"

#include <boost/signals2.hpp>
#include <thread>
#include <future>
#include <set>

FORWARD_DECL(ECGGenerator)

class ECGGenerator : public IECGGenerator
{
public:

    explicit ECGGenerator(const IECGSignalSPtr& ecgSignal)
        : m_ecgSignal(ecgSignal)
    {
    }

    void play(double playSpeed) override;

    void stop() override;

    ECGSampleCallbackSPtr connectSampleEvent(ECGSampleCallback& evnt) override;

    void disconnectSampleEvent(ECGSampleCallbackSPtr registration) override;

private:
    IECGSignalSPtr m_ecgSignal;

    std::vector<std::shared_ptr<ECGSampleCallback>> m_observers;

    std::future<void> m_playFuture;

    bool m_playing;

    void onSampleGenerated(const ECGSample &sample);

    std::mutex m_mutex;
};