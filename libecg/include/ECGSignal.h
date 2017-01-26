
#pragma once
#include <memory>

#include "IECGSignal.h"

class ECGSignal;
typedef std::shared_ptr<ECGSignal> ECGSignalSPtr;

class ECGSignal : public IECGSignal
{
public:
    ~ECGSignal() { }
};
