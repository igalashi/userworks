#include "MessageUtil.h"

#include <TROOT.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "Reporter.h"
#include "ExMonitor.h"

TH1* h {nullptr};

//______________________________________________________________________________
highp::e50::
ExMonitor::ExMonitor()
  : FairMQDevice()
{
}

//______________________________________________________________________________
void
highp::e50::
ExMonitor::Decode(std::vector<std::vector<char>>&& inputData)
{
  for (const auto& c : inputData) {
    std::cout << " container size = " << c.size() << std::endl;
    h->Fill(c.size());
  }
  gSystem->ProcessEvents();
}

//______________________________________________________________________________
bool
highp::e50::
ExMonitor::HandleData(FairMQParts& parts, int index)
{
  std::vector<std::vector<char>> inputData;
  inputData.reserve(parts.Size());
  for (const auto& m : parts) {
    std::vector<char> msg(std::make_move_iterator(reinterpret_cast<char*>(m->GetData())), 
                          std::make_move_iterator(reinterpret_cast<char*>(m->GetData()) + m->GetSize())); 
    inputData.emplace_back(std::move(msg));
  }

  Decode(std::move(inputData));

  return true;
}

//______________________________________________________________________________
void 
highp::e50::
ExMonitor::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
ExMonitor::Initialize(std::string_view server, 
                      std::string_view id)
{
  if (!fServer) {
    std::cout << " THttpServer = " << server << std::endl;
    fServer = std::make_unique<THttpServer>(Form("%s?top=%s", server.data(), id.data()));
  }

  if (!h) {
    h = new TH1F("h", "h;ch;count", 1000, 0, 1000);
    h->SetDirectory(0);
    fServer->Register("/hoge", h);
    std::cout << " histogram registered"  << std::endl;
  }
  else {
    h->Reset();
  }
  gSystem->ProcessEvents();
}

//______________________________________________________________________________
void 
highp::e50::
ExMonitor::InitTask()
{
  using opt = OptionKey;
  fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
  auto server        = fConfig->GetValue<std::string>(opt::Http.data());

  OnData(fInputChannelName, &ExMonitor::HandleData);

  Initialize(server, fConfig->GetValue<std::string>("id"));
}



