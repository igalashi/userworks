#include <stdio.h>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <functional>
#include <thread>
#include <cassert>
#include <numeric>
#include <unordered_map>
#include <sstream>

#include "utility/HexDump.h"

#include "MessageUtil.h"
#include "Reporter.h"
#include "HulStrTdcEmulator.h"

// y.ma 20181210
// to emulate HUL data structure
typedef struct {
  // from lower to higher bit
  uint64_t tdc  : 19;
  uint64_t ch   : 6;
  uint64_t type : 2;
  uint64_t tot  : 8;
  uint64_t rsv  : 1;
  uint64_t head : 4;
} hul_bit_t;
typedef struct {
  uint8_t data[5];
} hul_data_word_t;
//Head(4) | RSV(1) | TOT(8) | Type(2) | Ch(6) | TDC(19)
// RAM: [3][2][1][0]
// HUL: original data --> FPGA swaped --> network swaped --> orignal data
// on RAM: head:bit40~bit36 --> data[4]&0xF0
uint32_t head = 0; 
uint32_t rsv = 0; 
uint32_t tot = 0;
uint32_t type = 0;
uint32_t ch = 0;
uint32_t tdc = 0;
uint8_t hul_data_word[5] = {0};
void assign_hul_data_word(uint8_t hul_data_word[5], uint32_t head, uint32_t rsv, 
			  uint32_t tot, uint32_t type, uint32_t ch, uint32_t tdc)
{
  hul_data_word[4] |= (head & 0xF) << 4;
  hul_data_word[4] |= (rsv & 0x1) << 3;
  hul_data_word[4] |= (tot >> 5) & 0x7;
  // std::cout << "assign_hul_data_word:" << std::endl;
  // std::cout << "(head & 0xF) << 4 = " << std::dec << std::bitset< 8 > ( ( head & 0xF ) << 4 ) << std::endl;
  // std::cout << "hul_data_word[4] = " << std::dec << std::bitset< 8 > ( hul_data_word[4] ) << std::endl;
  hul_data_word[3] |= (tot & 0x1F) << 3;
  hul_data_word[3] |= (type & 0x3) << 1;
  hul_data_word[3] |= (ch >> 5) & 0x1;
  //  std::cout << "hul_data_word[3] = " << std::dec << std::bitset< 8 > ( hul_data_word[3] ) << std::endl;
  hul_data_word[2] |= (ch & 0x1F) << 3;
  hul_data_word[2] |= (tdc >> 16) & 0x7;
  //  std::cout << "hul_data_word[2] = " << std::dec << std::bitset< 8 > ( hul_data_word[2] ) << std::endl;
  hul_data_word[1] |= (tdc >> 8) & 0xFF;
  //  std::cout << "hul_data_word[1] = " << std::dec << std::bitset< 8 > ( hul_data_word[1] ) << std::endl;
  hul_data_word[0] |= (tdc >> 0) & 0xFF;
  // std::cout << "hul_data_word[0] = " << std::dec << std::bitset< 8 > ( hul_data_word[0] ) << std::endl;
  // std::cout << "hul_data_word = " << std::dec << std::bitset< 64 > ( *((uint64_t*) hul_data_word) ) << std::endl;
  //std::cout << "hul_data_word[4] = " << std::dec << unsigned( hul_data_word[4] ) << std::endl;
}


//______________________________________________________________________________
highp::e50::
HulStrTdcEmulator::HulStrTdcEmulator()
  : FairMQDevice()
{}

