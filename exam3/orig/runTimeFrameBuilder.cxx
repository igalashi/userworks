#include <runFairMQDevice.h>
#include "TimeFrameBuilder.h"

namespace bpo = boost::program_options;

//______________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::TimeFrameBuilder::OptionKey;
  options.add_options()
    (opt::NumSource.data(),         bpo::value<int>()->default_value(1),             "Number of source endpoint")
    (opt::BufferTimeoutInMs.data(), bpo::value<int>()->default_value(100000),        "Buffer timeout in milliseconds")
    (opt::InputChannelName.data(),  bpo::value<std::string>()->default_value("in"),  "Name of the input channel")
    (opt::OutputChannelName.data(), bpo::value<std::string>()->default_value("out"), "Name of the output channel")
    ;
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::TimeFrameBuilder;
}
