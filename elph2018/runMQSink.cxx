#include <runFairMQDevice.h>


#include "MQSink.h"

namespace bpo = boost::program_options;

//______________________________________________________________________________
void
addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::MQSink::OptionKey;
  options.add_options()
    (opt::NumMessages.data(),      bpo::value<uint64_t>()->default_value(1),       "Number of messages")
    (opt::InputChannelName.data(), bpo::value<std::string>()->default_value("in"), "Name of the input channel")
    ;
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::MQSink;
}
