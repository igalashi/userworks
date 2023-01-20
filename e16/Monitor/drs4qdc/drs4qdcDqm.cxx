#include <iostream>

#include "unpacker/drs4qdc/Unpacker.h"
#include "utility/filesystem.h"
#include "mq/Monitor/Dqm.h"

namespace bpo = boost::program_options;

namespace e16::daq {

//_____________________________________________________________________________
void Dqm::AddOptions(bpo::options_description &options) {}

//_____________________________________________________________________________
bool Dqm::ConditionalRun()
{
   return true;
}

//_____________________________________________________________________________
void Dqm::InitDqm() {}

//_____________________________________________________________________________
void Dqm::ProcessData(const char *buffer, std::size_t nbytes) {}

//_____________________________________________________________________________
void Dqm::SetRunNumber() {}

} // namespace e16::daq