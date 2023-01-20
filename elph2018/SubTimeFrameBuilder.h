#ifndef SubTimeFrameBuilder_h
#define SubTimeFrameBuilder_h

#include <string>
#include <string_view>
#include <cstdint>
#include <deque>
#include <memory>
#include <queue>

#include <FairMQDevice.h>

namespace highp::e50 {

class SubTimeFrameBuilder : public FairMQDevice
{
public:
  using WorkBuffer = std::unique_ptr<std::vector<FairMQMessagePtr>>;
  using RecvBuffer = std::deque<FairMQMessagePtr>;
  using SendBuffer = std::queue<WorkBuffer>;

  struct OptionKey {
    static constexpr std::string_view InputChannelName  {"in-chan-name"};
    static constexpr std::string_view OutputChannelName {"out-chan-name"};
    static constexpr std::string_view StripHBF          {"strip-hbf"};
    static constexpr std::string_view MaxHBF            {"max-hbf"};
  };

  SubTimeFrameBuilder();
  SubTimeFrameBuilder(const SubTimeFrameBuilder&)            = delete;
  SubTimeFrameBuilder& operator=(const SubTimeFrameBuilder&) = delete;
  ~SubTimeFrameBuilder() = default;

protected:
  virtual void BuildFrame(FairMQParts& msgParts, int index);
  bool HandleData(FairMQParts& msgParts, int index);
  void Init() override;
  void InitTask() override;
  void PostRun() override;

private:
  int         fNumDestination {0};
  std::string fInputChannelName;
  std::string fOutputChannelName;
  int fMaxHBF {1};
  int fHBFCounter {0};
  uint64_t fSTFId {0};
  std::size_t fFirst;
  std::size_t fSeek;
  RecvBuffer fInputPayloads;
  WorkBuffer fWorkingPayloads;
  SendBuffer fOutputPayloads;
};

} // namespace highp::e50

#endif
