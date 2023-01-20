#include <algorithm>
#include <bitset>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <map>
#include <regex>
#include <thread>

#include <boost/asio.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <runFairMQDevice.h>

#include "utility/HexDump.h"
#include "mq/DaqServiceConstants.h"
#include "mq/MessageUtil.h"
#include "mq/EventBuilder/EventBuilder.h"

using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace highp::e50;

namespace net = boost::asio;
namespace bpo = boost::program_options;
using opt = e16::daq::EventBuilder::OptionKey;

//=============================================================================
void addCustomOptions(bpo::options_description &options)
{
   auto toDqm = [](auto a) -> std::string { return (std::string(a.data()) + "-dqm"); };
   options.add_options()
      //
      (opt::InputDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::InputDataChannelName.data())),
       "Name of input data channel")
      //
      (opt::BeginRunDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::BeginRunDataChannelName.data())),
       "Name of begin-run data channel (reply out)")
      //
      (opt::StatusDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::StatusDataChannelName.data())),
       "Name of status data channel (output)")
      //
      (opt::SpillStartDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::SpillStartDataChannelName.data())),
       "Name of spill-start data channel (output)")
      //
      (opt::SpillEndDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::SpillEndDataChannelName.data())),
       "Name of spill-end data channel (output)")
      //
      (opt::CompleteDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::CompleteDataChannelName.data())),
       "Name of complete data channel (output)")
      //
      (opt::IncompleteDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::IncompleteDataChannelName.data())),
       "Name of incomplete data channel (output)")
      //
      // --------------------------------------------------
      // DQM channels
      //
      (toDqm(opt::StatusDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::StatusDataChannelName)), "Name of status data channel (DQM)")
      //
      (toDqm(opt::SpillStartDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::SpillStartDataChannelName)),
       "Name of spill-start data channel (output to DQM)")
      //
      (toDqm(opt::SpillEndDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::SpillEndDataChannelName)),
       "Name of spill-end data channel (output to DQM)")
      //
      (toDqm(opt::CompleteDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::CompleteDataChannelName)),
       "Name of monitor channel for build-complete data (output to DQM)")
      //
      (toDqm(opt::IncompleteDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::IncompleteDataChannelName)),
       "Name of monitor channel for build-incomplete data (output to DQM)")
      //
      // --------------------------------------------------
      //
      (opt::BufferTimeoutMS.data(), bpo::value<std::string>()->default_value("10000"),
       "Wait time for event frame in milliseconds (integer)")
      //
      (opt::NumSource.data(), bpo::value<std::string>()->default_value("1"),
       "Number of data sources to be checked (integer)")
      //
      (opt::NumSourceStatus.data(), bpo::value<std::string>()->default_value("0"),
       "Number of status data sources to be checked (integer)")
      //
      (opt::NumSourceSpillStart.data(), bpo::value<std::string>()->default_value("0"),
       "Number of spill-start data sources to be checked (integer)")
      //
      (opt::NumSourceSpillEnd.data(), bpo::value<std::string>()->default_value("0"),
       "Number of spill-end data sources to be checked (integer)")
      //
      (opt::NumSink.data(), bpo::value<std::string>()->default_value("1"),
       "Number of data sink for build-complete data (integer)")
      //
      (opt::RoundRobin.data(),
       bpo::value<std::string>()->default_value(e16::daq::EventBuilder::RoundRobinByEventID.data()),
       "round robin policy (string. event|spill|timestamp|sequence)")
      //
      (opt::NumRetry.data(), bpo::value<std::string>()->default_value("10"), "Number of retry to send data (integer)")
      //
      (opt::PollTimeout.data(), bpo::value<std::string>()->default_value("1"),
       "Timeout value for polling MQSocket (integer in msec)")
      //
      (opt::RunNumber.data(), bpo::value<std::string>(), "Run number (integer)")
      //
      (opt::Sort.data(), bpo::value<std::string>()->default_value("true"), "Sort fragments by source ID")
      //
      (opt::Discard.data(), bpo::value<std::string>()->default_value("true"), "Discard data received in PostRun");
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
   return new e16::daq::EventBuilder;
}

//=============================================================================

