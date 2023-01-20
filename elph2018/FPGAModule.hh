#ifndef FPGAMODULE_
#define FPGAMODULE_

#include<vector>

#include"Uncopyable.hh"

//static const unsigned int module_id_mask  = 0xF;
//static const unsigned int module_id_shift = 28U; // <<

//static const unsigned int address_mask    = 0xFFF;
//static const unsigned int address_shift   = 16U;  // <<

//static const unsigned int exdata_mask     = 0xFFFF00;
//static const unsigned int exdata_shift    = 8U;  // >>

static const unsigned int data_mask       = 0xFF;

struct rbcp_header;

class FPGAModule
  : Uncopyable<FPGAModule>
{
public:
  typedef std::vector<unsigned char> dType;
  typedef dType::const_iterator      dcItr;
  
private:

  // RBCP data structure
  // RBCP_ADDR [31:0]
  // RBCP_WD   [7:0]
  //
  // Module ID     : RBCP_ADDR[31:28]
  // Not in use    : RBCP_ADDR[27:12]
  // Local address : RBCP_ADDR[11:0]
  // original data : RBCP_WD[7:0]

  const char*  ipAddr_;
  unsigned int port_;
  rbcp_header* sendHeader_;
  int          disp_mode_;

  dType        rd_data_;
  unsigned int rd_word_;

public:
  FPGAModule(const char* ipAddr, unsigned int port, rbcp_header* sendHeader,
	     int disp_mode = 1);
  virtual ~FPGAModule();

  // n cycle write rbcp by inlrementing laddr
  int WriteModule(unsigned int register_address,
		  unsigned int write_data,
		  int nCycle
		  );
  
  // n cycle read rbcp by inlrementing laddr
  // (data are storeod in rd_word_)
  unsigned int ReadModule(unsigned int register_address,
			  int nCycle
			  );
  
  // n byte read rbcp
  // (data are stored in rd_data_)
  int ReadModule_nByte(unsigned int register_address,
		       unsigned int nbyte
		       );

  unsigned int GetReadWord(){return rd_word_;};

  dcItr GetDataIteratorBegin(){ return rd_data_.begin(); };
  dcItr GetDataIteratorEnd(){ return rd_data_.end(); };
};

#endif
