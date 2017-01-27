
#include "ECGSignal.h"

#include <gmock/gmock.h>

FORWARD_DECL(MockECGSignal)

class MockECGSignal : public IECGSignal
{
public:
    MOCK_CONST_METHOD0(recordLengthSec, double ());
    MOCK_CONST_METHOD0(numChannels, size_t ());
    MOCK_CONST_METHOD0(sampleRateHz, double ());
    MOCK_CONST_METHOD0(cbegin, ECGSampleIter ());
    MOCK_CONST_METHOD0(cend, ECGSampleIter ());
};
