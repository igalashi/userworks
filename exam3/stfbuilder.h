#ifndef HulStrTdcSTFBuilder_h
#define HulStrTdcSTFBuilder_h

#include <string>
#include <string_view>
#include <cstdint>
#include <deque>
#include <memory>
#include <queue>

#include <FairMQDevice.h>

#include "HulStrTdcData.h"

class HulStrTdcSTFBuilder : public FairMQDevice
{
public:
  using WorkBuffer = std::unique_ptr<std::vector<FairMQMessagePtr>>;
  using RecvBuffer = std::vector<HulStrTdc::Data::Word>;
  using SendBuffer = std::queue<WorkBuffer>;

  struct OptionKey {
    static constexpr std::string_view FEMId             {"fem-id"};
    static constexpr std::string_view InputChannelName  {"in-chan-name"};
    static constexpr std::string_view OutputChannelName {"out-chan-name"};
    static constexpr std::string_view DQMChannelName    {"dqm-chan-name"};
    static constexpr std::string_view StripHBF          {"strip-hbf"};
    static constexpr std::string_view MaxHBF            {"max-hbf"};
    static constexpr std::string_view SplitMethod       {"split"};
  };

  HulStrTdcSTFBuilder();
  HulStrTdcSTFBuilder(const HulStrTdcSTFBuilder&)            = delete;
  HulStrTdcSTFBuilder& operator=(const HulStrTdcSTFBuilder&) = delete;
  ~HulStrTdcSTFBuilder() = default;

private:
  void BuildFrame(FairMQMessagePtr& msg, int index);
  void FillData(HulStrTdc::Data::Word* first, 
                HulStrTdc::Data::Word* last, 
                bool isSpillEnd);
  void FinalizeSTF();
  bool HandleData(FairMQMessagePtr&, int index);
  void Init() override;
  void InitTask() override;
  void NewData();
  void PostRun() override;

  uint64_t fFEMId {0};
  int         fNumDestination {0};
  std::string fInputChannelName;
  std::string fOutputChannelName;
  std::string fDQMChannelName;
  int fMaxHBF {1};
  int fHBFCounter {0};
  uint64_t fSTFId {0};
  int fSplitMethod {0};
  RecvBuffer fInputPayloads;
  WorkBuffer fWorkingPayloads;
  SendBuffer fOutputPayloads;
};

#endif
