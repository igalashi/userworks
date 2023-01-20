#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>

#include <boost/algorithm/string/case_conv.hpp>

#include "frontend/drs4qdc/RegisterMapPath.h"
#include "sitcp/SiTCP_constants.h"
#include "unpacker/GlobalTag.h"
#include "unpacker/MQHeader.h"
#include "mq/MessageUtil.h"

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
      //      using opt = DaqTCP::OptionKey;
      //      DaqTCP::AddOptions(options,                                              //
      //                         {{opt::TcpPort.data(), sitcp::DefaultTCPPort.data()}, //
      //                          {opt::UdpPort.data(), sitcp::DefaultUDPPort.data()}});
      //      drs4qdc::DaqFileSampler::AddOptions(options);
   }
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
}

//______________________________________________________________________________
void Sampler::ProcessData()
{
   // ---------------------------------
   // implementation of ProcessData
   // ---------------------------------
}

//_____________________________________________________________________________
void Sampler::ProcessIncompleteData() {}

} // namespace e16::daq