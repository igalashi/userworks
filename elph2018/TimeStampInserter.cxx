#include <cassert>
#include <chrono>
#include <iostream>

#include "utility/HexDump.h"
#include "HeartbeatFrameHeader.h"
#include "TimeStampFrameHeader.h"
#include "Reporter.h"
#include "TimeStampInserter.h"

//______________________________________________________________________________
highp::e50::
TimeStampInserter::TimeStampInserter()
    : FairMQDevice()
{
}

//______________________________________________________________________________
bool
highp::e50::
TimeStampInserter::HandleData(FairMQParts& msgParts, 
			      int index)
{
  (void)index;
  namespace HBF = HeartbeatFrame;
  assert(msgParts.Size() >= 2);
  
  TimeStampFrame::Header h;
  auto t  = std::chrono::steady_clock::now().time_since_epoch();
  h.time = std::chrono::duration_cast<std::chrono::nanoseconds>(t).count();


  // for (auto i=1, n=msgParts.Size(); i<n; ++i) {
  //     auto& m = msgParts.At(i);
  //     auto hbf = reinterpret_cast<HBF::Header*>(m->GetData());
  //     if (hbf->magic == HBF::Magic) {
  //         std::cout << hbf->time << " " << (h.time - hbf->time) << std::endl;
  //     }
  // }

  

  FairMQParts parts;
  parts.AddPart(std::move(msgParts.At(0)));
  parts.AddPart(NewSimpleMessage(h));
  for (auto i=1, n=msgParts.Size(); i<n; ++i) {
      parts.AddPart(std::move(msgParts.At(i)));
  }

  //while (CheckCurrentState(RUNNING) && Send(parts, fOutputChannelName) <= 0) {
  while ((GetCurrentState() == fair::mq::State::Running)
    && Send(parts, fOutputChannelName) <= 0) {
      LOG(warn) << "failed to send a message.";
  }
  
  return true;
}

//______________________________________________________________________________
void
highp::e50::
TimeStampInserter::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
TimeStampInserter::InitTask()
{
  using opt = OptionKey;
  fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
  fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());

  OnData(fInputChannelName, &TimeStampInserter::HandleData);
}
