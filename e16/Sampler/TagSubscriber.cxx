#include <chrono>
#include <iostream>
#include <random>
#include <regex>

#include <boost/asio.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <runFairMQDevice.h>

#include "unpacker/GlobalTag.h"
#include "unpacker/MQHeader.h"
#include "mq/DaqServiceConstants.h"
#include "mq/MessageUtil.h"
#include "mq/Sampler/TagSubscriber.h"

namespace net = boost::asio;
namespace bpo = boost::program_options;
using namespace highp::e50;
using opt = e16::daq::TagSubscriber::OptionKey;

std::random_device rnd_device;
std::mt19937_64 mt64_gen(rnd_device()); // 64-bit Mersenne twister engine
using dist_type = std::normal_distribution<>;
dist_type dist;

//_____________________________________________________________________________
void addCustomOptions(bpo::options_description &options)
{
   options.add_options()
      //
      (opt::TagInputChannelName.data(), bpo::value<std::string>()->default_value(opt::TagInputChannelName.data()),
       "Tag data input channel name")
      //
      (opt::BeginRunDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::BeginRunDataChannelName.data())),
       "Name of output channel for begin run data")
      //
      (opt::DataChannelName.data(), bpo::value<std::string>()->default_value(opt::DataChannelName.data()),
       "Name of data output channel")
      //
      (opt::LengthMean.data(), bpo::value<std::string>()->default_value("4000"), "Data length (double, mean)")
      //
      (opt::LengthStddev.data(), bpo::value<std::string>()->default_value("20"), "Data length (double, stddev)")
      //
      (opt::Discard.data(), bpo::value<std::string>()->default_value("true"), "Discard data received in PostRun")
      //
      (opt::Timeout.data(), bpo::value<std::string>()->default_value("100"), "Timeout of received messages (msec)")
      //
      (opt::PollTimeout.data(), bpo::value<std::string>()->default_value("100"),
       "Timeout (in msec) of polling of requests")
      //
      (opt::RunNumber.data(), bpo::value<std::string>(), "Run number");
}

//_____________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
   return new e16::daq::TagSubscriber;
}

