#include "runFairMQDevice.h"
#include "HulStrTdcFilter.h"

namespace bpo = boost::program_options;
//______________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::HulStrTdcFilter::OptionKey;
  options.add_options()
    (opt::InputChannelName.data(),  bpo::value<std::string>()->default_value("in"),  "Name of input channel")
    (opt::OutputChannelName.data(), bpo::value<std::string>()->default_value("out"), "Name of output channel")
    ;
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&) 
{
  return new highp::e50::HulStrTdcFilter;
}

