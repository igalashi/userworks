#ifndef TimeStampInserter_h
#define TimeStampInserter_h

#include <string>
#include <string_view>
#include <cstdint>

#include <FairMQDevice.h>
#include <FairMQParts.h>

namespace highp::e50 {

class TimeStampInserter : public FairMQDevice
{
public:
  struct OptionKey {
      static constexpr std::string_view InputChannelName  {"in-chan-name"};
      static constexpr std::string_view OutputChannelName {"out-chan-name"};
  };

  TimeStampInserter();
  TimeStampInserter(const TimeStampInserter&)            = delete;
  TimeStampInserter& operator=(const TimeStampInserter&) = delete;
  ~TimeStampInserter() = default;

protected:
  bool HandleData(FairMQParts& msgParts, int index);
  void Init() override;
  void InitTask() override;

private:
  std::string fInputChannelName;
  std::string fOutputChannelName;

};

} // namespace highp::e50

#endif
