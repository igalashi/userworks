
#include <FairMQLogger.h>

#include <THttpServer.h>
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <TMultiGraph.h>

#include "DqmUnit.h"
namespace e16::daq {
//_____________________________________________________________________________
void DqmUnit::Clear() {}

//_____________________________________________________________________________
void DqmUnit::Initialize(THttpServer &serv, std::string_view path, std::string_view name)
{
   LOG(debug) << __FUNCTION__ << " path = " << path << ", name = " << name;
}

//_____________________________________________________________________________
void DqmUnit::Unpack() {}

} // namespace e16::daq