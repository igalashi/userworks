#ifndef REGISTERH_
#define REGISTERH_

namespace hul_strtdc{
//-------------------------------------------------------------------------
// DCT Module
//-------------------------------------------------------------------------
namespace DCT{
  enum RegisterAddress{
    addr_gate       = 0x00000000, // W/R, [0:0] Set DAQ Gate reg
    addr_evb_reset  = 0x00000010, // W,   Assert EVB reset (self counter reset)
    addr_status     = 0x00000020  // R,   BUSY
  };
};

//-------------------------------------------------------------------------
// ODP Module
//-------------------------------------------------------------------------
namespace ODP{
  enum RegisterAddress{
    addr_en_filter      = 0x10000000, // W/R, [0:0] Enable/Disable TOT filter
    addr_min_th         = 0x10000010, // W,R, [7:0] TOT filter Min. Th.
    addr_max_th         = 0x10000020, // W,R, [7:0] TOT filter Max. Th.
    addr_en_zerothrough = 0x10000030,  // W,R, [7:0] Timewalk corr .
    addr_tw_corr0       = 0x10000040, // W,R, [7:0] Timewalk corr. 
    addr_tw_corr1       = 0x10000050, // W,R, [7:0] Timewalk corr. 
    addr_tw_corr2       = 0x10000060, // W,R, [7:0] Timewalk corr. 
    addr_tw_corr3       = 0x10000070, // W,R, [7:0] Timewalk corr. 
    addr_tw_corr4       = 0x10000080  // W,R, [7:0] Timewalk corr. 
  };
};

// BCT --------------------------------------------------------------------
namespace BCT{
  enum RegisterAddress{
    addr_Reset   = 0xe0000000, // W, Assert module reset signal
    addr_Version = 0xe0000010, // R, [31:0]
    addr_ReConfig= 0xe0000020  // W, Reconfig FPGA by SPI
  };
};
};
#endif
