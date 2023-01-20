#ifndef Sink_h
#define Sink_h

#include <cstdint>

#include "FairMQDevice.h"

namespace highp::e50 {

class Sink : public FairMQDevice
{
public: 
  Sink();
  Sink(const Sink&)            = delete;
  Sink& operator=(const Sink&) = delete;
  ~Sink()                      = default;

protected:
  void Init() override;
  void InitTask() override;
  bool HandleData(FairMQMessagePtr&, int);
  void ResetTask() override;

private:
  uint64_t fMaxIterations {0};
  uint64_t fNumIterations {0};


};

} // namespace highp::e50

#endif
