#include <string>

#include "Reporter.h"
#include "Sink.h"

//______________________________________________________________________________
highp::e50::
Sink::Sink()
{
  // register a handler for data arriving on "data" channel
  OnData("data", &Sink::HandleData);
}

//______________________________________________________________________________
void
highp::e50::
Sink::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
Sink::InitTask()
{
  try {
    // Get the fMaxIterations value from the command line options (via fConfig)
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
    fNumIterations = 0;
  }
  catch (...) {
    LOG(error) << " exception in IintTask() ";
  }
}

//______________________________________________________________________________
// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool
highp::e50::
Sink::HandleData(FairMQMessagePtr& msg, int)
{
  LOG(info) << "Received: \"" << std::string(reinterpret_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
  ++fNumIterations;
  if (fMaxIterations > 0 && fNumIterations >= fMaxIterations)  {
    LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
    return false;
  }


  Reporter::Set(std::to_string(fNumIterations));

  // return true if what to be called again (otherwire return false go to IDLE state)
  return true;
}

//______________________________________________________________________________
void
highp::e50::
Sink::ResetTask()
{
  // read messages until the queue becomes empty. 
  // However, this method doesn't guarantee the queue is empty because the sender side may be still running state.
  for (auto i=0; i<100; ) {
    auto msg = NewMessage(); 
    if (ReceiveAsync(msg, "data") == -2) ++i;
  }
}
