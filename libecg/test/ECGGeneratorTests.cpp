#include <gtest/gtest.h>

#include "ECGGenerator.h"
#include "MockECGSignal.h"
#include <numeric>

using namespace testing;

class ECGGeneratorTests : public Test
{

protected:
    ECGGeneratorSPtr m_ecgGenerator;

    MockECGSignalSPtr m_mockEcgSignal;

    std::vector<ECGSample> m_ecgTestData;

    void SetUp() override
    {
        m_mockEcgSignal = std::make_shared<MockECGSignal>();

        m_ecgGenerator = std::make_shared<ECGGenerator>(m_mockEcgSignal);
    }

    void SetUpSimpleEcgSignal()
    {
        for (auto i = 0; i < 5; ++i)
        {
            m_ecgTestData.emplace_back(i, i + 0.1, i + 0.2, i + 0.3);
        }

        EXPECT_CALL(*m_mockEcgSignal, cbegin()).WillRepeatedly(Return(m_ecgTestData.cbegin()));

        EXPECT_CALL(*m_mockEcgSignal, cend()).WillRepeatedly(Return(m_ecgTestData.cend()));
    }
};

TEST_F(ECGGeneratorTests, PlaysAllSamplesConsecutively)
{
    SetUpSimpleEcgSignal();

    auto numSamples = m_ecgTestData.size();
    auto counter = 0;
    std::condition_variable done;
    ECGSampleCallback lambda = [&counter, &done, &numSamples](const ECGSample &s)
    {
        counter++;
        if(counter == numSamples)
        {
            done.notify_all();
        }
    };
   
    m_ecgGenerator->connectSampleEvent(lambda);

    m_ecgGenerator->play(0);

    std::mutex mtx;
    std::unique_lock<std::mutex> lck(mtx);

    EXPECT_TRUE(m_ecgGenerator->isPlaying());

    std::cv_status status = done.wait_for(lck, std::chrono::milliseconds(10));

    EXPECT_EQ(std::cv_status::no_timeout, status) << "Not all samples were reproduced";

    EXPECT_FALSE(m_ecgGenerator->isPlaying());
}

class ECGGeneratorTestsPlaySpeed : public WithParamInterface<double>,
    public ECGGeneratorTests
{
};

TEST_P(ECGGeneratorTestsPlaySpeed, PlaysAllSamplesAtSamplingFrequency)
{
    auto playSpeed = GetParam();
    auto sampleFreqHz = 1000.0;

    EXPECT_CALL(*m_mockEcgSignal, sampleRateHz()).WillRepeatedly(Return(sampleFreqHz));

    SetUpSimpleEcgSignal();

    auto numSamples = m_ecgTestData.size();
    auto counter = 0;
    std::condition_variable done;

    auto tStart = std::chrono::steady_clock::now();
    std::vector<std::chrono::nanoseconds::rep> durations_ns;
    durations_ns.reserve(numSamples);
    ECGSampleCallback lambda = [&](const ECGSample &s)
    {
        auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>
            (std::chrono::steady_clock::now() - tStart).count();

        durations_ns.push_back(elapsed_ns);

        if (++counter == numSamples)
        {
            done.notify_all();
        }
    };

    m_ecgGenerator->connectSampleEvent(lambda);

    // Verify durations are at the expected sampling period +/- 1ms jitter
    auto period_ns = 1000000000.0 / sampleFreqHz / playSpeed;

    std::vector<double> diffs_ms;
    std::adjacent_difference(durations_ns.begin(), durations_ns.end(), diffs_ms.begin());

    auto it = durations_ns.begin();

    //for (auto it = durations_ns)
}



INSTANTIATE_TEST_CASE_P(InstantiationName, ECGGeneratorTestsPlaySpeed, Values(1.5, 2.1));
