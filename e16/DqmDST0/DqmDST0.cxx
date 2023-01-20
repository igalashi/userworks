#include <iostream>

#include <boost/algorithm/string/case_conv.hpp>

#include <runFairMQDevice.h>

#include "online/DqmDST0Impl.h"
#include "unpacker/MQHeader.h"
#include "mq/DqmDST0/DqmDST0.h"

namespace bpo = boost::program_options;
using opt = e16::daq::DqmDST0::OptionKey;

//_____________________________________________________________________________
void addCustomOptions(bpo::options_description &options)
{
   options.add_options()
      //
      (opt::InputDataChannelName.data(), bpo::value<std::string>()->default_value(opt::InputDataChannelName.data()),
       "Name of input data channel")
      //
      (opt::PollTimeout.data(), bpo::value<std::string>()->default_value("100"), "Poll timeout in milliseconds")
      //
      (opt::RunNumber.data(), bpo::value<std::string>(), "Run number (integer) given by DAQ plugin")
      //
      (opt::Prefix.data(), bpo::value<std::string>(), "File name prefix")
      //
      (opt::RunNumberFormat.data(), bpo::value<std::string>()->default_value("run{:06d}"), "Run number format")
      //
      (opt::ResetHist.data(), bpo::value<std::string>()->default_value("false"), "Reset histograms");

   DqmDST0Impl::AddOptions(options);
}

//_____________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
   return new e16::daq::DqmDST0;
}

//=============================================================================
namespace e16::daq {

//_____________________________________________________________________________
bool DqmDST0::ConditionalRun()
{
   Recv();
   fImpl->Update();
   return true;
}

//_____________________________________________________________________________
void DqmDST0::InitTask()
{
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

   fInputDataChannelName = get(opt::InputDataChannelName);
   fPrefix = get(opt::Prefix);
   fRunNumberFormat = get(opt::RunNumberFormat);

   fPoller = NewPoller(fInputDataChannelName);
   fPollTimeoutMS = std::stoi(get(opt::PollTimeout));
   fImpl = std::make_unique<DqmDST0Impl>();
   fImpl->Init(fConfig->GetVarMap());
}

//_____________________________________________________________________________
void DqmDST0::PostRun()
{
   auto filepath = fPrefix;
   if (!filepath.empty()) {
      filepath += "/";
   }
   filepath += e16::FileUtil::ToFormattedString(fRunNumberFormat, fRunNumber);
   LOG(debug) << filepath + ".pdf";
   fImpl->SaveROOTFile(filepath + ".root");
   fImpl->SavePDF(filepath + ".pdf");
}

//_____________________________________________________________________________
void DqmDST0::PreRun()
{
   if (fConfig->Count(opt::RunNumber.data()) > 0) {
      fRunNumber = std::stoi(fConfig->GetProperty<std::string>(opt::RunNumber.data()));
      LOG(debug) << " RUN number = " << fRunNumber;
      fImpl->SetRunNumber(fRunNumber);
   }
   fImpl->Update();
}

//_____________________________________________________________________________
void DqmDST0::Recv()
{
   // LOG(debug) << __func__ << ":" << __LINE__;
   fPoller->Poll(fPollTimeoutMS);
   if (!fPoller->CheckInput(fInputDataChannelName, 0)) {
      return;
   }

   // data receive
   FairMQParts parts;
   if (Receive(parts, fInputDataChannelName) <= 0) {
      return;
   }

   // check multipart size
   if (parts.Size() >= 2) {
      // received data contain header (MQ header) and body (DST0 data)

      // check message header size
      auto &msgBegin = parts[0];
      if (msgBegin.GetSize() < sizeof(MQHeader)) {
         LOG(error) << __func__ << " too small message size = " << msgBegin.GetSize();
         return;
      }

      // check message type
      const auto &mqHeader = *reinterpret_cast<e16::daq::MQHeader *>(msgBegin.GetData());
      if (mqHeader.magic != magic::Monitor) {
         LOG(error) << __func__ << " bad mesasge type  = "
                    << std::string(reinterpret_cast<const char *>(&mqHeader.magic), sizeof(mqHeader.magic));
      }

      for (auto i = 1, n = parts.Size(); i < n; ++i) {
         auto &msgBody = parts[i];
         fImpl->ProcessEvent(reinterpret_cast<char *>(msgBody.GetData()), msgBody.GetSize());
      }
   } else {
      // DST0 data only
      auto &msgBody = parts[0];
      fImpl->ProcessEvent(reinterpret_cast<char *>(msgBody.GetData()), msgBody.GetSize());
   }
}

//_____________________________________________________________________________
} // namespace e16::daq
