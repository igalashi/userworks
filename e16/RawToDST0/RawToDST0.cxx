#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_set>

#include <boost/algorithm/string/case_conv.hpp>

#include <fairmq/tools/Strings.h>
#include <runFairMQDevice.h>

#include "utility/HexDump.h"
#include "unpacker/dst0/UnpackerManagerDST0.h"
#include "mq/MessageUtil.h"
#include "mq/TaskProcessorMT.h"
#include "mq/RawToDST0/RawToDST0.h"

using namespace std::chrono_literals;
namespace bpo = boost::program_options;
using namespace highp::e50;
using opt = e16::daq::RawToDST0::OptionKey;
//=============================================================================
void addCustomOptions(bpo::options_description &options)
{
   options.add_options()
      //
      (opt::InputDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::InputDataChannelName.data())),
       "Name of input data channel")
      //
      (opt::BeginRunDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::BeginRunDataChannelName.data())),
       "Name of begin run data channel")
      //
      (opt::OutputDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::OutputDataChannelName.data())),
       "Name of output data channel")
      //
      (opt::InsertMQHeader.data(), bpo::value<std::string>()->default_value("false"),
       "Insert MQHeader before DST0 data")
      //
      (opt::NThreads.data(), bpo::value<std::string>()->default_value("0"),
       "Number of worker threads (if 0 or negative, number of logical CPU cores  will be used)")
      //
      (opt::Timeout.data(), bpo::value<std::string>()->default_value("1000"),
       "Timeout of message queue in milliseconds")
      //
      (opt::InProcMQLength.data(), bpo::value<std::string>()->default_value("1"), "in-process message queue length");

   // SRS-ATCA
   e16::daq::UnpackerManagerDST0::AddOptions(options);
}

//_____________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
   return new e16::daq::RawToDST0;
}

