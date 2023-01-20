#ifndef ExFilter_h
#define ExFilter_h

#include <string>
#include <string_view>
#include <vector>

#include <FairMQDevice.h>

namespace highp::e50 {

class ExFilter : public FairMQDevice
{
public:
  struct OptionKey {
    static constexpr std::string_view InputChannelName  {"in-chan-name"};
    static constexpr std::string_view OutputChannelName {"out-chan-name"};
  };


  ExFilter();
  ExFilter(const ExFilter&)            = delete;
  ExFilter& operator=(const ExFilter&) = delete;
  ~ExFilter() = default;

private:
  std::vector<std::vector<char>> ApplyFilter(const std::vector<std::vector<char>>& inputData);
  bool HandleData(FairMQParts& parts, int index);
  void Init() override;
  void InitTask() override;

  std::string fInputChannelName;
  std::string fOutputChannelName;

};

} // namespace highp::e50


#endif
