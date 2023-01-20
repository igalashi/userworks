#include <algorithm>
#include <fstream>
#include <iostream>

#include <boost/algorithm/string/case_conv.hpp>

#include <FairMQLogger.h>

#include <TROOT.h>
#include <TSystem.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <THttpServer.h>
#include <TH1.h>
#include <TH2.h>

#include "unpacker/gbtxemu/GBTxEMUSpec.h"
#include "unpacker/gbtxemu/Unpacker.h"
#include "unpacker/gbtxemu/DefaultUpLinkMapPath.h"
#include "utility/filesystem.h"
#include "mq/Monitor/Dqm.h"
#include "mq/Monitor/gbtxemu/DqmGBTxEMU.h"

namespace bpo = boost::program_options;

std::unique_ptr<gbtxemu::Dqm> impl;

namespace e16::daq {

//_____________________________________________________________________________
void Dqm::AddOptions(bpo::options_description &options)
{
   using opt = gbtxemu::Dqm::OptionKey;
   options.add_options() //
      (opt::RunNumber.data(), bpo::value<std::string>(), "Run number (integer)")
      //
      (opt::Prefix.data(), bpo::value<std::string>(), "File name prefix")
      //
      (opt::RunNumberFormat.data(), bpo::value<std::string>()->default_value("run{:06d}"), "Run number format")
      //
      (opt::HttpEngine.data(), bpo::value<std::string>()->default_value("http:8080"), "http engine (e.g. http:8080)")
      //
      (opt::UpdateInterval.data(), bpo::value<std::string>()->default_value("100"),
       "Update interval of canvases in milliseconds")
      //
      (opt::UpLinkMapFile.data(), bpo::value<std::string>()->default_value(gbtxemu::DefaultUpLinkMapPath.data()),
       "file path for mapping from uplink-id to asic-id")
      //
      (opt::TrgUpLink.data(), bpo::value<std::string>()->default_value("-1"), "uplink ID for trigger input channel")
      //
      (opt::TrgChannel.data(), bpo::value<std::string>()->default_value("-1"), "channel ID for trigger input channel");
}

//_____________________________________________________________________________
bool Dqm::ConditionalRun()
{
   Recv();
   if (impl) {
      impl->Update();
   }
   return true;
}

//_____________________________________________________________________________
void Dqm::InitDqm()
{
   LOG(debug) << " initialize data quality monitor";
   auto get = [this](auto name) -> std::string {
      if (fConfig->Count(name.data()) < 1) {
         LOG(debug) << " variable: " << name << " not found";
         return "";
      }
      return fConfig->GetProperty<std::string>(name.data());
   };

   auto checkFlag = [this, &get](auto name) {
      std::string s = get(name);
      s = boost::to_lower_copy(s);
      return (s == "1") || (s == "true") || (s == "yes");
   };

   if (!impl) {
      impl = std::make_unique<gbtxemu::Dqm>();
   }
   // using opt = gbtxemu::Dqm::OptionKey;
   // auto runno = get(opt::RunNumber);
   // if (runno.empty()) {
   //   std::cout << " run number is empty.  " << std::endl;
   //   impl->fRunNumber = -1;
   //} else {
   //   std::cout << " run number =  " << runno << std::endl;
   //   impl->fRunNumber = std::stoi(get(opt::RunNumber));
   //}
   impl->Init(fConfig->GetVarMap());
}

//_____________________________________________________________________________
void Dqm::ProcessData(const char *buffer, std::size_t nbytes)
{
   impl->fBuffer.insert(impl->fBuffer.end(), buffer, buffer + nbytes);

   auto n = impl->fBuffer.size();
   const auto frameSize = sizeof(gbtxemu::v1::Word);
   gbtxemu::Hit hit;
   auto itr = impl->fBuffer.cbegin();
   while (n > sizeof(gbtxemu::v1::Word)) {
      gbtxemu::Clear(hit);
      impl->fUnpacker->Unpack(itr);

      hit = impl->fUnpacker->GetHit();

      impl->FillHit(hit);

      itr += frameSize;
      n -= frameSize;
   }

   std::vector<char> tmp(itr, impl->fBuffer.cend());
   impl->fBuffer.swap(tmp);
}

//_____________________________________________________________________________
void Dqm::SetRunNumber()
{
   auto get = [this](auto name) -> std::string {
      if (fConfig->Count(name.data()) < 1) {
         LOG(debug) << " variable: " << name << " not found";
         return "";
      }
      return fConfig->GetProperty<std::string>(name.data());
   };

   using opt = gbtxemu::Dqm::OptionKey;
   impl->fRunNumber = std::stoi(get(opt::RunNumber));
}

} // namespace e16::daq

