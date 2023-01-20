#ifndef HulStrTdcFilter_h
#define HulStrTdcFilter_h

#include <string>
#include <string_view>
#include <vector>

#include <FairMQDevice.h>

// to emulate HUL data structure
//Head(4) | RSV(1) | TOT(8) | Type(2) | Ch(6) | TDC(19)                                                                                              
// RAM: [3][2][1][0]: on RAM: head:bit40~bit36 --> data[4]&0xF0
// HUL data flow to RAM: original data --> FPGA swaped --> network swaped --> orignal data
typedef struct {
  // from lower to higher bit                                                                                                                        
  uint64_t tdc  : 19;
  uint64_t ch   : 6;
  uint64_t type : 2;
  uint64_t tot  : 8;
  uint64_t rsv  : 1;
  uint64_t head : 4;
} hul_bit_map_t;
typedef struct {
  uint8_t data[5];
} hul_raw_word_t;
typedef struct {
    uint16_t ch;
    uint8_t  layer;
    uint8_t  tot;
    uint64_t time_stamp;
} hul_decode_word_t;

namespace highp::e50 {

class HulStrTdcFilter : public FairMQDevice
{
public:
  struct OptionKey {
    static constexpr std::string_view InputChannelName  {"in-chan-name"};
    static constexpr std::string_view OutputChannelName {"out-chan-name"};
  };

  HulStrTdcFilter();
  HulStrTdcFilter(const HulStrTdcFilter&)            = delete;
  HulStrTdcFilter& operator=(const HulStrTdcFilter&) = delete;
  ~HulStrTdcFilter() = default;

private:
  std::vector< hul_decode_word_t > ApplyFilter(const std::vector<std::vector<char>>& inputData);
  bool HandleData(FairMQParts& parts, int index);
  void Init() override;
  void InitTask() override;

  std::string fInputChannelName;
  std::string fOutputChannelName;

};

} // namespace highp::e50


#endif
