#ifndef BenchmarkSampler_h
#define BenchmarkSampler_h

#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <atomic>
#include <memory>

#include <FairMQDevice.h>
#include <FairMQPoller.h>

namespace highp::e50 {


class BenchmarkSampler : public FairMQDevice
{
public: 
  struct OptionKey {
    static constexpr std::string_view FEMId             {"fem-id"}; 
    static constexpr std::string_view MaxIterations     {"max-iterations"};
    static constexpr std::string_view MsgRate           {"msg-rate"};
    static constexpr std::string_view MsgSize           {"msg-size"};
    static constexpr std::string_view HBFRate           {"hbf-rate"};
    static constexpr std::string_view HBFPosition       {"hbf-position"};
    static constexpr std::string_view OutputChannelName {"out-chan-name"};
  };

  BenchmarkSampler();
  BenchmarkSampler(const BenchmarkSampler&)            = delete;
  BenchmarkSampler& operator=(const BenchmarkSampler&) = delete;
  ~BenchmarkSampler() = default;

  void ResetMsgCounter();

protected:
  bool ConditionalRun() override;
  void Init() override; 
  void InitTask() override; 
  void PreRun() override;
  void PostRun() override;

  int fHBFPosition {0};
  unsigned int fMsgSize {0};
  std::atomic<int> fMsgCounter {0};
  int fMsgRate {0};
  int fHBFRate {0};
  int fHBFId   {0};
  uint64_t fNumIterations {0};
  uint64_t fMaxIterations {0};
  uint32_t    fFEMId {0};
  std::string fOutputChannelName;
  std::thread fResetMsgCounter;
  FairMQPollerPtr fPoller;

};

} // namespace highp::e50


#endif
