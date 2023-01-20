#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>

#include <boost/algorithm/string/case_conv.hpp>

#include <FairMQPoller.h>
#include <runFairMQDevice.h>

#include "frontend/Daq.h"
#include "frontend/DaqFileSampler.h"
#include "utility/scope_guard.h"
#include "mq/MessageUtil.h"

#include "mq/Sampler/Sampler.h"

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace bpo = boost::program_options;
using namespace highp::e50;
using opt = e16::daq::Sampler::OptionKey;

//=============================================================================
// defined in runFairMQDevice.h
void addCustomOptions(bpo::options_description &options)
{
   auto toDqm = [](auto a) -> std::string { return (std::string(a.data()) + "-dqm").data(); };

   options.add_options()
      //
      (opt::BeginRunDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::BeginRunDataChannelName.data())),
       "Name of output channel for begin run data")
      //
      (opt::DataChannelName.data(), bpo::value<std::string>()->default_value(std::string(opt::DataChannelName.data())),
       "Name of output channel for data")
      //
      (opt::NoTagDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::NoTagDataChannelName.data())),
       "Name of output channel for data without tag")
      //
      (opt::EndRunDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::EndRunDataChannelName.data())),
       "Name of output channel for end run data")
      //
      (opt::PedestalDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::PedestalDataChannelName.data())),
       "Name of output channel for pedestal data")
      //
      (opt::StatusDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::StatusDataChannelName.data())),
       "Name of output channel for status data")
      //
      (opt::SpillStartDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::SpillStartDataChannelName.data())),
       "Name of output channel for spill start data")
      //
      (opt::SpillEndDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::SpillEndDataChannelName.data())),
       "Name of output channel for spill end data")
      //
      (opt::SummaryDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::SummaryDataChannelName.data())),
       "Name of output channel for summary data")
      //
      (opt::UnknownDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::UnknownDataChannelName.data())),
       "Name of output channel for unknown data")
      //
      // --------------------------------------------------
      // channel names for data quality monitor
      //
      //(toDqm(opt::BeginRunDataChannelName).data(),
      // bpo::value<std::string>()->default_value(toDqm(opt::BeginRunDataChannelName)),
      // "Name of output channel for begin run data (DQM)")
      //
      (toDqm(opt::DataChannelName).data(), bpo::value<std::string>()->default_value(toDqm(opt::DataChannelName)),
       "Name of output channel for data (DQM)")
      //
      (toDqm(opt::NoTagDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::NoTagDataChannelName)),
       "Name of output channel for data without tag (DQM)")
      //
      (toDqm(opt::EndRunDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::EndRunDataChannelName)),
       "Name of output channel for end run data (DQM)")
      //
      (toDqm(opt::PedestalDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::PedestalDataChannelName)),
       "Name of output channel for pedestal data (DQM)")
      //
      (toDqm(opt::StatusDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::StatusDataChannelName)),
       "Name of output channel for status data (DQM)")
      //
      (toDqm(opt::SpillStartDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::SpillStartDataChannelName)),
       "Name of output channel for spill start data (DQM)")
      //
      (toDqm(opt::SpillEndDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::SpillEndDataChannelName)),
       "Name of output channel for spill end data (DQM)")
      //
      (toDqm(opt::SummaryDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::SummaryDataChannelName)),
       "Name of output channel for summary data (DQM)")
      //
      (toDqm(opt::UnknownDataChannelName).data(),
       bpo::value<std::string>()->default_value(toDqm(opt::UnknownDataChannelName)),
       "Name of output channel for unknown data (DQM)")
      //
      (opt::MaxIteration.data(), bpo::value<std::string>()->default_value("-1"), "Number of iterations")
      //
      (opt::PollTimeout.data(), bpo::value<std::string>()->default_value("100"),
       "Timeout (in msec) of polling of requests")
      //
      (opt::NoHeader.data(), "Data without MQ header and condition data")
      //
      (opt::RunNumber.data(), bpo::value<std::string>(), "Run number")
      //
      (opt::EventWait.data(), bpo::value<std::string>()->default_value("-1"),
       "wait time before sending event fragment for DAQ test (in milliseconds. If negative, no wait)")
      //
      (opt::WaitStopCommand.data(), bpo::value<std::string>()->default_value("true"),
       "Wait stop command if DAQ main loop ends.")
      //
      (opt::RoundRobin.data(), bpo::value<std::string>()->default_value(e16::daq::Sampler::RoundRobinByEventID.data()),
       "round robin policy (string. event|spill|timestamp|sequence)");

   e16::daq::Sampler::AddOptions(options);
}

