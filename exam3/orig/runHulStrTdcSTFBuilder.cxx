#include <runFairMQDevice.h>
#include "HulStrTdcSTFBuilder.h"

namespace bpo = boost::program_options;

//______________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::HulStrTdcSTFBuilder::OptionKey;
  options.add_options()
    (opt::FEMId.data(),             bpo::value<std::string>(),                       "FEM ID")
    (opt::InputChannelName.data(),  bpo::value<std::string>()->default_value("in"),  "Name of the input channel")
    (opt::OutputChannelName.data(), bpo::value<std::string>()->default_value("out"), "Name of the output channel")
    (opt::DQMChannelName.data(),    bpo::value<std::string>()->default_value("dqm"), "Name of the data quality monitoring")
    (opt::MaxHBF.data(),            bpo::value<int>()->default_value(1),             "maximum number of heartbeat frame in one sub time frame")
    (opt::SplitMethod.data(),       bpo::value<int>()->default_value(0),             "STF split method")
    ;
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::HulStrTdcSTFBuilder;
}
