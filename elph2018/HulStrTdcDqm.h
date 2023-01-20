#ifndef HulStrTdcDqm_h
#define HulStrTdcDqm_h

#include <chrono>
#include <string>
#include <string_view>
#include <memory>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <FairMQDevice.h>

#include <THttpServer.h>

#include "HulStrTdcData.h"

class TH1; 

namespace highp::e50 {


class HulStrTdcDqm : public FairMQDevice
{
public:
  struct OptionKey {
    static constexpr std::string_view NumSource         {"num-source"};
    static constexpr std::string_view BufferTimeoutInMs {"buffer-timeout"};
    static constexpr std::string_view InputChannelName  {"in-chan-name"};
    static constexpr std::string_view Http              {"http"};
    static constexpr std::string_view UpdateInterval    {"update-ms"};
  };

  struct STFBuffer {
    FairMQParts parts;
    std::chrono::steady_clock::time_point start;
  };

  enum Status {
    OK, 
    Mismatch, 
    Discarded
  };

  HulStrTdcDqm();
  HulStrTdcDqm(const HulStrTdcDqm&)            = delete;
  HulStrTdcDqm& operator=(const HulStrTdcDqm&) = delete;
  ~HulStrTdcDqm() = default;

private:
  void Check(std::vector<STFBuffer>&& stfs);
  bool HandleData(FairMQParts& parts, int index);
  void Init() override;
  void InitServer(std::string_view server);
  void InitTask() override;
  void PostRun() override;

  int fNumSource {1};
  int fBufferTimeoutInMs {100000};
  std::string fInputChannelName;
  std::unique_ptr<THttpServer> fServer;
  std::map<uint64_t, int> fFEMId;
  std::vector<uint16_t> fHbc;
  std::unordered_map<std::string, TH1*> fH1Map;
  int fUpdateIntervalInMs {1000};
  std::chrono::steady_clock::time_point fPrevUpdate;

  std::unordered_map<uint32_t, std::vector<STFBuffer>> fTFBuffer;
  std::unordered_set<uint64_t> fDiscarded;
};
  
} // namespace highp::e50

#endif
