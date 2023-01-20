#include <vector>
#include <chrono>
#include <algorithm>
#include <iostream>

#include "utility/HexDump.h"

#include "RAWHeader.h"
#include "HeartbeatFrameHeader.h"
#include "MessageUtil.h"
#include "Reporter.h"
#include "BenchmarkSampler.h"

//______________________________________________________________________________
highp::e50::
BenchmarkSampler::BenchmarkSampler()
  : FairMQDevice()
{}

//______________________________________________________________________________
bool
highp::e50::
BenchmarkSampler::ConditionalRun()
{
  namespace HBF = HeartbeatFrame;
  using namespace std::chrono_literals;

  fPoller->Poll(100);

  // check availability of remote endpoint
  if (fPoller->CheckOutput(fOutputChannelName, 0)) {
    auto header   = std::make_unique<RAW::Header>(); 
    header->word1 = fFEMId;
    header->word2 = fNumIterations;
    // std::cout << " header " << std::endl;
    // std::for_each(reinterpret_cast<uint64_t*>(header.get()), 
    //               reinterpret_cast<uint64_t*>(header.get())+sizeof(RAW::Header)/sizeof(uint64_t),
    //               HexDump{4});

    FairMQParts parts;
    parts.AddPart(MessageUtil::NewMessage(*this, std::move(header)));
    // std::cout << " header (messageparts)" << std::endl;
    // std::for_each(reinterpret_cast<uint64_t*>(parts.At(0)->GetData()), 
    //               reinterpret_cast<uint64_t*>(parts.At(0)->GetData())+(parts.At(0)->GetSize())/sizeof(uint64_t),
    //               HexDump{4});


    std::unique_ptr<std::vector<uint8_t>> body{std::make_unique<std::vector<uint8_t>>(fMsgSize)};
    if ((fHBFRate >0) && (fNumIterations % fHBFRate == 0)) {
      auto h     = reinterpret_cast<HBF::Header*>(body->data() + fHBFPosition);
      h->magic   = HBF::Magic;
      h->frameId = fHBFId;
      auto t     = std::chrono::steady_clock::now().time_since_epoch();
      h->time    = std::chrono::duration_cast<std::chrono::nanoseconds>(t).count();
      ++fHBFId;
    }
    parts.AddPart(MessageUtil::NewMessage(*this, std::move(body)));

    // std::for_each(reinterpret_cast<uint64_t*>(parts.At(1)->GetData()), 
    //               reinterpret_cast<uint64_t*>(parts.At(1)->GetData())+(parts.At(1)->GetSize())/sizeof(uint64_t),
    //               HexDump{4});
    
    Reporter::AddOutputMessageSize(parts);

    // push multipart message into send queue
    if (Send(parts, fOutputChannelName) > 0) {
      ++fNumIterations;
      //if (!CheckCurrentState(RUNNING)) {
      if (GetCurrentState() != fair::mq::State::Running) {
        LOG(info) << "Device is not RUNNING";
        return false;
      }
      else if ((fMaxIterations >0) && (fNumIterations >= fMaxIterations)) {
        LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
      }
    } 
    else {
      LOG(warn) << "failed to send a message.";
    }

    --fMsgCounter;
  }
  else {
    LOG(info) << "output channel looks busy or missing ... ";
    std::this_thread::yield();
    std::this_thread::sleep_for(1s);
  }

  while (fMsgCounter == 0) {
    std::this_thread::yield();
    std::this_thread::sleep_for(1us);
    //LOG(info) << "main thread waiting for reset of message counter";
  }


  return true;
}

//______________________________________________________________________________
void
highp::e50::
BenchmarkSampler::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
BenchmarkSampler::InitTask()
{
  using opt     = OptionKey;
  namespace HBF = HeartbeatFrame;
  fFEMId             = std::stoll(fConfig->GetValue<std::string>(opt::FEMId.data()), nullptr, 0);
  fMaxIterations     = fConfig->GetValue<uint64_t>(opt::MaxIterations.data());
  fMsgRate           = fConfig->GetValue<int>(opt::MsgRate.data());
  fMsgSize           = fConfig->GetValue<int>(opt::MsgSize.data());
  fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());
  fHBFRate           = fConfig->GetValue<int>(opt::HBFRate.data());
  fHBFPosition       = fConfig->GetValue<int>(opt::HBFPosition.data());

  if (fMsgRate<1) {
    auto errMsg = "invalid output message rate " + std::to_string(fMsgRate) + " Hz";
    LOG(error) << errMsg;
    throw std::runtime_error(errMsg);
  }

  if (fMsgSize < sizeof(HBF::Header)) {
    fMsgSize = sizeof(HBF::Header);
  }
  LOG(info) << "message rate = " << fMsgRate << " /sec, "
            << " message size = " << fMsgSize << " byte";

  if (fHBFPosition+sizeof(HBF::Header) > fMsgSize) {
    fHBFPosition = fMsgSize - sizeof(HBF::Header);
  }
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
BenchmarkSampler::PreRun()
{
  fResetMsgCounter = std::thread(&BenchmarkSampler::ResetMsgCounter, this);
}

//______________________________________________________________________________
void
highp::e50::
BenchmarkSampler::PostRun()
{
  fResetMsgCounter.join();
}

//______________________________________________________________________________

void
highp::e50::
BenchmarkSampler::ResetMsgCounter()
{
  using namespace std::chrono_literals;
  //while (CheckCurrentState(RUNNING)) {
  while (GetCurrentState() == fair::mq::State::Running) {
    //LOG(info) << "reset message counter thread";
    if (fMsgRate>=100) {
//      fMsgCounter = fMsgRate / 100;
      std::this_thread::sleep_for(10ms);
    } 
    else if (fMsgRate>=10) {
//      fMsgCounter = fMsgRate / 10;
      std::this_thread::sleep_for(100ms);
    }
    else {
//      fMsgCounter = fMsgRate;
      std::this_thread::sleep_for(1000ms);
    }
  }
  fMsgCounter = -1;
}

