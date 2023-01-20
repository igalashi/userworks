#include <thread> // sleep
#include <chrono>

#include "Reporter.h"

#include "Sampler.h"

highp::e50::Reporter repo;

//______________________________________________________________________________
highp::e50::
Sampler::Sampler()
{
}

//______________________________________________________________________________
void
highp::e50::
Sampler::Init()
{
  Reporter::Instance(fConfig);
}
//______________________________________________________________________________
void
highp::e50::
Sampler::InitTask()
{
  // Get the fText and fMaxIterations values from the command line options (via fConfig)
  fText          = fConfig->GetValue<std::string>("text");
  fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
  fNumIterations = 0;

  if (fConfig->Count("run-id")) {
    fRunId = fConfig->GetValue<int>("run-id");
    std::cout << "Sampler found run-id " << fRunId << std::endl;
  }
  else ++fRunId;

}

//______________________________________________________________________________
bool
highp::e50::
Sampler::ConditionalRun()
{
  // create a copy of the data with new(), that will be deleted after the transfer is complete
  std::string* text = new std::string {fText 
                                       + " run: "   + std::to_string(fRunId)
                                       + " niter: " + std::to_string(fNumIterations)};

  LOG(info) << "Sending \"" << *text << "\"";
  // create message object with a pointer to the buffer,
  // its size, 
  // custom deletion function (called when transfer is done), 
  // and pointer to the object managing the data buffer
  FairMQMessagePtr msg(NewMessage(const_cast<char*>(text->c_str()), 
                                  text->length(), 
                                  [](void*, void* object) { delete static_cast<std::string*>(object); }, 
                                  text));


  // in case of error of transfer interruption, return false to go to IDLE state
  // successfull trnasfer will return number of bytes transfered (can be 0 if sending an empty message).
  ++fNumIterations;
  if (CheckCurrentState(RUNNING) && Send(msg, "data") < 0) {
    return false;
  }
  else if (fMaxIterations > 0 && fNumIterations >= fMaxIterations) {
    LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state."; 
    return false;
  }

  Reporter::Set(std::to_string(fNumIterations));

  std::this_thread::sleep_for(std::chrono::seconds(1));
  return true;
}
