#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <boost/algorithm/string/case_conv.hpp>

#include <runFairMQDevice.h>
#include <FairMQParts.h>

#include "mq/MessageUtil.h"
#include "unpacker/MQHeader.h"
#include "utility/string_to_container.h"
#include "mq/Monitor/Dqm.h"

namespace bpo = boost::program_options;
using namespace highp::e50;
using opt = e16::daq::Dqm::OptionKey;

// static constexpr std::string_view MyClass{"e16::daq::Dqm"};

//_____________________________________________________________________________
void addCustomOptions(bpo::options_description &options)
{
   options.add_options()
      //
      (opt::BeginRunDataChannelName.data(),
       bpo::value<std::string>()->default_value(opt::BeginRunDataChannelName.data()), "Name for begin-run-data channel")
      //
      (opt::InputDataChannelName.data(), bpo::value<std::string>()->default_value(opt::InputDataChannelName.data()),
       "name for data input channel")
      //
      (opt::PollTimeout.data(), bpo::value<std::string>()->default_value("100"), "Poll timeout in milliseconds");

   e16::daq::Dqm::AddOptions(options);
}

//_____________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
   return new e16::daq::Dqm;
}

//=============================================================================

namespace e16::daq {

//_____________________________________________________________________________
void Dqm::InitChannelName()
{
   LOG(info) << fId << ":" << __func__;
   if (fConfig->Count(opt::BeginRunDataChannelName.data()) > 0) {
      auto channelName = fConfig->GetProperty<std::string>(opt::BeginRunDataChannelName.data());
      if (fChannels.count(channelName) > 0) {
         fBeginRunDataChannelName = channelName;
      }
   }
   if (fConfig->Count(opt::InputDataChannelName.data()) > 0) {
      auto channelName = fConfig->GetProperty<std::string>(opt::InputDataChannelName.data());
      if (fChannels.count(channelName) > 0) {
         fInputDataChannelName = channelName;
      }
   }
}

//_____________________________________________________________________________
void Dqm::InitTask()
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

   LOG(info) << fId << ":" << __func__;
   InitChannelName();
   fPoller = NewPoller(fInputDataChannelName);
   fPollTimeoutMS = std::stoi(get(opt::PollTimeout));
   InitDqm();
}

//_____________________________________________________________________________
void Dqm::PostRun()
{
   LOG(info) << fId << ":" << __func__;
}

//_____________________________________________________________________________
void Dqm::PreRun()
{
   LOG(info) << fId << ":" << __func__;
   // RequestBeginRunData();
   SetRunNumber();
}

//_____________________________________________________________________________
void Dqm::Recv()
{
   // polling data input
   if (!fPoller) {
      LOG(error) << " no poller";
      return;
   }
   fPoller->Poll(fPollTimeoutMS);
   if (!fPoller->CheckInput(fInputDataChannelName, 0)) {
      return;
   }

   // data receive
   FairMQParts parts;
   if (Receive(parts, fInputDataChannelName) <= 0) {
      return;
   }

   for (const auto &m : parts) {
      ProcessData(reinterpret_cast<char *>(m->GetData()), m->GetSize());
   }
}

//_____________________________________________________________________________
void Dqm::RequestBeginRunData()
{
   /*
   LOG(info) << fId << ":" << __func__;
   auto buf = std::make_unique<std::vector<char>>(sizeof(MQHeader));
   auto header = reinterpret_cast<MQHeader *>(buf->data());
   header->magic = magic::Monitor;
   header->header_size = sizeof(MQHeader);
   header->msg_type = message_type::BeginRun | message_type::Request;

   FairMQParts request;
   request.AddPart(MessageUtil::NewMessage(*this, std::move(buf)));
   request.AddPart(NewSimpleMessage(fId));

   LOG(info) << __func__ << ":"
             << " Send a request for begin-run-data via " << fBeginRunDataChannelName << " channel";
   auto n = fChannels.count(fBeginRunDataChannelName);
   for (auto i = 0; i < n; ++i) {
      if (Send(request, fBeginRunDataChannelName, i) > 0) {
         FairMQParts reply;
         // LOG(info) << __func__ << ":" << " Receive";
         if (Receive(reply, fBeginRunDataChannelName, i) >= 0) {
            std::vector<std::vector<char>> v;
            for (auto &&x : reply) {
               auto first = reinterpret_cast<char *>(x->GetData());
               auto last = first + x->GetSize();
               v.emplace_back(first, last);
            }

            auto replyHeader = reinterpret_cast<MQHeader *>(v[0].data());
            const uint64_t srcId = *reinterpret_cast<const uint64_t *>(replyHeader->id);
            LOG(warn) << __func__ << ":"
                      << " Received reply from id = " << std::hex << srcId << std::dec;
            fBeginRunData[srcId] = std::move(v);
         }
      }
   }
   */
}
} // namespace e16::daq