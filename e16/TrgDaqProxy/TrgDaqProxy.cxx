#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include <thread>

#include <boost/asio.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <runFairMQDevice.h>

#include "utility/DecompressHelper.h"
#include "utility/Decompressor.h"
#include "utility/HexDump.h"
#include "unpacker/GlobalTag.h"
#include "unpacker/MQHeader.h"
#include "unpacker/trg/TrgData.h"
#include "unpacker/trg/run0b/TrgData.h"
#include "unpacker/trg/run0c/TrgData.h"
#include "unpacker/trg/run0b/UnpackerTrgDaq.h"
#include "unpacker/trg/run0c/UnpackerTrgDaq.h"

#include "mq/DaqServiceConstants.h"
#include "mq/MessageUtil.h"
#include "mq/TrgDaqProxy/TrgDaqProxy.h"

namespace net = boost::asio;
namespace bpo = boost::program_options;
using namespace std::literals::string_literals;
using namespace highp::e50;
using opt = e16::daq::TrgDaqProxy::OptionKey;

//=============================================================================
void addCustomOptions(bpo::options_description &options)
{
   options.add_options()
      //
      (opt::Multipart.data(), bpo::value<std::string>()->default_value("true"), "Flag to use multipart message")
      //
      (opt::InputDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::InputDataChannelName.data())),
       "Input data channel name")
      //
      (opt::OutputDataChannelName.data(),
       bpo::value<std::string>()->default_value(std::string(opt::OutputDataChannelName.data())),
       "Output data channel name")
      //
      (opt::TrgMrgIp.data(), bpo::value<std::vector<std::string>>()->multitoken(), "IP list of TRG-MRG")
      //
      (opt::Ut3Ip.data(), bpo::value<std::vector<std::string>>()->multitoken(), "IP list of UT3")
      //
      (opt::InputDataFileName.data(), bpo::value<std::string>(), "Input data file name")
      //
      (opt::UnpackerVersion.data(), bpo::value<std::string>()->default_value("run0c"),
       "TrgDaq Unpacker version (run0b | run0c)")
      //
      (opt::RunNumber.data(), bpo::value<std::string>(), "Run number (integer)");
}

//_____________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
   return new e16::daq::TrgDaqProxy;
}

//=============================================================================

