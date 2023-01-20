#ifndef TimeFrameBuilder_h
#define TimeFrameBuilder_h

#include <string>
#include <string_view>
#include <cstdint>
#include <chrono>

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <FairMQDevice.h>


class TimeFrameBuilder : public FairMQDevice
{
public:
  struct OptionKey {
    static constexpr std::string_view NumSource         {"num-source"};
    static constexpr std::string_view BufferTimeoutInMs {"buffer-timeout"};
    static constexpr std::string_view InputChannelName  {"in-chan-name"};
    static constexpr std::string_view OutputChannelName {"out-chan-name"};
  };

  struct STFBuffer {
    FairMQParts parts;
    std::chrono::steady_clock::time_point start;
  };

  TimeFrameBuilder();
  TimeFrameBuilder(const TimeFrameBuilder&)            = delete;
  TimeFrameBuilder& operator=(const TimeFrameBuilder&) = delete;
  ~TimeFrameBuilder() = default;

protected:
  bool ConditionalRun() override;
  void Init() override;
  void InitTask() override;
  void PostRun() override;

private:
  int fNumSource {0};
  int fBufferTimeoutInMs {100000};
  std::string fInputChannelName;
  std::string fOutputChannelName;

  std::unordered_map<uint32_t, std::vector<STFBuffer>> fTFBuffer;
  std::unordered_set<uint64_t> fDiscarded;

};

#endif
