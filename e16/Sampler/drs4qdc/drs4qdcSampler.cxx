#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>

#include <boost/algorithm/string/case_conv.hpp>

#include "frontend/drs4qdc/RegisterMapPath.h"
#include "unpacker/GlobalTag.h"
#include "unpacker/MQHeader.h"
#include "unpacker/drs4qdc/DataFormat.h"
#include "unpacker/drs4qdc/Unpacker.h"
#include "unpacker/drs4qdc/AdcUnpackerFactory.h"
#include "unpacker/drs4qdc/TagUnpackerFactory.h"
#include "unpacker/drs4qdc/TagUnpacker.h"
#include "unpacker/drs4qdc/TagUnpackerE16V1.h"
#include "unpacker/drs4qdc/TagUnpackerHDHUL.h"
#include "mq/MessageUtil.h"

#include "frontend/drs4qdc/Daq.h"
#include "frontend/drs4qdc/DaqFileSampler.h"
#include "mq/Sampler/Sampler.h"

namespace bpo = boost::program_options;
using namespace highp::e50;
namespace {
std::shared_ptr<e16::RegisterMap> regMap;
}

namespace e16::daq {
//__________________________________________w__________________________________
void Sampler::AddOptions(bpo::options_description &options)
{
   drs4qdc::Daq::AddOptions(options);
   drs4qdc::DaqFileSampler::AddOptions(options);
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
      LOG(debug) << " drs4qdc::CreateDaq";
      if (!regMap) {
         regMap = std::make_shared<e16::RegisterMap>();
         regMap->Parse(drs4qdc::RegisterMapPath.data());
         LOG(debug) << " drs4qdc::RegisterMap created";
      }
      LOG(debug) << " create unpacker";
      auto unpacker = drs4qdc::CreateUnpacker();
      unpacker->SetFindOnly();
      if (fUseFileSampler) {
         LOG(debug) << " input file name is specified";
         daq_setup(std::make_unique<drs4qdc::DaqFileSampler>, regMap, std::move(unpacker));
      } else {
         daq_setup(std::make_unique<drs4qdc::Daq>, regMap, std::move(unpacker));
      }
      fFemType = fem_type::DRS4_QDC;
   } catch (const std::exception &e) {
      LOG(error) << __func__ << ": e.what() = " << e.what();
   } catch (...) {
      LOG(error) << __func__ << " unknown exception";
   }
}