//______________________________________________________________________________
// defined in runFairMQDevice.h
FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
   return new e16::daq::Sampler;
}

//=============================================================================
namespace e16::daq {

//______________________________________________________________________________
void Sampler::CreateBeginRunData()
{
   fBeginRunData.fParts.clear();
   // LOG(debug) << fDaq->GetName() << ":" << __func__;
   auto header_buf = CreateHeader();
   auto header = reinterpret_cast<MQHeader *>(header_buf->GetData());
   header->num_sequence = fDaq->GetNumBeginRun();
   header->msg_type = message_type::BeginRun;

   auto buffer = MessageUtil::NewMessage(*this, std::make_unique<std::vector<char>>(std::move(fDaq->GetCondition())));
   header->body_size = buffer->GetSize();

   fBeginRunData.AddPart(std::move(header_buf));
   fBeginRunData.AddPart(std::move(buffer));
}

//______________________________________________________________________________
FairMQMessagePtr Sampler::CreateHeader()
{
   // LOG(debug) << "CreateHeader() " << daq.GetName() << " " <<  std::hex << fFemType << std::dec << " " << n;
   const std::size_t n = sizeof(MQHeader);
   auto ret = NewMessage(n);
   {
      auto buf = reinterpret_cast<char *>(ret->GetData());
      std::fill(buf, buf + ret->GetSize(), 0);
   }
   auto header = reinterpret_cast<MQHeader *>(ret->GetData());
   header->magic = magic::Sampler;
   *reinterpret_cast<uint64_t *>(header->id) = fDaq->GetID();
   header->src_type = fFemType;
   header->run_id = fRunNumber;
   header->header_size = n;
   // LOG(debug) << "CreateHeader() " << fDaq->GetName() << " " << std::hex << fDaq->GetID() << " " << header->run_id <<
   // std::dec << " " << header->header_size;

   uint64_t t =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
         .count();
   auto tp = reinterpret_cast<uint8_t *>(&t);
   std::copy(tp, tp + sizeof(uint64_t), reinterpret_cast<uint8_t *>(header->timestamp));

   // LOG(debug) << "CreateHeader() " << daq.GetName() << " done";
   return ret;
}

//______________________________________________________________________________
uint64_t Sampler::GetDirection(ChannelID channelId)
{
   auto index = static_cast<int>(channelId);
   auto n = fNDestination[index];
   if (n == 0) {
      return 0;
   }

   switch (fRoundRobin) {
   case RoundRobinPolicy::RREvent: return fEventID % n;
   case RoundRobinPolicy::RRSpill: return fSpillID % n;
   case RoundRobinPolicy::RRTimestamp: return fTimestamp % n;
   case RoundRobinPolicy::RRSequence: return fNumIteration % n;
   default: return 0;
   }
   return 0;
}

//______________________________________________________________________________
void Sampler::HandleRequest()
{
   LOG(info) << __func__ << " start";
   const auto &channelName = fChList[static_cast<int>(ChannelID::BeginRun)];
   if (channelName.empty()) {
      LOG(info) << " begin-run-data channel is not specified.";
      return;
   }
   auto poller = NewPoller(channelName);
   auto n = fChannels.count(channelName);
   LOG(info) << channelName << ": number of sub sockets = " << n;
   while (!NewStatePending() && !fDaqThreadEnd) {
      poller->Poll(fPollTimeoutMS);
      // LOG(debug) << channelName << ": waiting";
      for (auto i = 0; i < n; ++i) {
         if (poller->CheckInput(channelName, i)) {
            LOG(debug) << channelName << ": has an input message";
            FairMQParts request;
            if (Receive(request, channelName, i, 10) > 0) {
               assert(request.Size() == 2);
               auto &m = request[1];
               std::string srcId(reinterpret_cast<char *>(m.GetData()), m.GetSize());
               LOG(warn) << "Received a request for begin-run-data from " << srcId;

               auto reply = MessageUtil::Copy(*this, fBeginRunData);
               FairMQDevice::Send(reply, channelName, i);
            }
            // else {
            //   LOG(warn) << " request timeout";
            //}
         }
      }
   }
   LOG(info) << __func__ << " end";
}

//______________________________________________________________________________
void Sampler::Init()
{
   // called during device initialization
}

//______________________________________________________________________________
void Sampler::InitTask()
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

