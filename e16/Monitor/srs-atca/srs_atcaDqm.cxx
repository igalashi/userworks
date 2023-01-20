#include <iostream>

#include "mq/Monitor/Dqm.h"
#include "mq/Monitor/srs-atca/DqmUnit.h"

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