//=============================================================================
namespace e16::daq {

const std::unordered_set<uint32_t> magicList{
   //
   magic::Sampler,      //
   magic::EventBuilder, //
   magic::Monitor,      //
};

//_____________________________________________________________________________
bool RawToDST0::CheckBeginRunData(FairMQParts &msgParts, int index)
{
   if (msgParts.Size() < 2) {
      LOG(error) << __func__ << " bad size of multipart message = " << msgParts.Size();
      return false;
   }
   // LOG(debug) << " message parts.Size() = " << msgParts.Size();
   //{
   //   int i=0;
   //   for (const auto &m : msgParts) {
   //      LOG(debug) << " dump msgParts[" << i << ""] " << m->GetSize();
   //      MessageUtil::HexDump(m);
   //   }
   //}

   {
      const auto &msg0 = msgParts[0];
      const auto first = reinterpret_cast<const char *>(msg0.GetData());
      auto n = msg0.GetSize();
      const auto last = first + n;
      // std::for_each(first, last, HexDump());
      if (n < sizeof(MQHeader)) {
         LOG(error) << __func__ << " too small message size = " << n;
         return false;
      }
      const auto &mqHeader = *reinterpret_cast<const MQHeader *>(first);
      // LOG(debug) << " msg0 header.magic = " << std::hex << mqHeader.magic << std::dec;
      if (magicList.count(mqHeader.magic) < 1) {
         LOG(error) << __func__ << " unknown magic word in mq header";
      }

      // LOG(debug) << " msg0 header.msg_type = " << std::hex << mqHeader.msg_type //
      //           << " (begin run = " << message_type::BeginRun  //
      //           << " and = " << (mqHeader.msg_type & message_type::BeginRun) << ")" << std::dec;
      if ((mqHeader.msg_type & message_type::BeginRun) == 0) {
         // LOG(error) << __func__ << " message is not begin run data frame.";
         return false;
      }

      LOG(info) << __func__ << " begin run data frame is found.";
   }

   uint64_t id = 0;
   int i = 0;
   for (auto &msg : msgParts) {
      auto n = msg->GetSize();
      auto first = reinterpret_cast<char *>(msg->GetData());
      auto last = first + n;
      LOG(debug) << " dump msgParts[" << i << "]  nbytes = " << n;
      // std::for_each(first, last, HexDump());

      // header part of begin run sub-frame is skipped.
      if (n >= sizeof(MQHeader)) {
         const auto &mqHeader = *reinterpret_cast<MQHeader *>(msg->GetData());
         if (magicList.count(mqHeader.magic) > 0) {
            // LOG(debug) << " begin run data: header found";
            id = *reinterpret_cast<const uint64_t *>(mqHeader.id);
            ++i;
            continue;
         }
      }

      // body part of begin run sub-frame is filled.
      LOG(debug) << " fill begin run data: id = " << std::hex << id << std::dec;
      fBeginRunData[id].emplace_back(std::make_move_iterator(first), std::make_move_iterator(last));
      ++i;
   }
   LOG(debug) << __func__ << ":" << __LINE__ << " begin run data info : .size() = " << fBeginRunData.size();
   for (const auto &[id, data] : fBeginRunData) {
      LOG(debug) << " id = " << std::hex << id << std::dec << ", size = " << data.size();
   }
   LOG(debug) << __func__ << ":" << __LINE__ << " done";
   return true;
}

//_____________________________________________________________________________
FairMQParts RawToDST0::Convert(FairMQParts &msgParts, int index)
{
   // LOG(debug) << __func__ << ":" << __LINE__ << " msgParts.Size() = " << msgParts.Size();
   UnpackerManagerDST0 &unpacker = (fWorker) ? *fUnpackers[index] : *fUnpackers[0];

   const MQHeader *mqHeader = reinterpret_cast<const MQHeader *>(msgParts[0].GetData());
   // MessageUtil::HexDump<char>(msgParts.At(0));
   unpacker.CreateEvent(*mqHeader);

   FairMQMessagePtr dst0MsgHeader(NewMessage());
   dst0MsgHeader->Copy(msgParts[0]);
   auto *outMqHeader = reinterpret_cast<MQHeader *>(dst0MsgHeader->GetData());
   outMqHeader->magic = magic::Monitor;

   std::vector<char> buf; // event data of one FEM
   auto i = 0;
   for (auto itr = msgParts.begin(), itrEnd = msgParts.end(); itr != itrEnd; ++itr, ++i) {
      const auto &msg = *itr;
      // LOG(debug) << __func__ << ":" << __LINE__ << " " << i << " msg->GetSize()" << msg->GetSize();
      if (msg->GetSize() >= sizeof(MQHeader)) {
         const auto h = reinterpret_cast<const MQHeader *>(msg->GetData());
         if (magicList.count(h->magic) > 0) {
            // update header
            mqHeader = h;
            // LOG(debug) << __func__ << ":" << __LINE__ << " " << i << " mq header->body_size = " <<
            // mqHeader->body_size;
            continue;
         }
      }

      const auto first = reinterpret_cast<const char *>(msg->GetData());
      const auto last = first + msg->GetSize();

      buf.insert(buf.end(), std::make_move_iterator(first), std::make_move_iterator(last));
      // LOG(debug) << __func__ << ":" << __LINE__ << " " << i << " buf.size() = " << buf.size() << " mq
      // header->body_size = " << mqHeader->body_size;
      if (buf.size() == mqHeader->body_size) {
         auto id = *reinterpret_cast<const uint64_t *>(mqHeader->id);
         // LOG(debug) << __LINE__ << " id = " << std::hex << id << std::dec;

         if (fBeginRunData.count(id) < 0) {
            LOG(error) << __func__ << ":" << __LINE__ << " unknown source id = " << std::hex << id << std::dec;
            continue;
         }
         const auto &beginRun = fBeginRunData[id];
         // LOG(debug) << __LINE__ << " id = " << std::hex << id << std::dec << " beginRun.size() = " <<
         // beginRun.size();

         if (beginRun.empty()) {
            LOG(error) << __func__ << " bad queue size of begin run data: id = " << std::hex << id << std::dec
                       << ", beginRun.size() = " << beginRun.size();
            buf.clear();
            continue;
         }
         unpacker.Unpack(*mqHeader, buf.cbegin(), beginRun[0].cbegin());
         buf.clear();
      }
   }

   // LOG(debug) << __func__ << ":" << __LINE__ << " serialize event. dst0.GetEventSize() = " <<
   // unpacker.DST0().GetEventSize();
   auto nserial = unpacker.SerializeAnEvent();
   // LOG(debug) << __func__ << ":" << __LINE__ << " n serial = " << nserial;
   auto dst0MsgBody =
      MessageUtil::NewMessage(*this, std::make_unique<std::vector<char>>(std::move(unpacker.ReleaseEventBuffer())));
   outMqHeader->body_size = dst0MsgBody->GetSize();

   FairMQParts dst0Msg;
   if (fInsertMQHeader) {
      // LOG(debug) << __func__ << ":" << __LINE__ << " dst0 insert header";
      dst0Msg.AddPart(std::move(dst0MsgHeader));
   }
   // LOG(debug) << __func__ << ":" << __LINE__ << " dst0 add body";
   dst0Msg.AddPart(std::move(dst0MsgBody));
   return dst0Msg;
}

//_____________________________________________________________________________
bool RawToDST0::HandleData(FairMQParts &msgParts, int index)
{
   // LOG(debug) << __func__ << ":" << __LINE__ << " msgParts.Size() = " << msgParts.size() << " index = " << index;
   if (CheckBeginRunData(msgParts, index)) {
      LOG(debug) << __func__ << ":" << __LINE__ << " begin run data found";
      return true;
   }
   ++fNReceived;
   auto dst0Msg = Convert(msgParts, index);
   return SendData(dst0Msg, index);
}

//_____________________________________________________________________________
bool RawToDST0::HandleDataMT(FairMQParts &msgParts, int index)
{
   if (CheckBeginRunData(msgParts, index)) {
      LOG(debug) << __func__ << ":" << __LINE__ << " begin run data found";
      return true;
   }
   auto i = fNReceived % fNThreads;
   ++fNReceived;

   // LOG(debug) << fClassName << ":" << __LINE__ << " splitter channel send i = " << i << " n-received = " <<
   // fNReceived << " n-send = " << fNSend;
   while (fWorker->fSplitterChannels[i]->Send(msgParts, fMQTimeoutMS) < 0) {
      // LOG(warn) << __func__ << " (local mq) failed to send";
      if (NewStatePending()) {
         return false;
      }
   }

   return true;
}

//_____________________________________________________________________________
void RawToDST0::InitTask()
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

