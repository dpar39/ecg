
#include "ECGSignalGenerator.h"

#include <gtest/gtest.h>

#include "data/config.h"


class ECGSignalGeneratorFixture : public ::testing::Test
{

protected:
    ECGSignalGeneratorSPtr m_ecgSignalGenerator;


    void InitializeFromFile(const std::string &fileName = "data/hrecg5min/p003")
    {
        auto fullPath = resolvePath(fileName);
        m_ecgSignalGenerator = ECGSignalGenerator::fromDaqViewFile(fullPath);
    }

};

TEST_F(ECGSignalGeneratorFixture, CanLoadSignalFromFile)
{
    InitializeFromFile();

    EXPECT_EQ(4, m_ecgSignalGenerator->numChannels());

    EXPECT_EQ(1000.0, m_ecgSignalGenerator->sampleRateHz());

    EXPECT_EQ(300.0, m_ecgSignalGenerator->recordLengthSec());
}

TEST_F(ECGSignalGeneratorFixture, SamplesAreGeneratedAtSamplingRate)
{

}