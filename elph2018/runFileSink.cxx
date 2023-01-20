#include <runFairMQDevice.h>
#include "FileSink.h"

namespace bpo = boost::program_options;

//_______________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
  using opt = highp::e50::FileSink::OptionKey;
  using ow  = highp::e50::FileSink::OverwriteOption;
  options.add_options()
    (opt::RunId.data(),            bpo::value<int>()->default_value(-1),                       "Run ID")
    (opt::BufferSize.data(),       bpo::value<int>()->default_value(1),                        "Buffer size in MB")
    (opt::FilePath.data(),         bpo::value<std::string>(),                                  "File path")
    (opt::FileType.data(),         bpo::value<std::string>(),                                  "File type")
    (opt::SubId.data(),            bpo::value<std::string>()->default_value(""),               "Sub name to identify this FileSink. This string is appended to filename")
    (opt::Overwrite.data(),        bpo::value<std::string>()->default_value(ow::Auto.data()),  "Overwrite policy for an existing file with same name")
    (opt::InputChannelName.data(), bpo::value<std::string>()->default_value("in"),             "Name of the input channel")
    ;
}

//_______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions&)
{
  return new highp::e50::FileSink;
}
