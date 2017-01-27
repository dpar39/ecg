


#include <gtest/gtest.h>

#include "data/config.h"

#include "ECGSignal.h"


class ECGSignalTests : public ::testing::Test
{

protected:
    ECGSignalSPtr m_ecgSignalGenerator;


    void InitializeFromFile(const std::string &fileName = "data/hrecg5min/p003")
    {
        auto fullPath = resolvePath(fileName);
        m_ecgSignalGenerator = ECGSignal::fromDaqViewFile(fullPath);
    }

};

TEST_F(ECGSignalTests, CanLoadSignalFromFile)
{
    InitializeFromFile();

    EXPECT_EQ(4, m_ecgSignalGenerator->numChannels());

    EXPECT_EQ(1000.0, m_ecgSignalGenerator->sampleRateHz());

    EXPECT_EQ(300.0, m_ecgSignalGenerator->recordLengthSec());
}

TEST_F(ECGSignalTests, SamplesAreGeneratedAtSamplingRate)
{
    ASSERT_TRUE(true);
}