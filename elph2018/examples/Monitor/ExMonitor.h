#ifndef ExMonitor_h
#define ExMonitor_h

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <FairMQDevice.h>

#include <THttpServer.h>

namespace highp::e50 {

class ExMonitor : public FairMQDevice
{
public:
  struct OptionKey {
    static constexpr std::string_view InputChannelName  {"in-chan-name"};
    static constexpr std::string_view Http              {"http"};
  };

  ExMonitor();
  ExMonitor(const ExMonitor&)            = delete;
  ExMonitor& operator=(const ExMonitor&) = delete;
  ~ExMonitor() = default;

private:
  void Decode(std::vector<std::vector<char>>&& inputData);
  bool HandleData(FairMQParts& parts, int index);
  void Init() override;
  void Initialize(std::string_view server, 
                  std::string_view id);
  void InitTask() override;
  
  std::string fInputChannelName;
  std::unique_ptr<THttpServer> fServer;

};

} // namespace highp::e50


#endif
