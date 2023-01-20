#include "runFairMQDevice.h"
#include "Sink.h"

//_______________________________________________________________________________
void addCustomOptions(boost::program_options::options_description& options)
{
  options.add_options()
    ("max-iterations", boost::program_options::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditinalRun/OnData (0 - infinite)")
    ;
}

//_______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::Sink;
}
