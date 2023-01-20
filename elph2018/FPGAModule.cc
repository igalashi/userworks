#include"FPGAModule.hh"
#include"UDPRBCP.hh"
#include<iostream>

// Constructor/Destructor ------------------------------------------------------
FPGAModule::FPGAModule(const char* ipAddr, unsigned int port, rbcp_header* sendHeader,
		       int disp_mode)
  :
  ipAddr_(ipAddr),
  port_(port),
  sendHeader_(sendHeader),
  disp_mode_(disp_mode)
{

}

FPGAModule::~FPGAModule()
{

}

// WriteModule ----------------------------------------------------------------
int
FPGAModule::WriteModule(unsigned int register_address,
			unsigned int write_data,
			int nCycle
			)
{
  if(nCycle > 4){
    std::cerr << "#E :FPGAModule::WriteModule, too many cycle " 
	      << nCycle << std::endl;
    return -1;
  }

  unsigned int udp_addr = register_address;
  int status = 0;
  for(int i = 0; i<nCycle; ++i){
    char udp_wd = static_cast<char>((write_data >> 8*i)& data_mask);

    UDPRBCP udpMan(ipAddr_, port_, sendHeader_,
		   static_cast<UDPRBCP::rbcp_debug_mode>(disp_mode_));
    udpMan.SetWD(udp_addr+i, 1, &udp_wd);
    

    if((status = udpMan.DoRBCP()) < 0){
      std::cerr << "#E :FPGAModule::WriteModule, write fail " 
		<< status << std::endl;
      break;
    }
  }

  return status;
}

// ReadModule ----------------------------------------------------------------
unsigned int
FPGAModule::ReadModule(unsigned int register_address,
		       int nCycle
		       )
{
  if(nCycle > 4){
    std::cerr << "#E :FPGAModule::ReadModule, too many cycle " 
	      << nCycle << std::endl;
    return 0xeeeeeeee;
  }

  unsigned int data = 0;
  for(int i = 0; i<nCycle; ++i){
    if( this->ReadModule_nByte(register_address+i, 1) > -1){
      unsigned int tmp = (unsigned int)rd_data_[0];
      data += (tmp & data_mask) << 8*i;
    }else{
      return 0xeeeeeeee;
    }
  }

  rd_word_ = data;
  return rd_word_;
}

// ReadModule_nByte ------------------------------------------------------------
int
FPGAModule::ReadModule_nByte(unsigned int register_address,
			     unsigned int length
			     )
{
  rd_data_.clear();
  unsigned int udp_addr = register_address;

  UDPRBCP udpMan(ipAddr_, port_, sendHeader_,
		 static_cast<UDPRBCP::rbcp_debug_mode>(disp_mode_));
  udpMan.SetRD(udp_addr, length);
  int ret;
  if((ret = udpMan.DoRBCP()) > -1){ udpMan.CopyRD(rd_data_); }

  return ret;
}
