#ifndef E16DAQ_MQ_DqmDST0_h
#define E16DAQ_MQ_DqmDST0_h

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <FairMQDevice.h>

#include "utility/FileUtil.h"

namespace boost::program_options {
class options_description;
}

class DqmDS0Impl;
namespace e16::daq {

class DqmDST0 : public FairMQDevice {
public:
   const std::string fClassName;

   struct OptionKey {
      static constexpr std::string_view InputDataChannelName{"in"};
      static constexpr std::string_view PollTimeout{"poll-timeout"};
      static constexpr std::string_view ResetHist{"reset-hist"};
      static constexpr std::string_view RunNumber{"run_number"};
      static constexpr std::string_view Prefix{"prefix"};
      static constexpr std::string_view RunNumberFormat{"run-number-format"};
   };

   DqmDST0() : FairMQDevice(), fClassName(__func__) {}
   DqmDST0(const DqmDST0 &) = delete;
   DqmDST0 &operator=(const DqmDST0 &) = delete;
   ~DqmDST0() override = default;

private:
   bool ConditionalRun() override;
   void InitTask() override;
   void PostRun() override;
   void PreRun() override;
   void Recv();

   int fRunNumber{0};
   FairMQPollerPtr fPoller;
   std::string fInputDataChannelName;
   int fPollTimeoutMS;
   std::unique_ptr<DqmDST0Impl> fImpl;
   std::string fPrefix;
   std::string fRunNumberFormat;
};

} // namespace e16::daq

#endif
