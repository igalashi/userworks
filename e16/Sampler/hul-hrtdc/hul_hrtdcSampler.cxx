#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>

#include <boost/algorithm/string/case_conv.hpp>

#include "frontend/hul-hrtdc/RegisterMapPath.h"
#include "sitcp/SiTCP_constants.h"
#include "unpacker/GlobalTag.h"
#include "unpacker/MQHeader.h"
#include "unpacker/hul-hrtdc/DataFormat.h"
#include "unpacker/hul-hrtdc/Unpacker.h"
#include "mq/MessageUtil.h"

#include "frontend/hul-hrtdc/Daq.h"
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
   {
      using opt = DaqTCP::OptionKey;
      DaqTCP::AddOptions(options,                                              //
                         {{opt::TcpPort.data(), sitcp::DefaultTCPPort.data()}, //
                          {opt::UdpPort.data(), sitcp::DefaultUDPPort.data()}});
   }
}

//_____________________________________________________________________________
void Sampler::CreateDaq()
{
   try {
      LOG(debug) << " hul::hrtdc CreateDaq";
      if (!regMap) {
         regMap = std::make_shared<e16::RegisterMap>();
         regMap->Parse(hul::hrtdc::RegisterMapPath.data());
         LOG(debug) << " hul::hrtdc::RegisterMap created";
      }
      auto daq = std::make_unique<hul::hrtdc::Daq>();
      daq->SetRegisterMap(regMap);
      auto &&unpacker = std::make_unique<hul::hrtdc::Unpacker>();
      unpacker->SetFindOnly();
      daq->SetUnpacker(std::move(unpacker));
      daq->Init(fConfig->GetVarMap());
      daq->SetName(fId + ":" + daq->GetIPAddress());

      fFemType = fem_type::HUL_HRTDC;
      fDaq = std::move(daq);
   } catch (const std::exception &e) {
      LOG(error) << __func__ << ": e.what() = " << e.what();
   } catch (...) {
      LOG(error) << __func__ << " unknown exception";
   }
}

//______________________________________________________________________________
void Sampler::ProcessData()
{
   using namespace hul::hrtdc;
   const auto &&frame = fDaq->PopFrame();
   if (frame.empty())
      return;

   auto header_buf = CreateHeader();
   auto header = reinterpret_cast<MQHeader *>(header_buf->GetData());
   header->num_sequence = fDaq->GetNumEvent();
   header->msg_type = message_type::Unknown;
   header->body_size = std::accumulate(frame.cbegin(), frame.cend(), 0, [](int acc, auto x) { return acc + x.size(); });

   GlobalTag *tag = reinterpret_cast<GlobalTag *>(header->tag);
   GlobalTag *tagMask = tag + 1;

   auto &u = dynamic_cast<hul::hrtdc::Unpacker &>(dynamic_cast<hul::hrtdc::Daq *>(fDaq.get())->GetUnpacker());
   header->msg_type |= message_type::GlobalTagShort;
   *tag = u.GetGlobalTag();
   SetGlobalTag(*tagMask, 0, 0x1, 0x7); // 1-bit spill, 3-bit event

   fEventID = GetEvent(*tag);
   fSpillID = GetSpill(*tag);
   fTimestamp = GetTime(*tag);

   FairMQParts parts;
   if (!fNoHeader) {
      parts.AddPart(std::move(header_buf));
   }
   for (auto &&m : frame) {
      parts.AddPart(MessageUtil::NewMessage(*this, std::make_unique<std::vector<char>>(std::move(m))));
   }
   auto h = reinterpret_cast<MQHeader *>(parts.At(0)->GetData());

   h->msg_type = message_type::SetTrgTyp(h->msg_type, message_type::TrgTyp::Normal);

   if (fEventWaitMS >= 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(fEventWaitMS));
   }
   //---------------------------------------------------------------------------
   // normal data
   Send(parts, ChannelID::Data);
}

//_____________________________________________________________________________
void Sampler::ProcessIncompleteData() {}

} // namespace e16::daq