   try {

      fMaxIteration = std::stoll(get(opt::MaxIteration));
      fPollTimeoutMS = std::stoi(get(opt::PollTimeout));
      fEventWaitMS = std::stoi(get(opt::EventWait));
      fWaitStopCommand = checkFlag(opt::WaitStopCommand);

      fNoHeader = (fConfig->Count(opt::NoHeader.data()) > 0);
      fUseFileSampler = (fConfig->Count(e16::daq::DaqFileSampler::OptionKey::InputDataFileName.data()) > 0);

      fChList.clear();
      fChList.resize(static_cast<int>(ChannelID::N));
      fDqmChList.clear();
      fDqmChList.resize(fChList.size());

      SetChannelName(ChannelID::BeginRun, opt::BeginRunDataChannelName);
      SetChannelName(ChannelID::Data, opt::DataChannelName);
      SetChannelName(ChannelID::NoTag, opt::NoTagDataChannelName);
      SetChannelName(ChannelID::EndRun, opt::EndRunDataChannelName);
      SetChannelName(ChannelID::Pedestal, opt::PedestalDataChannelName);
      SetChannelName(ChannelID::Status, opt::StatusDataChannelName);
      SetChannelName(ChannelID::SpillStart, opt::SpillStartDataChannelName);
      SetChannelName(ChannelID::SpillEnd, opt::SpillEndDataChannelName);
      SetChannelName(ChannelID::Summary, opt::SummaryDataChannelName);
      SetChannelName(ChannelID::Unknown, opt::UnknownDataChannelName);
      {
         std::ostringstream oss;
         oss << "found channels: ";
         for (const auto &s : fChList) {
            if (!s.empty()) {
               oss << " " << s;
            }
         }
         LOG(debug) << oss.str();
      }

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

      fNDestination.resize(fChList.size());
      int i = -1;
      for (const auto &channelName : fChList) {
         ++i;
         if (channelName.empty()) {
            continue;
         }
         auto nPeers = get("n-peers:"s + channelName);
         if (!nPeers.empty()) {
            fNDestination[i] = std::stoi(nPeers);
         }
      }

      //-----------------------
      // initialize DAQ objects
      //-----------------------
      CreateDaq();

      if (!fNoHeader) {
         fDaq->SetFuncProcessBeginRunData([this]() { ProcessBeginRunData(); });
         fDaq->SetFuncProcessEndRunData([this]() { ProcessEndRunData(); });
      }

      fDaq->SetFuncProcessData([this]() {
         ProcessData();
         ++fNumIteration;
      });

      fDaq->SetFuncProcessIncompleteData([this]() { ProcessIncompleteData(); });

   } catch (const bpo::error_with_option_name &e) {
      LOG(error) << __func__ << " e.what() = " << e.what();
   } catch (const std::exception &e) {
      LOG(error) << __func__ << " e.what() = " << e.what();
   } catch (...) {
      LOG(error) << __func__ << " unknown exception";
   }
}

