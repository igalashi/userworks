#ifndef HulStrTdcEmulator_h
#define HulStrTdcEmulator_h

#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <atomic>
#include <memory>

#include <FairMQDevice.h>
#include <FairMQPoller.h>

namespace highp::e50 {


class HulStrTdcEmulator : public FairMQDevice
{
public: 
  struct OptionKey {
    static constexpr std::string_view MaxIterations     {"max-iterations"};
    static constexpr std::string_view MsgRate           {"msg-rate"};
    static constexpr std::string_view MsgSize           {"msg-size"};
    static constexpr std::string_view HBFRate           {"hbf-rate"};
    static constexpr std::string_view NumHBF            {"num-hbf"};
    static constexpr std::string_view HBFPosition       {"hbf-position"};
    static constexpr std::string_view OutputChannelName {"out-chan-name"};
  };

  HulStrTdcEmulator();
  HulStrTdcEmulator(const HulStrTdcEmulator&)            = delete;
  HulStrTdcEmulator& operator=(const HulStrTdcEmulator&) = delete;
  ~HulStrTdcEmulator() = default;

  void ResetMsgCounter();

protected:
  bool ConditionalRun() override;
  void Init() override; 
  void InitTask() override; 
  void PreRun() override;
  void PostRun() override;

  int fHBFPosition {0};
  int fMsgSize {0};
  std::atomic<int> fMsgCounter {0};
  int fMsgRate {0};
  int fHBFRate {0};
  int fHBFId   {0};
  int fNumHBF  {1};
  uint64_t fNumIterations {0};
  uint64_t fMaxIterations {0};
  std::string fOutputChannelName;
  std::thread fResetMsgCounter;
  FairMQPollerPtr fPoller;

};

} // namespace highp::e50


#endif
