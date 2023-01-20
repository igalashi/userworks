#include <runFairMQDevice.h>
#include "TimeStampInserter.h"

namespace bpo = boost::program_options;

//______________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::TimeStampInserter::OptionKey;
  options.add_options()
    (opt::InputChannelName.data(),  bpo::value<std::string>()->default_value("in"),  "Name of the input channel")
    (opt::OutputChannelName.data(), bpo::value<std::string>()->default_value("out"), "Name of the output channel")
    ;

}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::TimeStampInserter;
}

