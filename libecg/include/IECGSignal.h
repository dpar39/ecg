
#pragma once
#include <memory>

class IECGSignal;
typedef std::shared_ptr<IECGSignal> IECGSignalSPtr;

class IECGSignal
{
public:
    virtual ~IECGSignal() = default;
};
