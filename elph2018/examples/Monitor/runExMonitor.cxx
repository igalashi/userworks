#include "runFairMQDevice.h"
#include "ExMonitor.h"

namespace bpo = boost::program_options;
//______________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::ExMonitor::OptionKey;
  options.add_options()
    (opt::InputChannelName.data(),  bpo::value<std::string>()->default_value("in"),         "Name of input channel")
    (opt::Http.data(),              bpo::value<std::string>()->default_value("http:8888"),  "http engine and port")
    ;
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&) 
{
  return new highp::e50::ExMonitor;
}

