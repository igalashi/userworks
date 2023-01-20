#ifndef E16DAQ_SAMPLER_H_
#define E16DAQ_SAMPLER_H_

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <FairMQDevice.h>

#include "frontend/Daq.h"
#include "unpacker/MQHeader.h"

namespace boost::program_options {
class optoins_description;
}
namespace e16::daq {

class Daq;

//______________________________________________________________________________
class Sampler : public FairMQDevice {
public:
   enum class ChannelID : int {
      BeginRun,
      Data,
      NoTag,
      EndRun,
      Pedestal,
      Status,
      SpillStart,
      SpillEnd,
      Summary,
      Unknown,
      N
   };

   const std::string fClassName;
   struct OptionKey {
      static constexpr std::string_view RunNumber{"run_number"};

      // rep (reply) socket is assumed
      static constexpr std::string_view BeginRunDataChannelName{"begin-run-data"};

      // push socket is assumed
      static constexpr std::string_view DataChannelName{"data"};
      static constexpr std::string_view NoTagDataChannelName{"no-tag-data"};
      static constexpr std::string_view EndRunDataChannelName{"end-run-data"};
      static constexpr std::string_view PedestalDataChannelName{"pedestal-data"};
      static constexpr std::string_view StatusDataChannelName{"status-data"};
      static constexpr std::string_view SpillStartDataChannelName{"spill-start-data"};
      static constexpr std::string_view SpillEndDataChannelName{"spill-end-data"};
      static constexpr std::string_view SummaryDataChannelName{"summary-data"};
      static constexpr std::string_view UnknownDataChannelName{"unknown-data"};

      static constexpr std::string_view MaxIteration{"max-iteration"};
      static constexpr std::string_view PollTimeout{"poll-timeout"};
      static constexpr std::string_view NoHeader{"no-header"};

      static constexpr std::string_view EventWait{"event-wait"};
      static constexpr std::string_view WaitStopCommand{"wait-stop-command"};
      static constexpr std::string_view RoundRobin{"round-robin"};
   };

   static constexpr std::string_view RoundRobinByEventID{"event"};
   static constexpr std::string_view RoundRobinBySpillID{"spill"};
   static constexpr std::string_view RoundRobinByTimestamp{"timestamp"};
   static constexpr std::string_view RoundRobinBySequenceID{"sequence"};
   enum class RoundRobinPolicy : int {
      RRNone,
      RREvent,
      RRSpill,
      RRTimestamp,
      RRSequence,
   };
   static void AddOptions(boost::program_options::options_description &options);

   Sampler() : FairMQDevice(), fClassName(__func__) {}
   Sampler(const Sampler &) = delete;
   Sampler &operator=(const Sampler &) = delete;
   ~Sampler() override = default;

   int64_t GetRunNumber() const { return fRunNumber; }

   void ProcessBeginRunData();
   void ProcessData();
   void ProcessEndRunData();
   void ProcessIncompleteData();

protected:
   void CreateBeginRunData();
   void CreateDaq();
   FairMQMessagePtr CreateHeader();
   uint64_t GetDirection(ChannelID id);
   void HandleRequest();
   void Init() override;
   void InitTask() override;
   void PostRun() override;
   void PreRun() override;
   void Run() override;
   void ResetTask() override;
   int64_t Send(FairMQParts &parts, ChannelID id, bool copyMessage = false, bool sameMessage = false);
   void SetChannelName(ChannelID id, std::string_view key);

   std::atomic<uint32_t> fEventID{0};
   std::atomic<uint16_t> fSpillID{0};
   std::atomic<uint64_t> fTimestamp{0};
   std::atomic<int64_t> fNumIteration{-1};
   int64_t fMaxIteration{-1};
   int fPollTimeoutMS;

   std::vector<std::string> fChList;
   std::vector<std::string> fDqmChList;
   std::vector<int> fNDestination;

   bool fUseFileSampler{false};
   bool fNoHeader{false};
   int64_t fRunNumber{-1};
   std::unique_ptr<Daq> fDaq;
   uint32_t fFemType;
   FairMQParts fBeginRunData;
   std::atomic<bool> fDaqThreadEnd;
   bool fWaitStopCommand;
   int fEventWaitMS;
   RoundRobinPolicy fRoundRobin;
};

} // namespace e16::daq

#endif
