#ifndef HulStrTdcSampler_h
#define HulStrTdcSampler_h

#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <atomic>
#include <memory>

#include <FairMQDevice.h>
#include <FairMQPoller.h>

namespace highp::e50 {


class HulStrTdcSampler : public FairMQDevice
{
public: 
  struct OptionKey {
    static constexpr std::string_view IpSiTCP           {"sitcp-ip"}; 
    static constexpr std::string_view OutputChannelName {"out-chan-name"}; 
    static constexpr std::string_view TotFilterEn       {"en-totfilter"}; 
    static constexpr std::string_view TotMinTh          {"tot-minth"}; 
    static constexpr std::string_view TotMaxTh          {"tot-maxth"}; 
    static constexpr std::string_view TotZeroAllow      {"en-totzero"}; 
    static constexpr std::string_view TWCorr0           {"corr0"}; 
    static constexpr std::string_view TWCorr1           {"corr1"}; 
    static constexpr std::string_view TWCorr2           {"corr2"}; 
    static constexpr std::string_view TWCorr3           {"corr3"}; 
    static constexpr std::string_view TWCorr4           {"corr4"}; 
  };

  HulStrTdcSampler();
  HulStrTdcSampler(const HulStrTdcSampler&)            = delete;
  HulStrTdcSampler& operator=(const HulStrTdcSampler&) = delete;
  ~HulStrTdcSampler() = default;

  int ConnectSocket(const char* ip);
  int Event_Cycle(uint8_t* buffer);
  int receive(int sock, char* data_buf, unsigned int length);

protected:
  bool ConditionalRun() override;
  void Init() override; 
  void InitTask() override; 
  void PreRun() override;
  void PostRun() override;
  void ResetTask() override; 

  int fHulSocket {0};
  std::string fIpSiTCP {"0"};
  std::string fOutputChannelName {"out"};

  int fTotFilterEn     {0};
  int fTotMinTh        {0};
  int fTotMaxTh        {200};
  int fTotZeroAllow    {1};
  int fTWCorr0         {0};
  int fTWCorr1         {0};
  int fTWCorr2         {0};
  int fTWCorr3         {0};
  int fTWCorr4         {0};
  
  const int fnByte         {5};
  const int fnWordPerCycle {16384*10};

  FairMQPollerPtr fPoller;

};

} // namespace highp::e50


#endif