   fNThreads = std::stoi(get(opt::NThreads));
   if (fNThreads < 1) {
      fNThreads = std::thread::hardware_concurrency();
   }
   LOG(debug) << " number of worker threads = " << fNThreads;
   fInProcMQLength = std::stoi(get(opt::InProcMQLength));

   fInputDataChannelName = get(opt::InputDataChannelName);
   fBeginRunDataChannelName = get(opt::BeginRunDataChannelName);
   fOutputDataChannelName = get(opt::OutputDataChannelName);

   fInsertMQHeader = checkFlag(opt::InsertMQHeader);

   UnpackerManagerDST0::InitializeSharedParameters(fConfig->GetVarMap());

   fWorker.reset();
   if (fNThreads > 1) {
      LOG(debug) << " set multithread handler for input channel";
      OnData(fInputDataChannelName, &RawToDST0::HandleDataMT);
      fWorker = std::make_unique<TaskProcessorMT>(*this);
   } else {
      LOG(debug) << " set single thread handler for input channel";
      OnData(fInputDataChannelName, &RawToDST0::HandleData);
   }
}

//_____________________________________________________________________________
void RawToDST0::PostRun()
{
   if (fWorker) {
      fWorker->Join();
   }
   if (fChannels.count(fInputDataChannelName) > 0) {
      auto n = fChannels.count(fInputDataChannelName);
      for (auto i = 0; i < n; ++i) {
         for (auto itry = 0; itry < 10; ++itry) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            FairMQParts part;
            if (Receive(part, fInputDataChannelName, i, fMQTimeoutMS) > 0) {
               // if (!fDiscard) {
               //   HandleMultipartData(part, i);
               //}
            } else {
               break;
            }
         }
      }
   }
}

//_____________________________________________________________________________
void RawToDST0::PreRun()
{
   fNReceived = 0;
   fNSend = 0;
   fUnpackers.clear();

   if (fWorker) {
      LOG(debug) << " setup threads: n = " << fNThreads;
      fWorker->InitChannels(fNThreads, fInProcMQLength, fMQTimeoutMS);
      for (auto i = 0; i < fNThreads; ++i) {
         fUnpackers.emplace_back(std::make_unique<UnpackerManagerDST0>());
      }

      if (!fWorker->fHandleInputMultipart) {
         LOG(debug) << " set multithread handler for worker input data";
         fWorker->fHandleInputMultipart = [this](auto &msgParts, auto index) {
            // LOG(debug) << fClassName << ":" << __LINE__ << " index = " << index;
            auto dst0Msg = Convert(msgParts, index);
            fWorker->fWorkerOutputChannels[index]->Send(dst0Msg, fMQTimeoutMS);
            return true;
         };
      }
      if (!fWorker->fHandleOutputMultipart) {
         // LOG(debug) << " set multithread handler for worker output data";
         fWorker->fHandleOutputMultipart = [this](auto &msgParts, auto index) { return SendData(msgParts, index); };
      }
      fWorker->Run();
   } else {
      fUnpackers.emplace_back(std::make_unique<UnpackerManagerDST0>());
   }
}

//_____________________________________________________________________________
bool RawToDST0::SendData(FairMQParts &msgParts, int index)
{
   // LOG(debug) << fClassName << ":" << __LINE__ << " index = " << index << " n-receive = " << fNReceived << " n-send =
   // " << fNSend;
   Send(msgParts, fOutputDataChannelName);
   ++fNSend;
   return true;
}

} // namespace e16::daq