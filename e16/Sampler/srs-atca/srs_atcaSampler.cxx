#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>

#include "frontend/srs-atca/RegisterMapPath.h"
#include "unpacker/GlobalTag.h"
#include "unpacker/MQHeader.h"
#include "unpacker/srs-atca/Unpacker.h"
#include "mq/MessageUtil.h"

#include "frontend/srs-atca/Daq.h"
#include "frontend/srs-atca/DaqFileSampler.h"
#include "mq/Sampler/Sampler.h"

namespace bpo = boost::program_options;
using namespace highp::e50;
using namespace srs_atca;

namespace {
std::shared_ptr<e16::RegisterMap> regMap;
}

namespace e16::daq {
//_____________________________________________________________________________
void Sampler::AddOptions(bpo::options_description &options)
{
   srs_atca::Daq::AddOptions(options);
   srs_atca::DaqFileSampler::AddOptions(options);
}

//_____________________________________________________________________________
void Sampler::CreateDaq()
{
   auto daq_setup = [this](auto &f, auto &r, auto &&u) {
      auto daq = f();
      daq->SetRegisterMap(r);
      daq->SetUnpacker(std::move(u));
      daq->Init(fConfig->GetVarMap());
      daq->SetName(fId + ":" + daq->GetIPAddress());
      fDaq = std::move(daq);
   };
   try {
      LOG(debug) << " srs_atca::CreateDaq";
      if (!regMap) {
         regMap = std::make_shared<e16::RegisterMap>();
         regMap->Parse(srs_atca::RegisterMapPath.data());
         LOG(debug) << " srs_atca::RegisterMap created";
      }
      LOG(debug) << " create unpacker";
      auto unpacker = std::make_unique<srs_atca::Unpacker>();
      unpacker->SetFindOnly();
      if (fUseFileSampler) {
         LOG(debug) << " input file name is specified";
         daq_setup(std::make_unique<srs_atca::DaqFileSampler>, regMap, std::move(unpacker));
      } else {
         daq_setup(std::make_unique<srs_atca::Daq>, regMap, std::move(unpacker));
      }
      fFemType = fem_type::SRS_ATCA;
   } catch (const std::exception &e) {
      LOG(error) << __func__ << ": e.what() = " << e.what();
   } catch (...) {
      LOG(error) << __func__ << ": unknown exception";
   }
}

//______________________________________________________________________________
void Sampler::ProcessData()
{
   // LOG(debug) << daq.GetName() << " " << __FUNCTION__ << ":" << __LINE__;

   // -------------------------------------------
   // implementation of ProcessData
   // -------------------------------------------
   auto impl = [this](srs_atca::Unpacker &u) {
      const auto &&frame = fDaq->PopFrame();
      if (frame.empty()) {
         return;
      }

      // LOG(debug) << " create header";
      auto header_buf = CreateHeader();
      auto header = reinterpret_cast<MQHeader *>(header_buf->GetData());
      header->num_sequence = fDaq->GetNumEvent();
      header->msg_type = message_type::Unknown;
      header->body_size =
         std::accumulate(frame.cbegin(), frame.cend(), 0, [](int acc, auto x) { return acc + x.size(); });

      // LOG(debug) << " extract global tag from header. (body size =  " << header->body_size << ")";
      GlobalTag *tag = reinterpret_cast<GlobalTag *>(header->tag);
      GlobalTag *tagMask = tag + 1;

      // LOG(debug) << " check global flag";
      if (u.HasGlobalTag()) {
         header->msg_type |= message_type::GlobalTagLong;
         (*tag) = u.GetGlobalTag();
         // LOG(debug) << __func__ << " tag = " << *tag;
         SetGlobalTag(*tagMask);
         // LOG(debug) << __func__ << " tag mask = " << *tagMask;

         fEventID = GetEvent(*tag);
         fSpillID = GetSpill(*tag);
         fTimestamp = GetTime(*tag);
      }

      if (fEventWaitMS >= 0) {
         std::this_thread::sleep_for(std::chrono::milliseconds(fEventWaitMS));
      }
      // LOG(debug) << " fill multipart message";
      FairMQParts parts;
      if (!fNoHeader) {
         parts.AddPart(std::move(header_buf));
      }
      for (auto &&m : frame) {
         parts.AddPart(MessageUtil::NewMessage(*this, std::make_unique<std::vector<char>>(std::move(m))));
      }
      auto h = reinterpret_cast<MQHeader *>(parts.At(0)->GetData());

      //----------------------------------------------------------------
      // set message type bits
      const auto &trig = u.GetTriggerFlag();
      if (trig[TriggerFlag::TRG]) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::Normal);
         // LOG(debug) << " set message type = normal";
         if (!u.HasGlobalTag()) {
            h->msg_type |= message_type::LocalTag;
            // LOG(debug) << " set message type += local tag";
         }
      }

