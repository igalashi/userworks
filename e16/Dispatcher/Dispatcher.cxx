#include <iostream>

#include <boost/algorithm/string/case_conv.hpp>

#include <runFairMQDevice.h>
#include "Dispatcher.h"

namespace bpo = boost::program_options;

//=============================================================================
void addCustomOptions(bpo::options_description &options)
{
   using opt = e16::daq::Dispatcher::OptionKey;
   options.add_options() //
      (opt::InputChannelName.data(), bpo::value<std::string>()->default_value("in"), "Name of input channel")
      //
      (opt::OutputChannelName.data(), bpo::value<std::string>()->default_value("out"), "Name of output channel")
      //
      (opt::MonitorChannelName.data(), bpo::value<std::string>()->default_value("monitor"), "Name of monitor channel")
      //
      (opt::Multipart.data(), bpo::value<std::string>()->default_value("true"), "Handle multipart message");
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &)
{
   return new e16::daq::Dispatcher;
}

//=============================================================================
namespace e16::daq {

//______________________________________________________________________________
bool Dispatcher::HandleData(FairMQMessagePtr &m, int index)
{
   FairMQMessagePtr msgCopy(NewMessage());
   msgCopy->Copy(*m);
   Send(msgCopy, fMonitorChannelName);

   int nretry = 0;
   while (Send(m, fOutputChannelName) < 0) {
      ++nretry;
      if (fNumRetry < 0) {
         continue;
      } else if (nretry >= fNumRetry) {
         LOG(error) << "Failed to send: " << fOutputChannelName;
         break;
      }
   }
   return true;
}

//______________________________________________________________________________
bool Dispatcher::HandleMultipartData(FairMQParts &parts, int index)
{
   FairMQParts partsCopy;
   for (const auto &m : parts) {
      FairMQMessagePtr msgCopy(NewMessage());
      msgCopy->Copy(*m);
      partsCopy.AddPart(std::move(msgCopy));
   }

   Send(partsCopy, fMonitorChannelName);

   int nretry = 0;
   while (Send(parts, fOutputChannelName) < 0) {
      ++nretry;
      if (fNumRetry < 0) {
         continue;
      } else if (nretry >= fNumRetry) {
         LOG(error) << "Failed to send: " << fOutputChannelName;
         break;
      }
   }

   return true;
}

//______________________________________________________________________________
void Dispatcher::Init() {}

//______________________________________________________________________________
void Dispatcher::InitTask()
{
   using opt = OptionKey;
   auto get = [this](auto name) -> std::string {
      if (fConfig->Count(name.data()) < 1) {
         std::cout << " variable: " << name << " not found" << std::endl;
         return "";
      }
      return fConfig->GetProperty<std::string>(name.data());
   };

   auto checkFlag = [this, &get](auto name) {
      std::string s = get(name);
      s = boost::to_lower_copy(s);
      return (s == "1") || (s == "true") || (s == "yes");
   };

   fInputChannelName = get(opt::InputChannelName);
   fOutputChannelName = get(opt::OutputChannelName);
   fMonitorChannelName = get(opt::MonitorChannelName);

   bool isMultipart = checkFlag(opt::Multipart);
   if (isMultipart) {
      OnData(fInputChannelName, &Dispatcher::HandleMultipartData);
   } else {
      OnData(fInputChannelName, &Dispatcher::HandleData);
   }
}
} // namespace e16::daq
