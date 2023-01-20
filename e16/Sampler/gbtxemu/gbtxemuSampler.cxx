#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>

#include "unpacker/GlobalTag.h"
#include "unpacker/MQHeader.h"
#include "unpacker/gbtxemu/Unpacker.h"
#include "mq/MessageUtil.h"

#include "frontend/gbtxemu/Daq.h"
#include "frontend/gbtxemu/DaqFileSampler.h"
#include "mq/Sampler/Sampler.h"

namespace bpo = boost::program_options;
using namespace highp::e50;
using namespace gbtxemu;

namespace e16::daq {
//_____________________________________________________________________________
void Sampler::AddOptions(bpo::options_description &options)
{
   gbtxemu::Daq::AddOptions(options);
   gbtxemu::DaqFileSampler::AddOptions(options);
}

//_____________________________________________________________________________
void Sampler::CreateDaq()
{
   auto daq_setup = [this](auto &ctor, auto &&u) {
      auto daq = ctor();
      daq->SetUnpacker(std::move(u));
      daq->Init(fConfig->GetVarMap());
      daq->SetName(fId + ":" + daq->GetIPAddress());
      fDaq = std::move(daq);
   };

   try {
      LOG(debug) << " gbtxemu CreateDaq";

      auto unpacker = std::make_unique<gbtxemu::Unpacker>();
      unpacker->SetFindOnly();

      if (fUseFileSampler) {
         LOG(debug) << " launch file sampler";
         daq_setup(std::make_unique<gbtxemu::DaqFileSampler>, std::move(unpacker));
         LOG(debug) << " launch file sampler done";
      } else {
         LOG(debug) << " launch tcp sampler";
         daq_setup(std::make_unique<gbtxemu::Daq>, std::move(unpacker));
         LOG(debug) << " launch tcp sampler done";
      }
      auto daq = std::make_unique<gbtxemu::Daq>();
   } catch (const std::exception &e) {
      LOG(error) << __func__ << ": e.what() " << e.what();
   } catch (...) {
      LOG(error) << __func__ << ": unknown exception";
   }
}

//_____________________________________________________________________________
void Sampler::ProcessData()
{
   auto impl = [this](gbtxemu::Unpacker &u, auto &daq) {
      using namespace gbtxemu;
      const auto &&frame = daq.PopFrame();
      if (frame.empty()) {
         return;
      }

      auto header_buf = CreateHeader();
      auto header = reinterpret_cast<MQHeader *>(header_buf->GetData());
      header->num_sequence = daq.GetNumEvent();
      header->msg_type = message_type::StreamingData;
      header->body_size =
         std::accumulate(frame.cbegin(), frame.cend(), 0, [](int acc, auto x) { return acc + x.size(); });

      FairMQParts parts;
      if (!fNoHeader) {
         parts.AddPart(std::move(header_buf));
      }

      for (auto &&m : frame) {
         parts.AddPart(MessageUtil::NewMessage(*this, std::make_unique<std::vector<char>>(std::move(m))));
      }
      auto h = reinterpret_cast<MQHeader *>(parts.At(0)->GetData());

      Send(parts, ChannelID::Data);
   };

   if (fUseFileSampler) {
      auto &u = dynamic_cast<gbtxemu::Unpacker &>(dynamic_cast<gbtxemu::DaqFileSampler *>(fDaq.get())->GetUnpacker());
      impl(u, dynamic_cast<gbtxemu::DaqFileSampler &>(*fDaq));

   } else {
      auto &u = dynamic_cast<gbtxemu::Unpacker &>(dynamic_cast<gbtxemu::Daq *>(fDaq.get())->GetUnpacker());
      impl(u, dynamic_cast<gbtxemu::Daq &>(*fDaq));
   }
}

//_____________________________________________________________________________
void Sampler::ProcessIncompleteData() {}

} // namespace e16::daq