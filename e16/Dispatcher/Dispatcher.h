#ifndef E16DAQ_DISPATCHER_H_
#define E16DAQ_DISPATCHER_H_

#include <string>
#include <string_view>

#include <FairMQDevice.h>

namespace e16::daq {
class Dispatcher : public FairMQDevice {
public:
   struct OptionKey {
      static constexpr std::string_view InputChannelName{"in"};
      static constexpr std::string_view OutputChannelName{"out"};
      static constexpr std::string_view MonitorChannelName{"monitor"};
      static constexpr std::string_view Multipart{"multipart"};
      static constexpr std::string_view NumRetry{"num-retry"};
   };

   Dispatcher() = default;
   Dispatcher(const Dispatcher &) = delete;
   Dispatcher(const Dispatcher &&) = delete;
   ~Dispatcher() = default;

private:
   bool HandleData(FairMQMessagePtr &m, int index);
   bool HandleMultipartData(FairMQParts &parts, int index);
   void Init() override;
   void InitTask() override;

   std::string fInputChannelName;
   std::string fOutputChannelName;
   std::string fMonitorChannelName;
   int fNumRetry{10};
};

} // namespace e16::daq

#endif
