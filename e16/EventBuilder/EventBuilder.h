#ifndef E16DAQ_EVENT_BUILDER_H_
#define E16DAQ_EVENT_BUILDER_H_

#include <atomic>
#include <cstdint>
#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <FairMQDevice.h>
#include <FairMQParts.h>

#include "unpacker/GlobalTag.h"
#include "unpacker/MQHeader.h"

namespace e16::daq {

class EventBuilder : public FairMQDevice {
public:
   const std::string fClassName;
   struct OptionKey {
      static constexpr std::string_view InputDataChannelName{"in"};

      // req-rep is assumed (EventBuilder (server) has a reply socket and Monitors (clients) have a request socket)
      static constexpr std::string_view BeginRunDataChannelName{"begin-run-data"};

      // output channels. push-pull is assumed
      static constexpr std::string_view StatusDataChannelName{"status-data"};
      static constexpr std::string_view SpillStartDataChannelName{"spill-start-data"};
      static constexpr std::string_view SpillEndDataChannelName{"spill-end-data"};
      static constexpr std::string_view CompleteDataChannelName{"out"};
      static constexpr std::string_view IncompleteDataChannelName{"incomplete"};

      static constexpr std::string_view BufferTimeoutMS{"buffer-timeout"};
      static constexpr std::string_view NumSource{"num-source"};
      static constexpr std::string_view NumSourceStatus{"num-source-status"};
      static constexpr std::string_view NumSourceSpillStart{"num-source-spill-start"};
      static constexpr std::string_view NumSourceSpillEnd{"num-source-spill-end"};
      static constexpr std::string_view NumSink{"num-sink"};
      static constexpr std::string_view RoundRobin{"round-robin"};
      static constexpr std::string_view NumRetry{"num-retry"};
      static constexpr std::string_view PollTimeout{"poll-timeout"};
      static constexpr std::string_view RunNumber{"run_number"};

      static constexpr std::string_view Sort{"sort"};
      static constexpr std::string_view Discard{"discard"};
   };

   static constexpr std::string_view RoundRobinByEventID{"event"};
   static constexpr std::string_view RoundRobinBySpillID{"spill"};
   static constexpr std::string_view RoundRobinByTimestamp{"timestamp"};
   static constexpr std::string_view RoundRobinBySequenceID{"sequence"};

   enum class ChannelID : int {
      // Input,
      BeginRun,
      Status,
      SpillStart,
      SpillEnd,
      Complete,
      Incomplete,
      N
   };

   enum class RoundRobinPolicy : int {
      RRNone,
      RREvent,
      RRSpill,
      RRTimestamp,
      RRSequence,
   };

   using time_point = std::chrono::high_resolution_clock::time_point;

   struct Fragment {
      FairMQParts parts;
      time_point time;

      Fragment() = default;
      Fragment(Fragment &&other) noexcept = default;
      Fragment(FairMQParts &&m, time_point &&t) noexcept
      {
         for (auto &x : m) {
            parts.AddPart(std::move(x));
         }
         time = std::move(t);
      }
      ~Fragment() = default;
   };

   struct EventFrame {
      uint32_t fMessageType;
      GlobalTag fTag;
      GlobalTag fTagMask;
      std::unordered_map<uint64_t, Fragment> fFrame;
   };
   // EventFrame = std::unordered_map<uint64_t, Fragment>;

   EventBuilder() : FairMQDevice(), fClassName(__func__) {}
   EventBuilder(const EventBuilder &) = delete;
   EventBuilder &operator=(const EventBuilder &) = delete;
   ~EventBuilder() override = default;

private:
   void Build();
   void CheckTimeout();
   void Clear();
   bool ConditionalRun() override;
   FairMQParts CopyBeginRunData();
   FairMQMessagePtr CreateHeader(uint32_t bodySize, uint32_t messageType);
   uint64_t FindTag(const MQHeader &h, const char *msgBegin, const char *msgEnd);
   void Init() override;
   void InitTask() override;
   void PostRun() override;
   void PreRun() override;
   void ReceiveRequest();
   bool Recv();
   bool Send(FairMQParts &parts, ChannelID id, int direction = 0);
   void SendBeginRunData(uint64_t srcId, FairMQParts &parts);
   bool SendFrame(FairMQParts &&parts, ChannelID id, uint32_t msg_type);
   bool SendFrame(EventFrame &&ev, ChannelID id, uint32_t msg_type);
   void SetChannelName(ChannelID id, std::string_view key);

   std::vector<std::string> fChList;
   std::vector<std::string> fDqmChList;
   std::string fInputDataChannelName;

   int fNumSource{0};
   int fNumSourceStatus{0};
   int fNumSourceSpillStart{0};
   int fNumSourceSpillEnd{0};
   int fBufferTimeoutMS{10000};
   int fPollTimeoutMS{100};

   std::unordered_map<uint64_t, FairMQParts> fBeginRunData;
   std::unordered_map<uint64_t, std::deque<Fragment>> fInBuffer;
   FairMQPollerPtr fPoller;

   bool fSort;
   bool fDiscard;
   uint64_t fHeaderSrcId{0};
   int64_t fRunNumber{0};
   int fNumSequence{0};
   int fNumSink{0};
   RoundRobinPolicy fRoundRobin;
   int fNumRetry{10};
   GlobalTag fTagMaskMin;
   GlobalTag fTagMaskMax;
   bool fTagMaskReady;
   std::unordered_map<std::string, EventFrame> fEventFrames;
};

} // namespace e16::daq

#endif
