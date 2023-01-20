#ifndef MQSink_h
#define MQSink_h

#include <cstdint>
#include <chrono>
#include <string>
#include <string_view>

#include <FairMQDevice.h>

namespace highp::e50 {

class MQSink : public FairMQDevice
{
public:
  struct OptionKey {
    static constexpr std::string_view NumMessages      = {"num-messages"};
    static constexpr std::string_view InputChannelName = {"in-chan-name"};
  };

  MQSink();
  MQSink(const MQSink&)            = delete;
  MQSink& operator=(const MQSink&) = delete;
  ~MQSink() = default;

protected:
  bool Count(int n);
  bool HandleMultipartMessageData(FairMQParts& parts, int index); 
  //bool HandleSingleMessageData(FairMQMessagePtr& msg, int index);
  void Init() override;
  void InitTask() override;

  std::string fInputChannelName;
  uint64_t    fNumMessages {0};
  uint64_t    fRecvCounter {0};
  uint64_t    fRecvBytes {0};
  std::chrono::steady_clock::time_point fStart;

};

} // namespace highp::e50


#endif
