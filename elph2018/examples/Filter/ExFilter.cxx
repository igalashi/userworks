#include "MessageUtil.h"
#include "Reporter.h"
#include "ExFilter.h"

//______________________________________________________________________________
highp::e50::
ExFilter::ExFilter()
  : FairMQDevice()
{
}

//______________________________________________________________________________
std::vector<std::vector<char>>
highp::e50::
ExFilter::ApplyFilter(const std::vector<std::vector<char>>& inputData)
{
  std::vector<std::vector<char>> outputData;


  outputData = inputData;

  return std::move(outputData);
}

//______________________________________________________________________________
bool
highp::e50::
ExFilter::HandleData(FairMQParts& parts, int index)
{
  std::vector<std::vector<char>> inputData;
  inputData.reserve(parts.Size());
  for (const auto& m : parts) {
    std::vector<char> msg(std::make_move_iterator(reinterpret_cast<char*>(m->GetData())), 
                          std::make_move_iterator(reinterpret_cast<char*>(m->GetData()) + m->GetSize())); 
    inputData.emplace_back(std::move(msg));
  }
  auto outputData = ApplyFilter(inputData);

  FairMQParts outParts;
  for (const auto& v : outputData) {
    auto vv = std::make_unique<std::vector<char>>(std::move(v));
    auto msg = MessageUtil::NewMessage(*this, std::move(vv));
    outParts.AddPart(std::move(msg));
  }
  while (CheckCurrentState(RUNNING) && Send(outParts, fOutputChannelName) < 0) {
    LOG(error) << " failed to enqueue filtered data "; 
  }
  return true;
}

//______________________________________________________________________________
void 
highp::e50::
ExFilter::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void 
highp::e50::
ExFilter::InitTask()
{
  using opt = OptionKey;
  fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
  fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());

  OnData(fInputChannelName, &ExFilter::HandleData);
}



