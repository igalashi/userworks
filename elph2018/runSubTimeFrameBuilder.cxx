#include <runFairMQDevice.h>
#include "SubTimeFrameBuilder.h"

namespace bpo = boost::program_options;

//______________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::SubTimeFrameBuilder::OptionKey;
  options.add_options()
    (opt::InputChannelName.data(),  bpo::value<std::string>()->default_value("in"),  "Name of the input channel")
    (opt::OutputChannelName.data(), bpo::value<std::string>()->default_value("out"), "Name of the output channel")
    (opt::MaxHBF.data(),            bpo::value<int>()->default_value(1),             "Maximum number of heart beat frame in one sub time frame")
    ;
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::SubTimeFrameBuilder;
}