      if (trig[TriggerFlag::PEDESTAL]) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::Pedestal);
         // LOG(debug) << " set message type += pedestal";
      }

      if (trig[TriggerFlag::SEM] | trig[TriggerFlag::BTIME]) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::Monitor);
         // LOG(debug) << " set message type += status";
      }

      if (trig[TriggerFlag::SPILL_START]) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::SpillStart);
         // LOG(debug) << " set message type += spill start";
      }

      if (trig[TriggerFlag::SPILL_END]) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::SpillEnd);
         // LOG(debug) << " set message type += spill end";
      }

      //--------------------------------------------------
      // pedestal data
      if (trig[TriggerFlag::PEDESTAL]) {
         LOG(debug) << " send pedestal data";
         Send(parts, ChannelID::Pedestal, true);
      }

      //--------------------------------------------------
      // monitor data
      if (trig[TriggerFlag::SEM] | trig[TriggerFlag::BTIME]) {
         LOG(debug) << " send monitor (status) data";
         Send(parts, ChannelID::Status, true);
      }

      //--------------------------------------------------
      // spill start data
      if (trig[TriggerFlag::SPILL_START]) {
         LOG(debug) << " send spill start data";
         Send(parts, ChannelID::SpillStart, true);
      }

      //--------------------------------------------------
      // spill end data
      if (trig[TriggerFlag::SPILL_END]) {
         LOG(debug) << " send spill end data";
         Send(parts, ChannelID::SpillEnd, true);
      }

      //--------------------------------------------------
      // normal data
      if (trig[TriggerFlag::TRG]) {
         if (u.HasGlobalTag()) {
            // LOG(debug) << " send data (with global tag)";
            Send(parts, ChannelID::Data);
         } else {
            // LOG(debug) << " send data (without global tag)";
            Send(parts, ChannelID::NoTag);
         }
         return;
      }

      //-------------------------------------------------
      // unknown data
      if (!trig[TriggerFlag::TRG] && !trig[TriggerFlag::PEDESTAL] && !trig[TriggerFlag::SEM] &&
          !trig[TriggerFlag::BTIME] && !trig[TriggerFlag::SPILL_START] && !trig[TriggerFlag::SPILL_END]) {
         LOG(debug) << " send unknown data";
         Send(parts, ChannelID::Unknown);
      }
   }; // -------------------- end of impl []() { ... --------------------------

   if (fUseFileSampler) {
      auto &u = dynamic_cast<srs_atca::Unpacker &>(dynamic_cast<srs_atca::DaqFileSampler *>(fDaq.get())->GetUnpacker());
      impl(u);
   } else {
      auto &u = dynamic_cast<srs_atca::Unpacker &>(dynamic_cast<srs_atca::Daq *>(fDaq.get())->GetUnpacker());
      impl(u);
   }
}

//_____________________________________________________________________________
void Sampler::ProcessIncompleteData()
{
   const auto &&frame = fDaq->PopIncompleteFrame();
   if (frame.empty())
      return;

   auto header_buf = CreateHeader();
   auto header = reinterpret_cast<MQHeader *>(header_buf->GetData());
   header->num_sequence = fDaq->GetNumEvent();
   header->msg_type = message_type::Unknown;
   header->body_size = std::accumulate(frame.cbegin(), frame.cend(), 0, [](int acc, auto x) { return acc + x.size(); });

   GlobalTag *tag = reinterpret_cast<GlobalTag *>(header->tag);
   GlobalTag *tagMask = tag + 1;
   SetGlobalTag(*tagMask);

   FairMQParts parts;
   if (!fNoHeader) {
      parts.AddPart(std::move(header_buf));
   }
   for (auto &&m : frame) {
      parts.AddPart(MessageUtil::NewMessage(*this, std::make_unique<std::vector<char>>(std::move(m))));
   }

   Send(parts, ChannelID::Unknown);
}

} // namespace e16::daq