namespace e16::daq {
//_____________________________________________________________________________
// for file read
bool TrgDaqProxy::ConditionalRun()
{
   fBuffer.clear();
   fBuffer.reserve(fBufferSize);
   uint32_t msg_type{message_type::Unknown};

   for (auto &unpacker : fUnpackers) {
      while (!fStream->eof()) {
         auto n = unpacker->NextBytes();
         auto m = fBuffer.size();
         if (n > 0) {
            fBuffer.resize(m + n);
            fStream->read(fBuffer.data() + m, n);
            auto rcount = fStream->gcount();
            if (rcount <= 0) {
               break;
            }

            unpacker->Unpack(fBuffer.cbegin() + m);
         }

         if (unpacker->IsCompleted()) {
            break;
         }
         unpacker->TransToNext();
      }
      unpacker->Reset();
   }

   if (fBeginRunData.Size() < 0) {
      SendBeginRunData();
   }

   auto body = MessageUtil::NewMessage(*this, std::make_unique<std::vector<char>>(std::move(fBuffer)));
   auto header = CreateHeader(body->GetSize(), msg_type);

   // TO DO: write tag data to header

   FairMQParts parts(std::move(header), std::move(body));

   ++fNumSequence;

   Send(parts, fOutputDataChannelName);

   return true;
}

//_____________________________________________________________________________
FairMQMessagePtr TrgDaqProxy::CreateHeader(uint32_t bodySize, uint32_t messageType)
{
   std::size_t n = sizeof(MQHeader);
   auto ret = NewMessage(n);
   {
      auto buf = reinterpret_cast<char *>(ret->GetData());
      std::fill(buf, buf + ret->GetSize(), 0);
   }
   auto header = reinterpret_cast<MQHeader *>(ret->GetData());
   header->magic = magic::TriggerDaq;

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

//_____________________________________________________________________________
// for received message
bool TrgDaqProxy::HandleMultipartData(FairMQParts &msgParts, int index)
{
   auto len = MessageUtil::TotalLength(msgParts);
   std::vector<char> body(len);
   len = 0;
   for (auto &m : msgParts) {
      auto first = reinterpret_cast<char *>(m->GetData());
      auto last = first + m->GetSize();
      std::copy(std::make_move_iterator(first), std::make_move_iterator(last), body.begin() + len);
      len += m->GetSize();
   }

   uint32_t msg_type{message_type::Unknown};
   auto itr = body.cbegin();
   auto last = body.cend();

   for (auto &unpacker : fUnpackers) {
      while (itr != last) {
         auto n = unpacker->NextBytes();
         if (n > 0) {
            unpacker->Unpack(itr);
            itr += n;
         }

         if (unpacker->IsCompleted()) {
            break;
         }
         unpacker->TransToNext();
      }
      unpacker->Reset();
   }

   return true;
}

//_____________________________________________________________________________
void TrgDaqProxy::InitTask()
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

   auto getIPs = [this](auto name) -> std::vector<uint64_t> {
      std::vector<uint64_t> ret;
      if (fConfig->Count(name.data()) > 0) {
         const auto &ipList = fConfig->GetProperty<std::vector<std::string>>(name.data());
         std::stringstream ss;
         ss << name;
         for (const auto &x : ipList) {
            ss << x << " ";
            ret.push_back(net::ip::make_address(x).to_v4().to_ulong());
         }
         LOG(info) << ss.str();
      } else {
         LOG(warn) << name << ": no ip list is specified. exit";
         std::exit(0);
      }
   };

   fUnpackerVersion = get(opt::UnpackerVersion);
   boost::to_lower(fUnpackerVersion);
   fTrgMrgIp = getIPs(opt::TrgMrgIp);
   fUt3Ip = getIPs(opt::Ut3Ip);

   fInputDataFileName = get(opt::InputDataFileName);
   if (fInputDataFileName.empty()) {
      LOG(debug) << " input data file name is empty.";
      fInputDataChannelName = get(opt::InputDataChannelName);
      fMultipart = checkFlag(opt::Multipart);
      OnData(fInputDataChannelName, &TrgDaqProxy::HandleMultipartData);
   } else {
      LOG(debug) << " input data file name = " << fInputDataFileName;
   }

   fOutputDataChannelName = get(opt::OutputDataChannelName);

   auto CreateUnpacker = [this](auto id, auto femType) {
      auto Init = [this](auto creator, auto id, auto femType) {
         auto unpacker = creator();
         unpacker->SetID(id);
         unpacker->SetFemType(femType);
         fUnpackers.emplace_back(std::move(unpacker));
      };
      if (fUnpackerVersion == "run0b") {
         Init(std::make_unique<run0b::TrgDaq::Unpacker>, id, femType);
      } else if (fUnpackerVersion == "run0c") {
         Init(std::make_unique<run0c::TrgDaq::Unpacker>, id, femType);
      }
   };
   for (const auto &id : fTrgMrgIp) {
      CreateUnpacker(id, fem_type::TRG_MRG);
   }
   for (const auto &id : fUt3Ip) {
      CreateUnpacker(id, fem_type::UT3);
   }

   std::vector<std::size_t> bufSize;
   {
      using namespace e16::daq::run0b::Mrg::Data;
      bufSize.push_back(sizeof(Main::Bits));
      bufSize.push_back(sizeof(Monitor::Bits));
      bufSize.push_back(sizeof(Spill::Bits));
      bufSize.push_back(sizeof(SpillFirst::Bits));
   }
   {
      using namespace e16::daq::run0b::Ut3::Data;
      bufSize.push_back(sizeof(Main::Bits));
      bufSize.push_back(sizeof(Monitor::Bits));
      bufSize.push_back(sizeof(Spill::Bits));
      bufSize.push_back(sizeof(SpillFirst::Bits));
   }
   fBufferSize = *std::max_element(bufSize.cbegin(), bufSize.cend());
   fBufferSize *= fUnpackers.size();
}

//_____________________________________________________________________________
void TrgDaqProxy::PostRun()
{
   if (!fInputDataFileName.empty()) {
      io::close(*fStream);
   }
}

//_____________________________________________________________________________
void TrgDaqProxy::PreRun()
{
   if (!fInputDataFileName.empty()) {
      fInFile.open(fInputDataFileName, std::ios::binary);
      if (!fInFile.good()) {
         LOG(error) << __FILE__ << ":" << __func__ << ":" << __LINE__
                    << " failed to open begin run data file: " << fInputDataFileName;
         return;
      }
      Decompressor::Format compressionFormat = Decompressor::ExtToFormat(fInputDataFileName);
      LOG(debug) << __func__ << ":" << __LINE__ << " input data filename = " << fInputDataFileName
                 << " compression format = " << compressionFormat;
      fStream = Decompressor::CreateFilter<io::filtering_istream>(compressionFormat);
      fStream->push(fInFile);
   }

   if (fConfig->Count(opt::RunNumber.data()) > 0) {
      fRunNumber = std::stoi(fConfig->GetProperty<std::string>(opt::RunNumber.data()));
      LOG(debug) << " RUN number = " << fRunNumber;
   }

   fNumSequence = 0;
}

//_____________________________________________________________________________
void TrgDaqProxy::SendBeginRunData()
{
   LOG(debug) << __func__;
   using namespace e16::daq::run0b::TrgDaq;
   auto bodySize = NBytesDaqCondition + fTrgMrgIp.size() * trg_mrg::NBytesDaqCondition + ut3::NBytesDaqCondition;

   auto body = NewMessage(bodySize);
   auto pBody = reinterpret_cast<char *>(body->GetData());
   std::copy(DaqConditionMagic.cbegin(), DaqConditionMagic.cend(), pBody);
   pBody += NBytesDaqCondition;

   for (auto id : fTrgMrgIp) {
      std::copy(trg_mrg::DaqConditionMagic.cbegin(), trg_mrg::DaqConditionMagic.cend(), pBody);

      auto p = reinterpret_cast<uint64_t *>(pBody + 32);
      *p = id;

      pBody += trg_mrg::NBytesDaqCondition;
   }
   for (auto id : fUt3Ip) {
      std::copy(ut3::DaqConditionMagic.cbegin(), ut3::DaqConditionMagic.cend(), pBody);
      auto p = reinterpret_cast<uint64_t *>(pBody + 32);
      *p = id;
      pBody += ut3::NBytesDaqCondition;
   }

   auto header = CreateHeader(body->GetSize(), message_type::BeginRun);

   if (fBeginRunData.Size() > 0) {
      fBeginRunData.fParts.clear();
   }
   fBeginRunData.AddPart(std::move(header));
   fBeginRunData.AddPart(std::move(body));

   auto parts = MessageUtil::Copy(*this, fBeginRunData);
   Send(parts, fOutputDataChannelName, 0);
}

} // namespace e16::daq