namespace e16::daq {

//______________________________________________________________________________
void EventBuilder::Build()
{
   // LOG(debug) << __func__ << ":" << __LINE__ << " " << fEventFrames.size();
   if (fBeginRunData.size() < fNumSource) {
      // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " begin run data: not ready";
      return;
   }

   if (fInBuffer.size() < fNumSource) {
      LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " received data: not ready to calculate tag mask";
      CheckTimeout();
      return;
   }

   // ---------------------------------------------------------------------------
   // 1. calculate tag mask
   // ---------------------------------------------------------------------------
   // fInBuffer : unordered_map<uint64_t, deque<Fragment>>
   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " calculate tag mask";
   if (!fTagMaskReady) {
      for (auto &[id, fifo] : fInBuffer) {
         if (fifo.empty())
            continue;

         // fifo     : deque<Fragment>
         // fragment : struct Fragment {FairMQParts, time_point};
         auto &fragment = fifo.front();
         const auto header = reinterpret_cast<const MQHeader *>(fragment.parts.At(0)->GetData());
         const auto tag0 = reinterpret_cast<const GlobalTag *>(header->tag);
         const auto &tagMask = *(tag0 + 1);

         // std::cout << "1. id = "  << std::hex << id //
         //           << " mask = " << tagMask  //
         //           << " " << fTagMaskMin
         //           << " " << fTagMaskMax
         //           << std::dec << std::endl;

         if (tagMask < fTagMaskMin) {
            // LOG(debug) << "tag mask min update: " << fTagMaskMin << " -> " << tagMask;
            fTagMaskMin = tagMask;
         }

         if (fTagMaskMax < tagMask) {
            // LOG(debug) << "tag mask max update: " << fTagMaskMax << " -> " << tagMask;
            fTagMaskMax = tagMask;
         }
      }
      fTagMaskReady = true;
   }

   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " tag mask calculation done. check data duplication" ;
   // ---------------------------------------------------------------------------
   // 2. check data duplication
   // ---------------------------------------------------------------------------
   for (auto &[id, fifo] : fInBuffer) {
      if (fifo.empty())
         continue;

      auto &fragment = fifo.front();
      const auto header = reinterpret_cast<MQHeader *>(fragment.parts.At(0)->GetData());
      const auto tag0 = reinterpret_cast<const GlobalTag *>(header->tag);
      const auto btag = *tag0 & fTagMaskMin;
      const auto stag = ToString(btag);

      // std::cout << "2. id = " << std::hex << id << ", tag = " << *tag0
      //           << ", masked = " << btag
      //           << ", mask = " << fTagMaskMin
      //           << std::dec << std::endl;

      if (auto fitr = fEventFrames.find(stag); fitr != fEventFrames.end()) {
         // ev : unordered_map<uint64_t, Fragment>
         auto &ev = fitr->second;
         if (ev.fFrame.count(id) > 0) {
            // data might be duplicated
            LOG(error) << " same ID and tag are detected. (id, tag) = " << std::hex << id << " " << btag << std::dec;
            const auto &h = ev.fFrame[id].parts.At(0);
            auto p0 = reinterpret_cast<const char *>(h->GetData());
            auto p1 = p0 + h->GetSize();
            std::for_each(p0, p1, HexDump());
            SendFrame(std::move(ev.fFrame), ChannelID::Incomplete, message_type::EvbIncomplete);
            fitr = fEventFrames.erase(fitr);
         }
      }
   }

   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " data duplication check done. merge data";
   // ---------------------------------------------------------------------------
   // 3. merge data from input FIFO
   // ---------------------------------------------------------------------------
   for (auto &[id, fifo] : fInBuffer) {
      if (fifo.empty())
         continue;

      auto &fragment = fifo.front();
      const auto header = reinterpret_cast<const MQHeader *>(fragment.parts.At(0)->GetData());
      const auto tag0 = reinterpret_cast<const GlobalTag *>(header->tag);
      const auto &tagMask = *(tag0 + 1);
      const auto stag = ToString(*tag0 & fTagMaskMin);
      auto &ev = fEventFrames[stag];

      ev.fMessageType |= header->msg_type;
      ev.fFrame.emplace(id, std::move(fragment));

      // calculate tag data  for header of event builder
      if (ev.fTagMask < tagMask) {
         ev.fTag = *tag0 & tagMask;
         ev.fTagMask = tagMask;
      }

      fifo.pop_front();
   }

   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " merge data done. check completion of event-build and send";
   // ---------------------------------------------------------------------------
   // 4. check completion of event-build
   // ---------------------------------------------------------------------------
   for (auto itr = fEventFrames.begin(); itr != fEventFrames.end();) {
      auto &ev = itr->second;
      if (message_type::HasTrgTyp(ev.fMessageType, message_type::TrgTyp::Normal) && (ev.fFrame.size() == fNumSource)) {
         // LOG(debug) << __func__ << " complete !";
         SendFrame(std::move(ev), ChannelID::Complete, message_type::EvbComplete);
         itr = fEventFrames.erase(itr);
      } else if (message_type::HasTrgTyp(ev.fMessageType, message_type::TrgTyp::Monitor) &&
                 (ev.fFrame.size() == fNumSourceStatus)) {
         LOG(debug) << __func__ << " status";
         SendFrame(std::move(ev), ChannelID::Status,
                   message_type::SetTrgTyp(ev.fMessageType, message_type::TrgTyp::Monitor));
         itr = fEventFrames.erase(itr);
      } else if (message_type::HasTrgTyp(ev.fMessageType, message_type::TrgTyp::SpillStart) &&
                 (ev.fFrame.size() == fNumSourceSpillStart)) {
         LOG(debug) << __func__ << " spill start";
         SendFrame(std::move(ev), ChannelID::SpillStart,
                   message_type::SetTrgTyp(ev.fMessageType, message_type::TrgTyp::SpillStart));
         itr = fEventFrames.erase(itr);
      } else if (message_type::HasTrgTyp(ev.fMessageType, message_type::TrgTyp::SpillEnd) &&
                 (ev.fFrame.size() == fNumSourceSpillEnd)) {
         LOG(debug) << __func__ << " spill end";
         SendFrame(std::move(ev), ChannelID::SpillEnd,
                   message_type::SetTrgTyp(ev.fMessageType, message_type::TrgTyp::SpillEnd));
         itr = fEventFrames.erase(itr);
      } else {
         ++itr;
      }
   }

   // ---------------------------------------------------------------------------
   // 5. check timeout
   // ---------------------------------------------------------------------------
   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " check timeout";
   CheckTimeout();

   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " one cycle ends";
   return;
}

//______________________________________________________________________________
void EventBuilder::CheckTimeout()
{
   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " check timeout " << fEventFrames.size();
   auto now1 = std::chrono::high_resolution_clock::now();
   for (auto itr = fEventFrames.begin(); itr != fEventFrames.end();) {
      auto &ev = itr->second;
      bool isObsolete = false;
      int64_t dt{0};
      for (auto &[id, f] : ev.fFrame) {
         dt = std::chrono::duration_cast<std::chrono::milliseconds>(now1 - f.time).count();
         if (dt > fBufferTimeoutMS) {
            isObsolete = true;
            break;
         }
      }

      if (isObsolete) {
         LOG(debug) << __FUNCTION__ << ":" << __LINE__ << ": has obsolete data " << dt << " " << std::hex
                    << ev.fMessageType << std::dec << " " << ev.fTag << " " << ev.fFrame.size();
         SendFrame(std::move(ev), ChannelID::Incomplete, message_type::EvbIncomplete);
         // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << ": erase iterator " << fEventFrames.size();
         itr = fEventFrames.erase(itr);
      } else {
         ++itr;
      }
   }
}

//______________________________________________________________________________
void EventBuilder::Clear()
{
   fBeginRunData.clear();
   fInBuffer.clear();
   fEventFrames.clear();

   fNumSequence = 0;
   SetGlobalTag(fTagMaskMin);          // set all bits
   SetGlobalTag(fTagMaskMax, 0, 0, 0); // unset all bits
   fTagMaskReady = false;
}

//______________________________________________________________________________
bool EventBuilder::ConditionalRun()
{
   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " receive ";
   if (!fPoller) {
      LOG(warn) << "Poller is not created. " << fClassName
                << " can't receive input data because input data channel is not configured.";
      return false;
   }
   Recv();
   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " receive done";
   Build();
   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " build done";
   return true;
}

//______________________________________________________________________________
FairMQParts EventBuilder::CopyBeginRunData()
{
   LOG(debug) << __func__;
   FairMQParts body;

   if (fSort) {
      using key_type = decltype(fBeginRunData)::key_type;
      using mapped_type = decltype(fBeginRunData)::mapped_type;
      std::map<key_type, mapped_type> sortedData;
      for (auto &[id, m] : fBeginRunData) {
         sortedData.emplace(id, MessageUtil::Copy(*this, m));
      }
      for (auto &[id, m] : sortedData) {
         body.fParts.insert(body.end(), std::make_move_iterator(m.begin()), std::make_move_iterator(m.end()));
      }
   } else {
      for (auto &[id, m] : fBeginRunData) {
         auto cpy = MessageUtil::Copy(*this, m);
         body.fParts.insert(body.end(), std::make_move_iterator(cpy.begin()), std::make_move_iterator(cpy.end()));
      }
   }

   auto header = CreateHeader(MessageUtil::TotalLength(body), message_type::BeginRun);

   FairMQParts ret;
   ret.AddPart(std::move(header));
   for (auto &m : body) {
      ret.AddPart(std::move(m));
   }
   return ret;
}

//______________________________________________________________________________
FairMQMessagePtr EventBuilder::CreateHeader(uint32_t bodySize, uint32_t messageType)
{
   std::size_t n = sizeof(MQHeader);
   auto ret = NewMessage(n);
   {
      auto buf = reinterpret_cast<char *>(ret->GetData());
      std::fill(buf, buf + ret->GetSize(), 0);
   }
   auto header = reinterpret_cast<MQHeader *>(ret->GetData());
   header->magic = magic::EventBuilder;

   *reinterpret_cast<uint64_t *>(header->id) = fHeaderSrcId;
   header->run_id = fRunNumber;
   header->num_sequence = fNumSequence;

   header->header_size = n;
   header->body_size = bodySize;
   header->msg_type = messageType;
   LOG(debug) << __func__ << " message type = " << std::hex << messageType << std::dec;

   uint64_t t =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
         .count();
   auto tp = reinterpret_cast<uint8_t *>(&t);
   std::copy(tp, tp + sizeof(uint64_t), reinterpret_cast<uint8_t *>(header->timestamp));

   return ret;
}

//______________________________________________________________________________
void EventBuilder::Init() {}

//______________________________________________________________________________
void EventBuilder::InitTask()
{
   auto get = [this](auto name) -> std::string {
      if (fConfig->Count(name.data()) < 1) {
         LOG(debug) << " variable: " << name << " not found";
         return "";
      }
      return fConfig->GetProperty<std::string>(name.data());
   };

   auto checkFlag = [this, &get](auto name) {
      std::string s = get(name);
      s = boost::to_lower_copy(s);
      return (s == "1") || (s == "true") || (s == "yes");
   };

   {
      const auto &ip = get(::daq::service::HostIpAddress);
      if (!ip.empty()) {
         fHeaderSrcId = net::ip::make_address(ip).to_v4().to_ulong();
      }
      auto id = get("id"s);
      std::regex r{R"((\d+$))"};
      std::smatch m;
      std::regex_search(id, m, r);
      if (m.size() > 0) {
         auto instanceIndex = std::stoull(m[0].str());
         fHeaderSrcId |= instanceIndex << 32;
      }
      LOG(debug) << " header source id = " << std::hex << fHeaderSrcId << std::dec;
   }

   fSort = checkFlag(opt::Sort);
   fDiscard = checkFlag(opt::Discard);

   fInputDataChannelName = get(opt::InputDataChannelName);
   fChList.clear();
   fChList.resize(static_cast<int>(ChannelID::N));
   fDqmChList.clear();
   fDqmChList.resize(fChList.size());
   SetChannelName(ChannelID::BeginRun, opt::BeginRunDataChannelName);
   SetChannelName(ChannelID::Status, opt::StatusDataChannelName);
   SetChannelName(ChannelID::SpillStart, opt::SpillStartDataChannelName);
   SetChannelName(ChannelID::SpillEnd, opt::SpillEndDataChannelName);
   SetChannelName(ChannelID::Complete, opt::CompleteDataChannelName);
   SetChannelName(ChannelID::Incomplete, opt::IncompleteDataChannelName);

   LOG(debug) << " channels: " << fChannels.size();
   for (auto &[n, v] : fChannels) {
      LOG(debug) << "   " << n << " " << v.size(); // << " " << v[0].Validate();
   }

   fBufferTimeoutMS = std::stoi(get(opt::BufferTimeoutMS));
   auto numSource = std::stoi(get(opt::NumSource));
   LOG(debug) << __func__ << ":" << __LINE__ << " " << ("n-peers:"s + fInputDataChannelName);

   auto nPeersInputDataChannel = get("n-peers:"s + fInputDataChannelName);
   if (nPeersInputDataChannel.empty()) {
      fNumSource = 0;
   } else {
      fNumSource = std::stoi(nPeersInputDataChannel);
   }
   fNumSource = std::max(fNumSource, numSource);
   LOG(debug) << __func__ << ":" << __LINE__ << " " << ("n-peers:"s + fInputDataChannelName)
              << " num-source = " << fNumSource;
   fNumSourceStatus = std::stoi(get(opt::NumSourceStatus));
   LOG(debug) << __func__ << ":" << __LINE__ << " num-source status = " << fNumSourceStatus;
   fNumSourceSpillStart = std::stoi(get(opt::NumSourceSpillStart));
   LOG(debug) << __func__ << ":" << __LINE__ << " num-source spill-start = " << fNumSourceSpillStart;
   fNumSourceSpillEnd = std::stoi(get(opt::NumSourceSpillEnd));
   LOG(debug) << __func__ << ":" << __LINE__ << " num-source spill-end = " << fNumSourceSpillEnd;
   fNumSink = std::stoi(get(opt::NumSink));

   {
      const auto &rr = get(opt::RoundRobin);
      if (rr == RoundRobinByEventID) {
         fRoundRobin = RoundRobinPolicy::RREvent;
      } else if (rr == RoundRobinBySpillID) {
         fRoundRobin = RoundRobinPolicy::RRSpill;
      } else if (rr == RoundRobinByTimestamp) {
         fRoundRobin = RoundRobinPolicy::RRTimestamp;
      } else {
         fRoundRobin = RoundRobinPolicy::RRSequence;
      }
   }

   fPollTimeoutMS = std::stoi(get(opt::PollTimeout));
   fNumRetry = std::stoi(get(opt::NumRetry));

   LOG(debug) << " number of source (FEM) = " << fNumSource << ", number of destinaion (Sink) = " << fNumSink;

   std::vector<std::string> pollChannels;
   pollChannels.emplace_back(fInputDataChannelName);
   std::string ch = fChList[static_cast<int>(ChannelID::BeginRun)];
   if (!ch.empty()) {
      pollChannels.emplace_back(ch);
   }

   if (fChannels.count(fInputDataChannelName) > 0) {
      fPoller = NewPoller(pollChannels);
   } else {
      LOG(warn) << " input data chahnel : " << fInputDataChannelName << " is not configured. Skip creating a poller.";
   }
   Clear();
}

//______________________________________________________________________________
void EventBuilder::PostRun()
{
   // read data to empty the buffer
   for (auto i = 0; i < 10; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      while (Recv()) {
         if (!fDiscard) {
            Build();
         }
      }
   }
   Clear();
}

//______________________________________________________________________________
void EventBuilder::PreRun()
{
   if (fConfig->Count(opt::RunNumber.data()) > 0) {
      fRunNumber = std::stoll(fConfig->GetProperty<std::string>(opt::RunNumber.data()));
      LOG(debug) << " RUN number = " << fRunNumber;
   }
}

//______________________________________________________________________________
void EventBuilder::ReceiveRequest()
{
   LOG(info) << __func__;
   const auto &channelName = fChList[static_cast<int>(ChannelID::BeginRun)];
   if (channelName.empty()) {
      LOG(info) << " begin-run-data channel is not specified.";
      return;
   }

   FairMQParts request;
   if (Receive(request, channelName, 0, 10) > 0) {
      assert(request.Size() == 2);
      auto &m = request[1];
      std::string srcId(reinterpret_cast<char *>(m.GetData()), m.GetSize());
      LOG(warn) << "Received a request for begin-run-data from " << srcId;

      auto reply = CopyBeginRunData();
      FairMQDevice::Send(reply, channelName, 0);
   }
}

//______________________________________________________________________________
bool EventBuilder::Recv()
{
   // LOG(debug) << __FUNCTION__ << ":" << __LINE__;
   if (!fPoller) {
      LOG(warn) << fClassName
                << " can't receive input data because input data channel is not configured. (Poller is not created.)";
      return false;
   }
   fPoller->Poll(fPollTimeoutMS);
   if (fBeginRunData.size() == fNumSource) {
      const auto &channelName = fChList[static_cast<int>(ChannelID::BeginRun)];
      if (!channelName.empty()) {
         if (fPoller->CheckInput(channelName, 0)) {
            ReceiveRequest();
         }
      }
   }

   // LOG(debug) << fInputDataChannelName << ".size() = " << fChannels[fInputDataChannelName].size();
   if (!fPoller->CheckInput(fInputDataChannelName, 0)) {
      return false;
   }

   FairMQParts parts;
   if (Receive(parts, fInputDataChannelName) <= 0) {
      return false;
   }
   auto timeRecv = std::chrono::high_resolution_clock::now();
   // LOG(debug) << __FUNCTION__ << " receive " << parts.Size() << " messages, " << MessageUtil::TotalLength(parts) << "
   // bytes";
   assert(parts.Size() >= 2);

   const auto h0 = reinterpret_cast<MQHeader *>(parts.At(0)->GetData());

   // check magic word
   if ((h0->magic != magic::Sampler) && (h0->magic != magic::EventBuilder)) {
      LOG(error) << "wrong magic word: " << std::hex << h0->magic << std::dec;
      SendFrame(std::move(parts), ChannelID::Incomplete, message_type::EvbIncomplete);
      return true;
   }

   const uint64_t &src_id = *reinterpret_cast<const uint64_t *>(h0->id);
   if ((fInBuffer.size() > fNumSource) || (fInBuffer.size() == fNumSource && fInBuffer.count(src_id) == 0)) {
      LOG(error) << " too many data source: id = " << std::hex << src_id << std::dec;
      SendFrame(std::move(parts), ChannelID::Incomplete, message_type::EvbIncomplete);
      return true;
   }

   // LOG(debug) << __func__ << " type = " << std::hex << h0->msg_type << " run = " << h0->run_id
   //           << " src = " << *reinterpret_cast<uint64_t*>(h0->id)
   //           << " num sequence = " << h0->num_sequence
   //           << std::dec;
   if ((h0->msg_type & message_type::BeginRun) > 0) {
      SendBeginRunData(src_id, parts);
      return true;
   }

   // LOG(debug) << __func__ << ":" << __LINE__ << " add msgparts to in-buffer";
   fInBuffer[src_id].emplace_back(std::move(parts), std::move(timeRecv));
   return true;
}

//______________________________________________________________________________
bool EventBuilder::Send(FairMQParts &parts, ChannelID id, int direction)
{
   // LOG(debug) << __func__ << ":" << __LINE__;
   auto index = static_cast<int>(id);
   assert(index < fDqmChList.size());
   const auto &dqmChannelName = fDqmChList[index];
   if (!dqmChannelName.empty()) {
      LOG(debug) << __func__ << ":" << __LINE__ << " send to DQM " << dqmChannelName;
      auto cpy = MessageUtil::Copy(*this, parts);
      FairMQDevice::Send(cpy, dqmChannelName, 0, 0);
   }

   const auto &channelName = fChList[index];
   if (!channelName.empty()) {
      int nretry = 0;
      while (FairMQDevice::Send(parts, channelName, direction, 0) < 0) {
         ++nretry;
         if (fNumRetry < 0) {
            continue;
         } else if (nretry >= fNumRetry) {
            LOG(warn) << " failed to send " << channelName << " " << direction;
            return false;
         }
         if (NewStatePending()) {
            return false;
         }
         std::this_thread::sleep_for(1ms);
      }
   }
   return true;
}

//______________________________________________________________________________
void EventBuilder::SendBeginRunData(uint64_t srcId, FairMQParts &parts)
{
   // LOG(debug) << __func__ << ":" << fBeginRunData.size();
   if (fBeginRunData.size() == fNumSource) {
      LOG(error) << __func__ << " too many begin-run-data";
      return;
   }
   fBeginRunData.emplace(srcId, MessageUtil::Copy(*this, parts));
   if (fBeginRunData.size() == fNumSource) {
      LOG(info) << __func__ << " send begin-run-data";
      for (auto id = static_cast<int>(ChannelID::Status); id < static_cast<int>(ChannelID::N); ++id) {
         LOG(debug) << __func__ << " channel id = " << id << " " << fChList.size();
         auto channelName = fChList[id];
         LOG(debug) << __func__ << " channel name = " << channelName;
         if (channelName.empty()) {
            continue;
         }
         auto nSubChannels = fChannels[channelName].size();
         LOG(debug) << __func__ << " channel: " << channelName << " n subchannels = " << nSubChannels;
         for (auto direction = 0; direction < nSubChannels; ++direction) {
            auto cpy = CopyBeginRunData();
            Send(cpy, static_cast<ChannelID>(id), direction);
         }
      }
   }
   // LOG(debug) << __func__ << " done";
}

//______________________________________________________________________________
bool EventBuilder::SendFrame(FairMQParts &&parts, ChannelID id, uint32_t msg_type)
{
   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " " << parts.Size() << " " << static_cast<int>(id) << " " <<
   // msg_type;

   const auto header = reinterpret_cast<const MQHeader *>(parts[0].GetData());

   auto buf = CreateHeader(MessageUtil::TotalLength(parts), msg_type | header->msg_type);
   // insert header message to top of the container
   FairMQParts p;
   p.AddPart(std::move(buf));
   p.AddPart(std::move(parts));

   return Send(p, id);
}

//______________________________________________________________________________
bool EventBuilder::SendFrame(EventFrame &&ev, ChannelID id, uint32_t msg_type)
{
   // LOG(debug) << __FUNCTION__ << ":" << __LINE__ << " " << static_cast<int>(id) << " " << msg_type;
   FairMQParts body;
   if (fSort) {
      using key_type = decltype(EventFrame::fFrame)::key_type;
      using mapped_type = decltype(EventFrame::fFrame)::mapped_type;
      std::map<key_type, mapped_type> sortedFrame(std::make_move_iterator(ev.fFrame.begin()),
                                                  std::make_move_iterator(ev.fFrame.end()));
      for (auto &[key, f] : sortedFrame) {
         body.AddPart(std::move(f.parts));
      }

   } else {
      for (auto &[key, f] : ev.fFrame) {
         body.AddPart(std::move(f.parts));
      }
   }

   auto body0 = reinterpret_cast<const MQHeader *>(body[0].GetData());

   auto headerBuf = CreateHeader(MessageUtil::TotalLength(body), msg_type | body0->msg_type);
   auto header = reinterpret_cast<MQHeader *>(headerBuf->GetData());
   {
      auto tag = reinterpret_cast<GlobalTag *>(header->tag);
      auto tagMask = tag + 1;

      *tag = ev.fTag;
      *tagMask = ev.fTagMask;
   }

   // LOG(debug) << " message total size = " << header->body_size;

   // insert header message to top of the container
   FairMQParts p;
   p.AddPart(std::move(headerBuf));
   p.AddPart(std::move(body));

   auto direction = 0;
   if (fRoundRobin == RoundRobinPolicy::RREvent) {
      direction = GetEvent(ev.fTag);
   } else if (fRoundRobin == RoundRobinPolicy::RRSpill) {
      direction = GetSpill(ev.fTag);
   } else if (fRoundRobin == RoundRobinPolicy::RRTimestamp) {
      direction = GetTime(ev.fTag);
   } else if (fRoundRobin == RoundRobinPolicy::RRSequence) {
      direction = fNumSequence;
   }
   direction %= fNumSink;
   // LOG(debug)  << " size = " << p.Size() << ", direction = " << direction;
   auto ret = Send(p, id, direction);
   ++fNumSequence;
   return ret;
}

//______________________________________________________________________________
void EventBuilder::SetChannelName(ChannelID id, std::string_view key)
{
   auto index = static_cast<int>(id);
   assert(index < fChList.size());
   if (fConfig->Count(key.data()) > 0) {
      const auto &channelName = fConfig->GetProperty<std::string>(key.data());
      LOG(debug) << __func__ << ":" << __LINE__ << " " << channelName;
      if (fChannels.count(channelName) > 0) {
         fChList[index] = channelName;
      }
   }

   std::string dqmKey{key.data()};
   dqmKey += "-dqm";
   if (fConfig->Count(dqmKey) > 0) {
      const auto &channelName = fConfig->GetProperty<std::string>(dqmKey);
      LOG(debug) << __func__ << ":" << __LINE__ << " " << channelName;
      if (fChannels.count(channelName) > 0) {
         fDqmChList[index] = channelName;
      }
   }
}

} // namespace e16::daq
