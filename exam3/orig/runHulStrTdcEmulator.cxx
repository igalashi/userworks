#include <runFairMQDevice.h>

#include "HulStrTdcEmulator.h"

namespace bpo = boost::program_options;

//______________________________________________________________________________
void
addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::HulStrTdcEmulator::OptionKey;
  options.add_options()
    (opt::MaxIterations.data(),     bpo::value<uint64_t>()->default_value(0),        "Number of run iterations (0 - infinite)")
    (opt::MsgRate.data(),           bpo::value<int>()->default_value(100),           "Message rate limit in maximum number of messages per second")
    (opt::MsgSize.data(),           bpo::value<int>()->default_value(1024),          "Message size in bytes")
    (opt::HBFRate.data(),           bpo::value<int>()->default_value(1),             "Heartbeat frame (HBF) rate. 1 HBF per N messages")
    (opt::HBFPosition.data(),       bpo::value<int>()->default_value(0),             "Heartbeat frame (HBF) position.")
    (opt::NumHBF.data(),            bpo::value<int>()->default_value(1),             "Number of HBF in 1 spill")
    (opt::OutputChannelName.data(), bpo::value<std::string>()->default_value("out"), "Name of the output channel")
    ;
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::HulStrTdcEmulator;
}