//______________________________________________________________________________
void Sampler::PostRun()
{
   LOG(debug) << fClassName << ":" << __func__;
   fDaq->Close();
   fDaq->Reset();
}

//______________________________________________________________________________
void Sampler::PreRun()
{
   LOG(debug) << fClassName << ":" << __func__;
   if (fConfig->Count(opt::RunNumber.data()) > 0) {
      fRunNumber = std::stoll(fConfig->GetProperty<std::string>(opt::RunNumber.data()));
      LOG(debug) << " RUN ID = " << fRunNumber;
      fDaq->SetRunNumber(fRunNumber);
   }
   fTimestamp = 0;
   fEventID = 0;
   fSpillID = 0;
   fNumIteration = 0;
   fDaqThreadEnd = false;
   fDaq->SetFuncCancelRequested([this]() {
      auto ret = NewStatePending();
      // LOG(debug) << " check cancel requested: " << ret;
      if ((fMaxIteration > 0) && (fNumIteration >= fMaxIteration)) {
         ret = true;
      }
      return ret;
   });
   CreateBeginRunData();
   LOG(debug) << fClassName << "::PreRun set cancel handler done";
}

//______________________________________________________________________________
void Sampler::ProcessBeginRunData()
{
   LOG(debug) << fDaq->GetName() << ":" << __func__;
   Send(fBeginRunData, ChannelID::Data, true, true);
   LOG(debug) << fDaq->GetName() << ":" << __func__ << " sent to Data channel";
   Send(fBeginRunData, ChannelID::NoTag, true, true);
   LOG(debug) << fDaq->GetName() << ":" << __func__ << " sent to no tag Data channel";
   Send(fBeginRunData, ChannelID::Pedestal, true, true);
   LOG(debug) << fDaq->GetName() << ":" << __func__ << " sent to pedestal Data channel";
   Send(fBeginRunData, ChannelID::Status, true, true);
   LOG(debug) << fDaq->GetName() << ":" << __func__ << " sent to status Data channel";
   Send(fBeginRunData, ChannelID::SpillStart, true, true);
   LOG(debug) << fDaq->GetName() << ":" << __func__ << " sent to spill start Data channel";
   Send(fBeginRunData, ChannelID::SpillEnd, true, true);
   LOG(debug) << fDaq->GetName() << ":" << __func__ << " sent to spill end Data channel";
   Send(fBeginRunData, ChannelID::Summary, true, true);
   LOG(debug) << fDaq->GetName() << ":" << __func__ << " sent to summary Data channel";
   Send(fBeginRunData, ChannelID::Unknown, true, true);
   LOG(debug) << fDaq->GetName() << ":" << __func__ << " sent to unknown Data channel";
}

//______________________________________________________________________________
void Sampler::ProcessEndRunData()
{
   LOG(debug) << fDaq->GetName() << ":" << __func__;
   auto header_buf = CreateHeader();
   auto header = reinterpret_cast<MQHeader *>(header_buf->GetData());
   header->num_sequence = fDaq->GetNumEndRun();
   header->msg_type = message_type::EndRun;

   auto buffer = MessageUtil::NewMessage(*this, std::make_unique<std::vector<char>>(std::move(fDaq->GetCondition())));
   header->body_size = buffer->GetSize();

   FairMQParts parts;
   parts.AddPart(std::move(header_buf));
   parts.AddPart(std::move(buffer));

   Send(parts, ChannelID::Data, true, true);
   Send(parts, ChannelID::NoTag, true, true);
   Send(parts, ChannelID::Pedestal, true, true);
   Send(parts, ChannelID::Status, true, true);
   Send(parts, ChannelID::SpillStart, true, true);
   Send(parts, ChannelID::SpillEnd, true, true);
   Send(parts, ChannelID::Summary, true, true);
   Send(parts, ChannelID::Unknown, true, true);
   LOG(debug) << fDaq->GetName() << ": ProcessEndRunData() done";
}