//______________________________________________________________________________
void Sampler::ProcessData()
{
   // ---------------------------------
   // implementation of ProcessData
   // ---------------------------------
   auto impl = [this](drs4qdc::Unpacker &u, auto &daq) {
      using namespace drs4qdc;
      const auto &&frame = daq.PopFrame();
      if (frame.empty())
         return;

      auto header_buf = CreateHeader();
      auto header = reinterpret_cast<MQHeader *>(header_buf->GetData());
      header->num_sequence = daq.GetNumEvent();
      header->msg_type = message_type::Unknown;
      header->body_size =
         std::accumulate(frame.cbegin(), frame.cend(), 0, [](int acc, auto x) { return acc + x.size(); });

      const auto &format = daq.GetFormat();
      auto j0 = format.fJ0;

      GlobalTag *tag = reinterpret_cast<GlobalTag *>(header->tag);
      GlobalTag *tagMask = tag + 1;

      const auto &trig = u.GetTriggerData();
      if (trig.HasTag()) {
         *tag = u.GetGlobalTag();
         if (j0 == static_cast<int>(J0type::E16)) {
            header->msg_type |= message_type::GlobalTagLong;
            SetGlobalTag(*tagMask);

         } else if (j0 == static_cast<int>(J0type::HD) || j0 == static_cast<int>(J0type::HDHUL)) {
            header->msg_type |= message_type::GlobalTagShort;
            SetGlobalTag(*tagMask, 0, 0x1, 0x7); // 1-bit spill, 3-bit event
         }

         fEventID = GetEvent(*tag);
         fSpillID = GetSpill(*tag);
         fTimestamp = GetTime(*tag);
      }

      // static GlobalTag prev;
      // if (*tag == prev) {
      //   LOG(warn) << fNumIteration << " same tag found = " << prev;
      //} else {
      //   LOG(debug) << fNumIteration << " tag = " << *tag << " prev = " << prev;
      //}
      // prev = *tag;

      /*
      {
         // sumary data
         FairMQParts summary;
         auto headerCpy = NewMessage();
         headerCpy->Copy(*header_buf);
         auto h = reinterpret_cast<MQHeader*>(headerCpy->GetData());
         h->msg_type |= message_type::Summary;
         h->body_size = sizeof(LT);
         summary.AddPart(std::move(headerCpy));

         auto body = NewSimpleMessage(sizeof(LT));
         auto lt = u.GetLocalTimestamp();
         auto first = reinterpret_cast<const char*>(&lt);
         std::copy(first, first + sizeof(lt), reinterpret_cast<char*>(body->GetData()));
         summary.AddPart(std::move(body));
         Send(summary, ChannelID::Summary, false);
      }
      */

      if (fEventWaitMS >= 0) {
         std::this_thread::sleep_for(std::chrono::milliseconds(fEventWaitMS));
      }

      FairMQParts parts;
      if (!fNoHeader) {
         parts.AddPart(std::move(header_buf));
      }
      for (auto &&m : frame) {
         parts.AddPart(MessageUtil::NewMessage(*this, std::make_unique<std::vector<char>>(std::move(m))));
      }
      auto h = reinterpret_cast<MQHeader *>(parts.At(0)->GetData());

      //----------------------------------------------------------------------
      // set message type bits
      if (trig.HasTag()) {
         if (j0 == static_cast<int>(J0type::E16)) {
            h->msg_type |= message_type::GlobalTagLong;
         } else if (j0 == static_cast<int>(J0type::HDHUL) || (j0 == static_cast<int>(J0type::HD))) {
            h->msg_type |= message_type::GlobalTagShort;
         }
      }

      if (trig.HasEvent()) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::Normal);
         if (!trig.HasTag()) {
            h->msg_type |= message_type::LocalTag;
         }
      }

      if (trig.HasPedestal()) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::Pedestal);
      }

      if (trig.HasScaler() || trig.HasSEM() || trig.HasBusyTime()) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::Monitor);
      }

      if (trig.HasSpillStart()) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::SpillStart);
      }

      if (trig.HasSpillEnd()) {
         h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::SpillEnd);
      }

      //----------------------------------------------------------------------
      // pedestal data
      if (trig.HasPedestal()) {
         LOG(debug) << " send pedestal data";
         Send(parts, ChannelID::Pedestal, true);
      }

      //----------------------------------------------------------------------
      // monitor data
      if (trig.HasScaler() || trig.HasSEM() || trig.HasBusyTime()) {
         LOG(debug) << " send monitor (status) data";
         Send(parts, ChannelID::Status, true);
      }

      //----------------------------------------------------------------------
      // spill start data
      if (trig.HasSpillStart()) {
         LOG(debug) << " send spill start data";
         Send(parts, ChannelID::SpillStart, true);
      }

      //----------------------------------------------------------------------
      // spill end data
      if (trig.HasSpillEnd()) {
         LOG(debug) << " send spill end data";
         Send(parts, ChannelID::SpillEnd, true);
      }

      //---------------------------------------------------------------------------
      // normal data
      if (trig.HasEvent()) {
         if (trig.HasTag()) {
            // LOG(debug) << " send data (with global tag)";
            Send(parts, ChannelID::Data);
         } else {
            // LOG(debug) << " send data (without global tag)";
            Send(parts, ChannelID::NoTag);
         }
         return;
      }

      //----------------------------------------------------------------------
      // unknown data
      if (!trig.HasEvent() && !trig.HasPedestal() && !trig.HasScaler() && !trig.HasSEM() && !trig.HasBusyTime() &&
          !trig.HasSpill()) {
         LOG(debug) << " send unknown data";
         Send(parts, ChannelID::Unknown);
      }
   }; // -------------------- end of impl []() --------------------------------

   if (fUseFileSampler) {
      // LOG(debug) << __func__ << " for file read";
      auto &u = dynamic_cast<drs4qdc::Unpacker &>(dynamic_cast<drs4qdc::DaqFileSampler *>(fDaq.get())->GetUnpacker());
      impl(u, dynamic_cast<drs4qdc::DaqFileSampler &>(*fDaq));
   } else {
      // LOG(debug) << __func__ << " for fem read";
      auto &u = dynamic_cast<drs4qdc::Unpacker &>(dynamic_cast<drs4qdc::Daq *>(fDaq.get())->GetUnpacker());
      impl(u, dynamic_cast<drs4qdc::Daq &>(*fDaq));
   }
}

//_____________________________________________________________________________
void Sampler::ProcessIncompleteData() {}

} // namespace e16::daq