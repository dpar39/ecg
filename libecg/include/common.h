#pragma once

#include <memory>
#include <string>

#define FORWARD_DECL(ClassName) \
class ClassName; \
typedef std::shared_ptr<##ClassName> ##ClassName##SPtr;

#include <tuple>

typedef std::tuple<double, double, double, double> ECGSample;

typedef std::vector<ECGSample>::const_iterator ECGSampleIter;

#define ROUND_INT(x) (static_cast<int>(x+0.5))

