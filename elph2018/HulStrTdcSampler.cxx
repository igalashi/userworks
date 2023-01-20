#include <vector>
#include <chrono>
#include <algorithm>
#include <iostream>

#include "utility/HexDump.h"

#include "RAWHeader.h"
#include "HeartbeatFrameHeader.h"
#include "MessageUtil.h"
#include "HulStrTdcSampler.h"
#include "Reporter.h"

#include "RegisterMap.hh"
#include "network.hh"
#include "rbcp.h"
#include "UDPRBCP.hh"
#include "FPGAModule.hh"

//______________________________________________________________________________
highp::e50::
HulStrTdcSampler::HulStrTdcSampler()
  : FairMQDevice()
{}

//______________________________________________________________________________
bool
highp::e50::
HulStrTdcSampler::ConditionalRun()
{
  using namespace hul_strtdc;
  using namespace std::chrono_literals;

  int n_word = 0;
  uint8_t* buffer = new uint8_t[fnByte*fnWordPerCycle]{};

  while( -1 == ( n_word = Event_Cycle(buffer))) continue;

  if(n_word == -4){
    for(int i = 0; i<fnWordPerCycle; ++i){

      if((buffer[fnByte*i+4] & 0xff) == 0x40){
        printf("\n#D : Spill End is detected\n");
        n_word = i+1; 
        break;
      }
    }// For(i)
  }
  if (n_word<=0) return true;

  FairMQMessagePtr msg(NewMessage((char*)buffer,
                                  //fnByte*fnWordPerCycle, 
                                  fnByte*n_word,
                                  [](void* object, void*)
                                  {delete [] static_cast<uint8_t*>(object);}
                                  )
                       );

  
  //    while (Send(msg, "data") == -2);

  Reporter::AddOutputMessageSize(msg->GetSize());
  Send(msg, fOutputChannelName);

  //     ++fNumIterations;
  //     if ((fMaxIterations >0) && (fNumIterations >= fMaxIterations)) {
  // 	  LOG(INFO) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
  // 	  return false;
  //     }
  // } 
  // else {
  //     LOG(WARNING) << "failed to send a message.";
  // }

  //    LOG(WARNING) << "You should swim!";
  //    std::this_thread::sleep_for(std::chrono::seconds(1));

  return true;
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcSampler::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcSampler::InitTask()
{
  using opt     = OptionKey;
  fIpSiTCP           = fConfig->GetValue<std::string>(opt::IpSiTCP.data());
  fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data()); 

  fTotFilterEn       = fConfig->GetValue<int>(opt::TotFilterEn.data()); 
  fTotMinTh          = fConfig->GetValue<int>(opt::TotMinTh.data()); 
  fTotMaxTh          = fConfig->GetValue<int>(opt::TotMaxTh.data()); 
  fTotZeroAllow      = fConfig->GetValue<int>(opt::TotZeroAllow.data()); 
  fTWCorr0           = fConfig->GetValue<int>(opt::TWCorr0.data()); 
  fTWCorr1           = fConfig->GetValue<int>(opt::TWCorr1.data()); 
  fTWCorr2           = fConfig->GetValue<int>(opt::TWCorr2.data()); 
  fTWCorr3           = fConfig->GetValue<int>(opt::TWCorr3.data()); 
  fTWCorr4           = fConfig->GetValue<int>(opt::TWCorr4.data()); 

  LOG(INFO) << fIpSiTCP;
  LOG(INFO) << fTotFilterEn;
  LOG(INFO) << fTotMinTh;
  LOG(INFO) << fTotMaxTh;
  LOG(INFO) << fTotZeroAllow;
  LOG(INFO) << fTWCorr0;
  LOG(INFO) << fTWCorr1;
  LOG(INFO) << fTWCorr2;
  LOG(INFO) << fTWCorr3;
  LOG(INFO) << fTWCorr4;

  rbcp_header rbcpHeader;
  rbcpHeader.type = UDPRBCP::rbcp_ver_;
  rbcpHeader.id   = 0;
    
  FPGAModule fModule(fIpSiTCP.c_str(), 4660, &rbcpHeader, 0);
  LOG(INFO) << std::hex << fModule.ReadModule(hul_strtdc::BCT::addr_Version, 4) << std::dec;

  Reporter::Reset();
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcSampler::PreRun()
{
  using namespace hul_strtdc;

  if(-1 == (fHulSocket = ConnectSocket(fIpSiTCP.c_str()))) return;
  LOG(INFO) << "TCP connected";

  rbcp_header rbcpHeader;
  rbcpHeader.type = UDPRBCP::rbcp_ver_;
  rbcpHeader.id   = 0;
    
  FPGAModule fModule(fIpSiTCP.c_str(), 4660, &rbcpHeader, 0);
  fModule.WriteModule(ODP::addr_en_filter,       fTotFilterEn,  1);
  fModule.WriteModule(ODP::addr_min_th,          fTotMinTh,     1);
  fModule.WriteModule(ODP::addr_max_th,          fTotMaxTh,     1);
  fModule.WriteModule(ODP::addr_en_zerothrough,  fTotZeroAllow, 1);

  fModule.WriteModule(ODP::addr_tw_corr0,        fTWCorr0,      1);
  fModule.WriteModule(ODP::addr_tw_corr1,        fTWCorr1,      1);
  fModule.WriteModule(ODP::addr_tw_corr2,        fTWCorr2,      1);
  fModule.WriteModule(ODP::addr_tw_corr3,        fTWCorr3,      1);
  fModule.WriteModule(ODP::addr_tw_corr4,        fTWCorr4,      1);

  fModule.WriteModule(DCT::addr_gate,  1, 1);

  LOG(INFO) << "Start DAQ";
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcSampler::PostRun()
{
  using namespace hul_strtdc;

  rbcp_header rbcpHeader;
  rbcpHeader.type = UDPRBCP::rbcp_ver_;
  rbcpHeader.id   = 0;
    
  FPGAModule fModule(fIpSiTCP.c_str(), 4660, &rbcpHeader, 0);
  fModule.WriteModule(DCT::addr_gate,  0, 1);

  LOG(INFO) << "End DAQ";
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcSampler::ResetTask()
{
  int n_word = 0;
  uint8_t buffer[fnByte*fnWordPerCycle];
  while( -1 == ( n_word = Event_Cycle(buffer))) continue;

  close(fHulSocket);

  LOG(INFO) << "Socket close";

}

//______________________________________________________________________________
int
highp::e50::
HulStrTdcSampler::ConnectSocket(const char* ip)
{
  struct sockaddr_in SiTCP_ADDR;
  unsigned int port = 24;

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  SiTCP_ADDR.sin_family      = AF_INET;
  SiTCP_ADDR.sin_port        = htons((unsigned short int) port);
  SiTCP_ADDR.sin_addr.s_addr = inet_addr(ip);

  struct timeval tv;
  tv.tv_sec  = 0;
  tv.tv_usec = 250000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));

  int flag = 1;
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

  if(0 > connect(sock, (struct sockaddr*)&SiTCP_ADDR, sizeof(SiTCP_ADDR))){
    LOG(ERROR) << "TCP connection error";
    close(sock);
    return -1;
  }

  return sock;
}

// Event Cycle ------------------------------------------------------------
int
highp::e50::
HulStrTdcSampler::Event_Cycle(uint8_t* buffer)
{
  // data read ---------------------------------------------------------
  static const unsigned int sizeData = fnByte*fnWordPerCycle*sizeof(uint8_t);
  int ret = receive(fHulSocket, (char*)buffer, sizeData);
  if(ret < 0) return ret;

  return fnWordPerCycle;
}

// receive ----------------------------------------------------------------
int
highp::e50::
HulStrTdcSampler::receive(int sock, char* data_buf, unsigned int length)
{
  unsigned int revd_size = 0;
  int tmp_ret            = 0;

  while(revd_size < length){
    tmp_ret = recv(sock, data_buf + revd_size, length -revd_size, 0);

    if(tmp_ret == 0) break;
    if(tmp_ret < 0){
      int errbuf = errno;
      perror("TCP receive");
      if(errbuf == EAGAIN){
	// this is time out
	std::cout << "#D : TCP recv time out" << std::endl;
	return -4;
      }else{
	// something wrong
	std::cerr << "TCP error : " << errbuf << std::endl;
      }

      revd_size = tmp_ret;
      break;
    }

    revd_size += tmp_ret;
  }

  return revd_size;
}

