#ifndef Sampler_h
#define Sampler_h

#include <string>
#include <cstdint>

#include "FairMQDevice.h"

namespace highp::e50 {

class Sampler : public FairMQDevice
{
public:
  Sampler();
  Sampler(const Sampler&)            = delete;
  Sampler& operator=(const Sampler&) = delete;
  ~Sampler()                         = default;

protected:
  void Init() override;
  void InitTask() override;
  bool ConditionalRun() override;



protected:
  std::string fText;
  uint64_t fMaxIterations {0};
  uint64_t fNumIterations {0};
  uint64_t fRunId {0};
};

} // namespace highp::e50


#endif