//______________________________________________________________________________
bool
highp::e50::
HulStrTdcEmulator::ConditionalRun()
{
  using namespace std::chrono_literals;

  fPoller->Poll(100);

  // check availability of remote endpoint
  if (fPoller->CheckOutput(fOutputChannelName, 0)) {
    fHBFId = 0;
    std::unique_ptr<std::vector<uint8_t>> body{std::make_unique<std::vector<uint8_t>>(fMsgSize)};
    if ((fHBFRate >0) && (fNumIterations % fHBFRate == 0)) {

      int hul_data_counter = 0;
      int maxWord = fMsgSize/sizeof(hul_data_word) - 1;

      int ntdc = fMsgSize/fNumHBF/sizeof(hul_data_word);

      std::cout << " max word = " << maxWord << " ntdc = " << ntdc << std::endl;

      for (int ihbf=0; ihbf<fNumHBF; ++ihbf) {
        std::cout << "ihbf = " << ihbf << " / " << fNumHBF << " hul_data_counter = " << hul_data_counter << std::endl;
        if (hul_data_counter >= maxWord) break;

        //fill heart beat data
        head = 0xF; 
        rsv = 0; 
        tot = 0;
        type = 0;
        ch = 0;
        tdc = fHBFId; // last 16 bits used as heart beat counter
        memset( hul_data_word, 0, sizeof(hul_data_word_t) );
        assign_hul_data_word(hul_data_word, head, rsv, tot, type, ch, tdc);
        for (int jj = 0; jj < 5; jj++ )
          {
            *( reinterpret_cast<uint8_t*>(body->data() + fHBFPosition + sizeof(hul_data_word)*hul_data_counter + jj ) ) = hul_data_word[jj];
          }
        hul_data_counter++;
        
        // fill tdc data
        for (int itdc=0; itdc<ntdc; ++ itdc) {
          if (hul_data_counter >= maxWord) break;

          head = 0xD; 
          rsv = 0; 
          tot = 8;
          type = 0b00;
          ch = 16;
          // tdc = 0x666;
          auto ttt = itdc%16;
          tdc = (ttt)<<8 | (ttt)<<4 | ttt;
          memset( hul_data_word, 0, sizeof(hul_data_word_t) );
          assign_hul_data_word(hul_data_word, head, rsv, tot, type, ch, tdc);
          for (int jj = 0; jj < 5; jj++ )
            {
              *( reinterpret_cast<uint8_t*>(body->data() + fHBFPosition + sizeof(hul_data_word)*hul_data_counter + jj ) ) = hul_data_word[jj];
            }
          hul_data_counter++;
        }
        ++fHBFId;
      }
      // // fill tdc data
      // head = 0xD; 
      // rsv = 0; 
      // tot = 8;
      // type = 0b00;
      // ch = 16;
      // tdc = 0x777;
      // memset( hul_data_word, 0, sizeof(hul_data_word_t) );
      // assign_hul_data_word(hul_data_word, head, rsv, tot, type, ch, tdc);
      // for (int jj = 0; jj < 5; jj++ )
      //   {
      //     *( reinterpret_cast<uint8_t*>(body->data() + fHBFPosition + sizeof(hul_data_word)*hul_data_counter + jj ) ) = hul_data_word[jj];
      //   }
      // hul_data_counter++;
      // // fill tdc data
      // head = 0xD; 
      // rsv = 0; 
      // tot = 8;
      // type = 0b00;
      // ch = 16;
      // tdc = 0x888;
      // memset( hul_data_word, 0, sizeof(hul_data_word_t) );
      // assign_hul_data_word(hul_data_word, head, rsv, tot, type, ch, tdc);
      // for (int jj = 0; jj < 5; jj++ )
      //   {
      //     *( reinterpret_cast<uint8_t*>(body->data() + fHBFPosition + sizeof(hul_data_word)*hul_data_counter + jj ) ) = hul_data_word[jj];
      //   }
      // hul_data_counter++;
      // // fill tdc data
      // head = 0xD; 
      // rsv = 0; 
      // tot = 8;
      // type = 0b00;
      // ch = 16;
      // tdc = 0x999;
      // memset( hul_data_word, 0, sizeof(hul_data_word_t) );
      // assign_hul_data_word(hul_data_word, head, rsv, tot, type, ch, tdc);
      // for (int jj = 0; jj < 5; jj++ )
      //   {
      //     *( reinterpret_cast<uint8_t*>(body->data() + fHBFPosition + sizeof(hul_data_word)*hul_data_counter + jj ) ) = hul_data_word[jj];
      //   }
      // hul_data_counter++;

      // fill spill end data
      head = 0x4; 
      rsv = 0; 
      tot = 0;
      type = 0;
      ch = 0;
      tdc = 0xFFFF;
      memset( hul_data_word, 0, sizeof(hul_data_word_t) );
      assign_hul_data_word(hul_data_word, head, rsv, tot, type, ch, tdc);
      for (int jj = 0; jj < 5; jj++ )
	{
	  *( reinterpret_cast<uint8_t*>(body->data() + fHBFPosition + sizeof(hul_data_word)*hul_data_counter + jj ) ) = hul_data_word[jj];
	}
      hul_data_counter++;

    }

    FairMQMessagePtr msg(MessageUtil::NewMessage(*this, std::move(body)));

    std::for_each(reinterpret_cast<hul_data_word_t*>(msg->GetData()), 
                  reinterpret_cast<hul_data_word_t*>(msg->GetData())+(msg->GetSize())/sizeof(hul_data_word_t),
                  HexDump{4});
    // y.ma 20181210
    std::this_thread::sleep_for(1000ms);
    
    
    Reporter::AddOutputMessageSize(msg->GetSize());
    // push multipart message into send queue
    if (Send(msg, fOutputChannelName) > 0) {
      ++fNumIterations;
      //if (!CheckCurrentState(RUNNING)) {
      if (GetCurrentState() != fair::mq::State::Running) {
        LOG(INFO) << " Device is not RUNNING";
        return false;
      }
      else if ((fMaxIterations >0) && (fNumIterations >= fMaxIterations)) {
        LOG(INFO) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
      }
    } 
    else {
      LOG(WARNING) << "failed to send a message.";
    }

    --fMsgCounter;
  }
  else {
    LOG(INFO) << "output channel looks busy or missing ... ";
    std::this_thread::yield();
    std::this_thread::sleep_for(1s);
  }

  while (fMsgCounter == 0) {
    std::this_thread::yield();
    std::this_thread::sleep_for(1us);
    //LOG(INFO) << "main thread waiting for reset of message counter";
  }


  return true;
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcEmulator::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcEmulator::InitTask()
{
  using opt     = OptionKey;
  fMaxIterations     = fConfig->GetValue<uint64_t>(opt::MaxIterations.data());
  fMsgRate           = fConfig->GetValue<int>(opt::MsgRate.data());
  fMsgSize           = fConfig->GetValue<int>(opt::MsgSize.data());
  fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());
  fHBFRate           = fConfig->GetValue<int>(opt::HBFRate.data());
  fHBFPosition       = fConfig->GetValue<int>(opt::HBFPosition.data());
  fNumHBF            = fConfig->GetValue<int>(opt::NumHBF.data());

  if (fMsgRate<1) {
    auto errMsg = "invalid output message rate " + std::to_string(fMsgRate) + " Hz";
    LOG(ERROR) << errMsg;
    throw std::runtime_error(errMsg);
  }

  LOG(INFO) << "message rate = " << fMsgRate << " /sec, "
            << " message size = " << fMsgSize << " byte";

  std::cout << "HBF pos = " << fHBFPosition << std::endl;
  std::cout << "out-ch-name = " << fOutputChannelName << std::endl;
  

  fNumIterations = 0;
  fMsgCounter    = 0;
  fHBFId         = 0;

  fPoller = std::move(NewPoller(fOutputChannelName));
  
  Reporter::Reset();
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcEmulator::PreRun()
{
  fResetMsgCounter = std::thread(&HulStrTdcEmulator::ResetMsgCounter, this);
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcEmulator::PostRun()
{
  fResetMsgCounter.join();
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcEmulator::ResetMsgCounter()
{
  using namespace std::chrono_literals;
  //while (CheckCurrentState(RUNNING)) {
  while (GetCurrentState() == fair::mq::State::Running) {
    //LOG(INFO) << "reset message counter thread";
    if (fMsgRate>=100) {
      //fMsgCounter = fMsgRate / 100;
      std::this_thread::sleep_for(10ms);
    } 
    else if (fMsgRate>=10) {
      //fMsgCounter = fMsgRate / 10;
      std::this_thread::sleep_for(100ms);
    }
    else {
      //fMsgCounter = fMsgRate;
      std::this_thread::sleep_for(1000ms);
    }
  }
  //fMsgCounter = -1;
}

