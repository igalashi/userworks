#ifndef MQSINK_H_
#define MQSINK_H_

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <FairMQDevice.h>

class MQSink : public FairMQDevice {
public:
   const std::string fClassName;
   struct OptionKey {
      static constexpr std::string_view RunId{"run-id"};
      static constexpr std::string_view InputChannelName{"in"};
      static constexpr std::string_view Multipart{"multipart"};
   };

   MQSink() : FairMQDevice(), fClassName(__func__) {}
   MQSink(const MQSink &) = delete;
   MQSink &operator=(const MQSink &) = delete;
   ~MQSink() = default;

private:
   // void Bind() override;
   // bool ConditionalRun() override;
   // void Connect() override;
   bool Count(int n);
   bool HandleData(FairMQMessagePtr &msg, int index);
   bool HandleMultipartData(FairMQParts &msgParts, int index);
   void Init() override;
   void InitTask() override;
   // void PostRun() override;
   void PreRun() override;
   // void Reset() override;
   // void ResetTask() override;
   // void Run() override;

   int32_t fRunId{-1};

   std::string fInputChannelName;
   uint64_t fNumMessages{0};
   uint64_t fRecvCounter{0};
   uint64_t fRecvBytes{0};
   std::chrono::steady_clock::time_point fStart;
};

#endif // MQSINK_H_
