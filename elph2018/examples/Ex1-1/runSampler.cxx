#include "runFairMQDevice.h"
#include "Sampler.h"

//______________________________________________________________________________
void addCustomOptions(boost::program_options::options_description& options)
{
  options.add_options()
    ("text",           boost::program_options::value<std::string>()->default_value("Hello"), "Text to send out")
    ("max-iterations", boost::program_options::value<uint64_t>()->default_value(0),          "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)")
    ;
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::Sampler;
}
