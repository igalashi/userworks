#include <runFairMQDevice.h>

#include "HulStrTdcSampler.h"

namespace bpo = boost::program_options;

//______________________________________________________________________________
void
addCustomOptions(bpo::options_description& options)
{
    using opt = highp::e50::HulStrTdcSampler::OptionKey;
    options.add_options()
      (opt::IpSiTCP.data(),           bpo::value<std::string>()->default_value("0"),   "SiTCP IP (xxx.yyy.zzz.aaa)")
      (opt::OutputChannelName.data(), bpo::value<std::string>()->default_value("out"), "Name of the output channel")
      (opt::TotFilterEn.data(),       bpo::value<int>()->default_value(0),             "Enable TOT filter (0/1)")
      (opt::TotMinTh.data(),          bpo::value<int>()->default_value(0),             "TOT min. threshold (0-200)")
      (opt::TotMaxTh.data(),          bpo::value<int>()->default_value(200),           "TOT max. threshold (0-200)")
      (opt::TotZeroAllow.data(),      bpo::value<int>()->default_value(1),             "TOT zero allowed (0/1)")
      (opt::TWCorr0.data(),           bpo::value<int>()->default_value(0),             "TimeWalk correction (0-255)")
      (opt::TWCorr1.data(),           bpo::value<int>()->default_value(0),             "TimeWalk correction (0-255)")
      (opt::TWCorr2.data(),           bpo::value<int>()->default_value(0),             "TimeWalk correction (0-255)")
      (opt::TWCorr3.data(),           bpo::value<int>()->default_value(0),             "TimeWalk correction (0-255)")
      (opt::TWCorr4.data(),           bpo::value<int>()->default_value(0),             "TimeWalk correction (0-255)")
      ;
  //   (opt::MaxIterations.data(),     bpo::value<uint64_t>()->default_value(0),       "Number of run iterations (0 - infinite)")
  //   (opt::MsgRate.data(),           bpo::value<int>()->default_value(100),          "Message rate limit in maximum number of messages per second")
  //   (opt::MsgSize.data(),           bpo::value<int>()->default_value(1024),         "Message size in bytes")
  //   (opt::HBFRate.data(),           bpo::value<int>()->default_value(1),            "Heartbeat frame (HBF) rate. 1 HBF per N messages")
  //   (opt::HBFPosition.data(),       bpo::value<int>()->default_value(0),            "Heartbeat frame (HBF) position.")
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::HulStrTdcSampler;
}
