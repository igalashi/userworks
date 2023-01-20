#ifndef HulStrTdcData_h
#define HulStrTdcData_h

#include <cstdint>

namespace highp::e50::HulStrTdc::Data {
inline
namespace v2_2 {
  struct Word {
    uint8_t d[5];
  };

  struct Bits {
    union {
      uint8_t d[5];

      struct {
        uint64_t raw : 40;
      };

      // common
      struct {
        uint64_t com_rsv : 36; // [35:0]
        uint64_t head    :  4; // [39:36]
      };
   
      // tdc
      struct { 
        uint64_t tdc     : 19; // [18:0]
        uint64_t ch      :  6; // [24:19]
        uint64_t type    :  2; // [26:25]
        uint64_t tot     :  8; // [34:27]
        uint64_t tdc_rsv :  1; // [35]
        //uint64_t head    :  4; // [39:36]
      };

      // heartbeat, error recovery, spill end
      struct { 
        uint64_t hbc    : 16; // [15:0]
        uint64_t hb_rsv : 20; // [35:16]
        //uint64_t head :  4; // [39:36]
      };
    };
  };

  enum HeadTypes {
    Data          = 0xD,
    Heartbeat     = 0xF,
    ErrorRecovery = 0xE,
    SpillEnd      = 0x4
  };

  enum DataTypes {
    LeadingEdge  = 0b00,
    BusyStart    = 0b01,
    BusyEnd      = 0b10
  };

} // v2_2


namespace v2_1 {
  enum DataTypes {
    LeadingEdge  = 0b00,
    BusyStart    = 0b01,
    BusyEnd      = 0b10, 
    TrailingEdge = 0b11
  };
} // v2_1


} // nemacspace hihgp::e50::HulStrTdc::Data

#endif
