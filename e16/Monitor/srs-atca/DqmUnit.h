#ifndef E16DAQ_SRS_ATCA_DQM_Unit_h
#define E16DAQ_SRS_ATCA_DQM_Unit_h

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <vector>

#include "unpacker/srs-atca/Unpacker.h"

class THttpServer;
class TH1;
class TH2;
class TGraph;
class TMultiGraph;

namespace srs_atca {
class Unpacker;
}

namespace e16::daq {

class DqmUnit {
public:
   struct Graphs {
      std::vector<TGraph *> fApvRaw;
      TGraph *fTimestampDelta;
   };

   struct Histograms {
      std::vector<TH2 *> fAdcRaw;
      TH1 *fTdc;
      TH1 *fTimestampDelta;
   };

   DqmUnit() = default;
   ~DqmUnit() = default;

   void Clear();
   void Initialize(THttpServer &serv, std::string_view path, std::string_view name);
   void Unpack();

private:
   std::unique_ptr<srs_atca::Unpacker> fUnpacker;
   std::vector<uint32_t> fTimestampLocal;
   std::vector<uint32_t> fTriggerCount;
   std::vector<std::vector<int16_t>> fApvRaw;
   std::vector<std::vector<int16_t>> fApvFiltered;
   std::vector<uint64_t> fTdc;
   uint64_t fTimestamp;
   uint16_t fSpillId;
   uint32_t fEventId;

   Graphs fGraphs;
   Histograms fHistograms;
};

} // namespace e16::daq

#endif