//______________________________________________________________________________
void Sampler::Run()
{
   //--------------------------------------------------------------
   // main event loop by nesting asynchronous callback functions
   //--------------------------------------------------------------
   try {
      std::thread beginRunDataThread([this] { HandleRequest(); });

      LOG(info) << fClassName << ":" << __func__ << " main loop start";
      fDaq->Start();
      LOG(info) << fClassName << ":" << __func__ << " main loop end";
      fDaqThreadEnd = true;
      beginRunDataThread.join();

      bool waiting = false;
      while (fWaitStopCommand && !NewStatePending()) {
         if (!waiting) {
            LOG(debug) << " waiting for stop command";
            waiting = true;
         }
         std::this_thread::yield();
         std::this_thread::sleep_for(100ms);
      }

   } catch (const std::exception &e) {
      LOG(info) << fClassName << " Exception in main loop: " << e.what();
   } catch (...) {
      LOG(info) << fClassName << " Exception in main loop: unknown";
   }
   LOG(info) << fClassName << ":" << __func__ << " main loop finished";
}

//______________________________________________________________________________
void Sampler::ResetTask()
{
   LOG(debug) << fClassName << ":" << __func__;
   // unique_ptr::reset()
   fDaq.reset();
}

//______________________________________________________________________________
int64_t Sampler::Send(FairMQParts &parts, ChannelID id, bool copyMessage, bool sameMessage)
{
   // LOG(debug) << fClassName << ":" << __func__ << ":" << __LINE__;// << " " << id << " " << copyMessage;
   int64_t ret{0};
   auto index = static_cast<int>(id);
   const auto &dqmChannelName = fDqmChList[index];
   // LOG(debug) << __func__ << " dqm channel name = " << dqmChannelName;
   if (!dqmChannelName.empty()) {
      auto p = MessageUtil::Copy(*this, parts);
      ret += FairMQDevice::Send(p, dqmChannelName);
   }

   const auto &channelName = fChList[index];
   // LOG(debug) << __func__ << " channel name = " << channelName;
   if (!channelName.empty()) {
      if (sameMessage) {
         auto nSubChannels = fChannels.at(channelName).size();
         for (auto i = 0; i < nSubChannels; ++i) {
            auto p = MessageUtil::Copy(*this, parts);
            ret += FairMQDevice::Send(p, channelName, i);
         }
      } else {
         auto direction = GetDirection(id);
         if (copyMessage) {
            auto p = MessageUtil::Copy(*this, parts);
            ret += FairMQDevice::Send(p, channelName, direction);
         } else {
            ret += FairMQDevice::Send(parts, channelName, direction);
         }
      }
   }

   // LOG(debug) << fClassName << ":" << __func__ << ":" << __LINE__ << " send  done";
   return ret;
}

//______________________________________________________________________________
void Sampler::SetChannelName(ChannelID id, std::string_view key)
{
   // LOG(debug) << __func__ << ":" << __LINE__ << " id = " << static_cast<int>(id) << " key = " << key;
   auto index = static_cast<int>(id);
   assert(index < fChList.size());
   if (fConfig->Count(key.data()) > 0) {
      const auto &channelName = fConfig->GetProperty<std::string>(key.data());
      // LOG(debug) << __func__ << ":" << __LINE__ << " channel name = " << channelName;
      if (fChannels.count(channelName) > 0) {
         // LOG(debug) << __func__ << ":" << __LINE__ << " channel name = " << channelName;
         fChList[index] = channelName;
      }
   }

   std::string dqmKey{key.data()};
   dqmKey += "-dqm";
   if (fConfig->Count(dqmKey) > 0) {
      const auto &channelName = fConfig->GetProperty<std::string>(dqmKey);
      // LOG(debug) << __func__ << ":" << __LINE__ << " channel name = " << channelName;
      if (fChannels.count(channelName) > 0) {
         // LOG(debug) << __func__ << ":" << __LINE__ << " channel name = " << channelName;
         fDqmChList[index] = channelName;
      }
   }
}

} // namespace e16::daq
