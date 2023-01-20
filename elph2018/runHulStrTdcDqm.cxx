#include <runFairMQDevice.h>

#include "HulStrTdcDqm.h"

namespace bpo = boost::program_options;

//______________________________________________________________________________
void
addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::HulStrTdcDqm::OptionKey;
  options.add_options()
    (opt::NumSource.data(),          bpo::value<int>()->default_value(1),                                  "Number of source endpoint")
    (opt::BufferTimeoutInMs.data(),  bpo::value<int>()->default_value(100000),                             "Buffer timeout in milliseconds")
    (opt::InputChannelName.data(),   bpo::value<std::string>()->default_value("in"),                       "Name of the input channel")
    (opt::Http.data(),               bpo::value<std::string>()->default_value("http:8888?monitoring=500"), "http engine and port, etc.")
    (opt::UpdateInterval.data(),     bpo::value<int>()->default_value(1000),                               "Canvas update rate in milliseconds")
    ;
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::HulStrTdcDqm;
}