namespace e16::daq {

//______________________________________________________________________________
void TagSubscriber::CreateBeginRunData()
{
   fBeginRunData.fParts.clear();
   LOG(debug) << fClassName << ":" << __func__;
   auto header_buf = CreateHeader();
   auto header = reinterpret_cast<MQHeader *>(header_buf->GetData());
   header->num_sequence = fNBeginRun;
   header->msg_type = message_type::BeginRun;

   auto buffer = NewSimpleMessage(fId);
   header->body_size = buffer->GetSize();

   fBeginRunData.AddPart(std::move(header_buf));
   fBeginRunData.AddPart(std::move(buffer));

   ++fNBeginRun;
}

//_____________________________________________________________________________
FairMQMessagePtr TagSubscriber::CreateHeader()
{
   const auto n = sizeof(MQHeader);
   auto ret = NewMessage(n);
   {
      auto buf = reinterpret_cast<char *>(ret->GetData());
      std::fill(buf, buf + ret->GetSize(), 0);
   }
   auto header = reinterpret_cast<MQHeader *>(ret->GetData());
   header->magic = magic::Sampler;
   *reinterpret_cast<uint64_t *>(header->id) = fHeaderSrcId;
   header->run_id = fRunNumber;
   header->header_size = n;
   uint64_t t =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
         .count();
   auto tp = reinterpret_cast<uint8_t *>(&t);
   std::copy(tp, tp + sizeof(uint64_t), reinterpret_cast<uint8_t *>(header->timestamp));
   return ret;
}

//______________________________________________________________________________
void TagSubscriber::HandleRequest()
{
   LOG(info) << __func__ << " start";
   const auto &channelName = fBeginRunDataChannelName;
   if (channelName.empty()) {
      LOG(info) << " begin-run-data channel is not specified.";
      return;
   }
   auto poller = NewPoller(channelName);
   auto n = fChannels.count(channelName);
   LOG(info) << channelName << ": number of sub sockets = " << n;
   while (!NewStatePending()) {
      poller->Poll(fPollTimeoutMS);
      // LOG(debug) << channelName << ": waiting";
      for (auto i = 0; i < n; ++i) {
         if (poller->CheckInput(channelName, i)) {
            // LOG(debug) << channelName << ": has an input message";
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
//_____________________________________________________________________________
bool TagSubscriber::HandleTag(FairMQMessagePtr &m, int index)
{
   auto buf = CreateHeader();
   auto header = reinterpret_cast<MQHeader *>(buf->GetData());
   header->num_sequence = fNSequence;
   header->msg_type = message_type::SetTrgTyp(header->msg_type, message_type::TrgTyp::Normal);
   header->body_size = std::nearbyint(dist(mt64_gen));

   auto tag = reinterpret_cast<GlobalTag *>(header->tag);
   auto tagMask = tag + 1;
   auto src = reinterpret_cast<GlobalTag *>(m->GetData());
   *tag = *src;
   SetGlobalTag(*tagMask);
   auto body = NewMessage(header->body_size);

   LOG(debug) << __func__ << " received tag : " << *tag;

   FairMQParts parts;
   parts.AddPart(std::move(buf));
   parts.AddPart(std::move(body));

   if (fChannels.count(fDataChannelName) == 0) {
      return true;
   }
   Send(parts, fDataChannelName);

   ++fNSequence;
   return true;
}

//_____________________________________________________________________________
void TagSubscriber::InitTask()
{
   auto get = [this](auto name) -> std::string {
      if (fConfig->Count(name.data()) < 1) {
         std::cout << " variable: " << name << " not found" << std::endl;
         return "";
      }
      return fConfig->GetProperty<std::string>(name.data());
   };

   auto checkFlag = [this, &get](auto name) {
      std::string s = get(name);
      s = boost::to_lower_copy(s);
      return (s == "1") || (s == "true") || (s == "yes");
   };

   if (fConfig->Count(::daq::service::HostIpAddress.data()) > 0) {
      const auto &ip = get(::daq::service::HostIpAddress);
      fHeaderSrcId = net::ip::make_address(ip).to_v4().to_ulong();
   }
   {
      auto id = fConfig->GetProperty<std::string>("id");
      std::regex r{R"((\d+$))"};
      std::smatch m;
      std::regex_search(id, m, r);
      if (m.size() > 0) {
         auto instanceIndex = std::stoull(m[0].str());
         fHeaderSrcId |= instanceIndex << 32;
      }
   }

   fTagInputChannelName = get(opt::TagInputChannelName);
   fDataChannelName = get(opt::DataChannelName);
   fTimeoutMS = std::stoi(get(opt::Timeout));
   fPollTimeoutMS = std::stoi(get(opt::PollTimeout));
   fDiscard = checkFlag(opt::Discard);

   auto mean = std::stod(get(opt::LengthMean));
   auto stddev = std::stod(get(opt::LengthStddev));
   dist_type::param_type param(mean, stddev);
   dist.param(param);

   if (fChannels.count(fTagInputChannelName) > 0) {
      OnData(fTagInputChannelName, &TagSubscriber::HandleTag);
   }

   fNBeginRun = 0;
}

//_____________________________________________________________________________
void TagSubscriber::PostRun()
{
   if (fChannels.count(fTagInputChannelName) > 0) {
      while (true) {
         auto msg = NewMessage();
         if (Receive(msg, fTagInputChannelName, 0, fTimeoutMS) <= 0) {
            break;
         }
         if (!fDiscard) {
            HandleTag(msg, 0);
         }
      }
   }
}

//_____________________________________________________________________________
void TagSubscriber::PreRun()
{
   LOG(debug) << fClassName << ":" << __func__;
   fNSequence = 0;
   if (fConfig->Count(opt::RunNumber.data()) > 0) {
      fRunNumber = std::stoll(fConfig->GetProperty<std::string>(opt::RunNumber.data()));
      LOG(debug) << " RUN ID = " << fRunNumber;
   }
   CreateBeginRunData();

   const auto n = fChannels.count(fDataChannelName);
   for (auto i = 0; i < n; ++i) {
      auto cpy = MessageUtil::Copy(*this, fBeginRunData);
      Send(cpy, fDataChannelName, i);
      //++fNSequence;
   }
}

} // namespace e16::daq