#include "ECGGenerator.h"

#include <thread>
#include <future>


void ECGGenerator::play(double playSpeed)
{
    if (m_playFuture.valid() && m_playFuture.wait_for(std::chrono::seconds(0))
        == std::future_status::timeout)
    {
        // We are already playing
        // TODO: Log this
        return;
    }

    m_isPlaying = true;
    m_playFuture = std::async(std::launch::async, [playSpeed, this]()
    {
        auto it = m_ecgSignal->cbegin();
        auto itBeg = m_ecgSignal->cbegin();
        auto itEnd = m_ecgSignal->cend();

        if (playSpeed <= 0)
        {
            // Play samples as fast as possible
            do
            {
                onSampleGenerated(*it++);
            }
            while (it != itEnd && m_isPlaying);
        }
        else
        {
            auto startPlayTime = std::chrono::steady_clock::now();
            auto actualPeriod_ns = ROUND_INT(1000000.0 / m_ecgSignal->sampleRateHz() / playSpeed);
            do
            {
                onSampleGenerated(*it++);
                auto newTime = startPlayTime + std::chrono::nanoseconds(std::distance(itBeg, it)*actualPeriod_ns);
                std::this_thread::sleep_until(newTime);
            } 
            while (it != itEnd && m_isPlaying);
        }
        m_isPlaying = false;
    });
}

bool ECGGenerator::isPlaying() const
{
    return m_isPlaying;
}

void ECGGenerator::stop()
{
    m_isPlaying = false;
    m_playFuture.get();
}

ECGSampleCallbackSPtr ECGGenerator::connectSampleEvent(ECGSampleCallback& evnt)
{
    std::lock_guard<std::mutex> g(m_mutex);

    auto registration = std::make_shared<ECGSampleCallback>(std::move(evnt));
    m_observers.push_back(registration);
    return registration;
}

void ECGGenerator::disconnectSampleEvent(ECGSampleCallbackSPtr registration)
{
    std::lock_guard<std::mutex> g(m_mutex);

    auto it = std::find(m_observers.begin(), m_observers.end(), registration);
    if (it != m_observers.end())
    {
        m_observers.erase(it);
    }
}

void ECGGenerator::onSampleGenerated(const ECGSample& sample)
{
    std::lock_guard<std::mutex> g(m_mutex);
    for (const auto &observer : m_observers)
    {
        try 
        {
            (*observer)(sample);
        }
        catch(const std::exception &e)
        {
            // TODO: Log this exception
            throw e;
        }
    }
}
