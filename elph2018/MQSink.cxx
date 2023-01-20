#include <algorithm>

#include "utility/HexDump.h"
#include "Reporter.h"

#include "MQSink.h"

//______________________________________________________________________________
highp::e50::
MQSink::MQSink()
  : FairMQDevice()
{}

//______________________________________________________________________________
bool
highp::e50::
MQSink::Count(int n)
{
  if ((fRecvCounter == 0) && (fRecvBytes == 0)) {
    fStart = std::chrono::steady_clock::now();
  }

  ++fRecvCounter;
  fRecvBytes += n;

  if (fRecvCounter % fNumMessages == 0) {
    auto tEnd = std::chrono::steady_clock::now();
    auto dt   = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - fStart);

    double msgRate = fRecvCounter;
    msgRate /= dt.count()*1e-3 * 1e6;
    double dataRate = fRecvBytes;
    dataRate /= dt.count()*1e-3 * 1e6;
    LOG(info) << "Number of received messages = " << fRecvCounter
              << ", n bytes = " << fRecvBytes
              << " in " << dt.count() << " msec.    "
              << msgRate << " MHz, "
              << dataRate << " MB/sec";

    fStart       = tEnd;
    fRecvCounter = 0;
    fRecvBytes   = 0;
  }

  return true;
}

//______________________________________________________________________________
bool
highp::e50::
MQSink::HandleMultipartMessageData(FairMQParts& parts, int index)
{
  (void)index;

  Reporter::AddInputMessageSize(parts);
  // auto n = 0; 
  // for (auto& m : parts) n += m->GetSize();

  // std::cout << " hoge multipart" << std::endl;
  // for (auto& m : parts) {
  //   std::for_each(reinterpret_cast<uint64_t*>(m->GetData()),
  //                 reinterpret_cast<uint64_t*>(m->GetData())+m->GetSize()/sizeof(uint64_t),
  //                 HexDump{});
  // }


  //  return Count(n);
  return true;
}

//______________________________________________________________________________
// bool
// highp::e50::
// MQSink::HandleSingleMessageData(FairMQMessagePtr& msg, int index)
// {
//   std::cout << " single message size " << msg->GetSize() << std::endl;
//   std::for_each(reinterpret_cast<uint64_t*>(msg->GetData()),
//                 reinterpret_cast<uint64_t*>(msg->GetData())+msg->GetSize()/sizeof(uint64_t),
//                 HexDump{});
//   return Count(msg->GetSize());
// }

//______________________________________________________________________________
void
highp::e50::
MQSink::Init() 
{
  Reporter::Instance(fConfig); 
}

//______________________________________________________________________________
void
highp::e50::
MQSink::InitTask() 
{
  using opt = OptionKey;
  fInputChannelName = fConfig->GetValue<std::string>(opt::InputChannelName.data());
  fNumMessages      = fConfig->GetValue<uint64_t>(opt::NumMessages.data());
  fRecvCounter      = 0;
  fRecvBytes        = 0;

	std::cout << "#D fInputChannelName : " << fInputChannelName << std::endl;

  OnData(fInputChannelName, &MQSink::HandleMultipartMessageData);
  //  OnData(fInputChannelName, &MQSink::HandleSingleMessageData);

  Reporter::Reset();
}
