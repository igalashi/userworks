#ifndef E16DAQ_DQM_h
#define E16DAQ_DQM_h

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <FairMQDevice.h>

namespace e16::daq {

class Dqm : public FairMQDevice {
public:
   const std::string fClassName;

   struct OptionKey {
      // REQ socket is assumed.
      static constexpr std::string_view BeginRunDataChannelName{"begin-run-data"};
      // SUB socket is assumed.
      static constexpr std::string_view InputDataChannelName{"in"};
      static constexpr std::string_view PollTimeout{"poll-timeout"};
   };

   Dqm() : FairMQDevice(), fClassName(__func__) {}
   Dqm(const Dqm &) = delete;
   Dqm &operator=(const Dqm &) = delete;
   ~Dqm() override = default;

   static void AddOptions(boost::program_options::options_description &options);

private:
   bool ConditionalRun() override;
   void InitChannelName();
   void InitDqm();
   void InitTask() override;
   void PreRun() override;
   void PostRun() override;
   void ProcessData(const char *buffer, std::size_t nbytes);
   void Recv();
   void RequestBeginRunData();
   void SetRunNumber();

   FairMQPollerPtr fPoller;
   int fPollTimeoutMS;

   std::string fBeginRunDataChannelName;
   std::string fInputDataChannelName;
   std::chrono::steady_clock::time_point fPrevUpdateTimePoint;
   std::unordered_map<uint64_t, std::vector<std::vector<char>>> fBeginRunData;
};

} // namespace e16::daq

#endif