//=============================================================================
namespace gbtxemu {

//_____________________________________________________________________________
std::unordered_map<int, int> makeUpLinkMap(const std::string &filename)
{
   std::unordered_map<int, int> ret;
   std::ifstream mapFile(filename);
   if (!mapFile.good()) {
      LOG(error) << __FILE__ << ":" << __func__ << " failed to open file: " << filename;
      return ret;
   }

   std::cout << " read uplink map file" << std::endl;
   while (mapFile) {
      std::string l;
      std::getline(mapFile, l);
      if (l.empty())
         continue;
      if (l.find("#") == 0)
         continue;

      std::istringstream iss(l);
      int k, v;
      iss >> k >> v;
      std::cout << " k = " << k << ", v = " << v << std::endl;
      ret[k] = v;
   }

   return ret;
}

//_____________________________________________________________________________
void Dqm::Init(const bpo::variables_map &vm, int run_number)
{
   using opt = gbtxemu::Dqm::OptionKey;

   auto get = [&vm](auto s) -> std::string {
      if (vm.count(s.data()) < 1) {
         std::cout << " variable: " << s << " not found" << std::endl;
         return "";
      }
      return vm[s.data()].template as<std::string>();
   };

   auto httpEngine = get(opt::HttpEngine);
   LOG(debug) << " http engine = " << httpEngine;
   fUpdateIntervalMS = std::stoi(get(opt::UpdateInterval));
   fHttpServer = std::make_unique<THttpServer>(httpEngine.data());

   fUpLinkMap = makeUpLinkMap(get(opt::UpLinkMapFile));
   if (fUpLinkMap.empty()) {
      return;
   }

   { // find min, max value of chip id
      for (auto &[k, v] : fUpLinkMap) {
         if (fMinChipId > v) {
            fMinChipId = v;
         }
         if (fMaxChipId < v) {
            fMaxChipId = v;
         }
      }
      LOG(debug) << " min chip id = " << fMinChipId << ", max chip id = " << fMaxChipId;
   }
   fTrgUpLink = std::stoi(get(opt::TrgUpLink));
   fTrgChannel = std::stoi(get(opt::TrgChannel));

   InitHist();
   MakeCanvas();

   fUnpacker = std::make_unique<gbtxemu::Unpacker>();
   fUnpacker->SetFindOnly(false);
}

//_____________________________________________________________________________
void Dqm::FillHit(const Hit &hit)
{
   if (hit.frameType != static_cast<int>(FrameType::Hit)) {
      return;
   }

   if (hit.elinkId == fTrgUpLink && hit.channel == fTrgChannel) {
      // TO DO : extract trigger tdc and tag data
      return;
   }

   int ch = fUpLinkMap[hit.elinkId] * NofChannelSmx + hit.channel;

   if (fH1Ch) {
      fH1Ch->Fill(ch);
   }
   if (fH1Adc) {
      fH1Adc->Fill(hit.adc);
   }
   if (fH1Tdc) {
      // fH1Tdc->Fill();
   }

   if (fH2Adc) {
      fH2Adc->Fill(ch, hit.adc);
   }

   if (fTrgUpLink > 0 && fTrgChannel > 0) {
      // TO DO: extract tdc of each hit
   }
}

//_____________________________________________________________________________
void Dqm::InitHist()
{
   const std::string regPrefix{"/gbtxemu"};
   auto nbins_ch = (fMaxChipId - fMinChipId + 1) * NofChannelSmx;
   auto xmin_ch = fMinChipId - 0.5;
   auto xmax_ch = fMaxChipId * NofChannelSmx - 0.5;
   if (!fH1Ch) {
      fH1Ch = new TH1D("hch", "GBTxEMU hit channel distribution", nbins_ch, xmin_ch, xmax_ch);
      fHttpServer->Register((regPrefix + "/ch").data(), fH1Ch);
   } else {
      fH1Ch->Reset();
   }

   if (!fH1Adc) {
      fH1Adc = new TH1D("hadc", "GBTxEMU adc", NLevelsAdc, -0.5, NLevelsAdc - 0.5);
      fHttpServer->Register((regPrefix + "/adc").data(), fH1Adc);
   } else {
      fH1Adc->Reset();
   }

   if (!fH1Tdc) {
      // H1Tdc = new TH1D("htdc", "tdc", );
      // fHttpServer->Register("/h/tdc", fH1Tdc);
   } else {
      fH1Tdc->Reset();
   }

   if (!fH2Adc) {
      fH2Adc = new TH2D("h2adc", "GBTxEMU hit : adc", nbins_ch, xmin_ch, xmax_ch, NLevelsAdc, -0.5, NLevelsAdc - 0.5);
      fHttpServer->Register((regPrefix + "/adc2d").data(), fH2Adc);
   } else {
      fH2Adc->Reset();
   }
}

//_____________________________________________________________________________
void Dqm::MakeCanvas() {}

//_____________________________________________________________________________
void Dqm::SavePDF(const TString &filename) {}

//_____________________________________________________________________________
void Dqm::SaveROOTFile(const TString &filename) {}

//_____________________________________________________________________________
void Dqm::Update(bool forceUpdate)
{
   auto t = std::chrono::steady_clock::now();
   if (std::chrono::duration_cast<std::chrono::milliseconds>(t - fPrevUpdateTimePoint).count() > fUpdateIntervalMS) {
      forceUpdate = true;
   }
   if (!forceUpdate) {
      return;
   }
   gSystem->ProcessEvents();
   fPrevUpdateTimePoint = t;
}

} // namespace gbtxemu