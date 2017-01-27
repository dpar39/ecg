#include <gtest/gtest.h>

#include "data/config.h"

#include "ECGSignal.h"


class ECGSignalTests : public ::testing::Test
{

protected:
    ECGSignalSPtr m_ecgSignal;


    void InitializeFromFile(const std::string &fileName = "data/hrecg5min/p003")
    {
        auto fullPath = resolvePath(fileName);
        m_ecgSignal = ECGSignal::fromDaqViewFile(fullPath);
    }

};

TEST_F(ECGSignalTests, CanLoadSignalFromDaqviewFile)
{
    InitializeFromFile();

    EXPECT_EQ(4, m_ecgSignal->numChannels());

    EXPECT_EQ(1000.0, m_ecgSignal->sampleRateHz());

    EXPECT_EQ(300.0, m_ecgSignal->recordLengthSec());
}

