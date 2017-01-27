#include <gtest/gtest.h>

#include "ECGGenerator.h"
#include "MockECGSignal.h"

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