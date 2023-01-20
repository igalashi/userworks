#include <algorithm>
#include "utility/HexDump.h"

#include <boost/algorithm/string/case_conv.hpp>

#include <runFairMQDevice.h>

#include "MQSink.h"

namespace bpo = boost::program_options;
using namespace highp::e50;
using opt = MQSink::OptionKey;

//=============================================================================
void addCustomOptions(bpo::options_description &options)
{
   // LOG(debug) << __FILE__ << ":" << __func__; // no output. FairLogger is not ready here.

   using opt = MQSink::OptionKey;
   options.add_options() //
      (opt::InputChannelName.data(), bpo::value<std::string>()->default_value("in"), "Name of input channel")
      //
      (opt::Multipart.data(), bpo::value<std::string>()->default_value("true"), "Handle multipart message");
}

//______________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
   LOG(debug) << __FILE__ << ":" << __func__;
   return new MQSink;
}

//=============================================================================
bool MQSink::Count(int n)
{
   return true;
}

//______________________________________________________________________________
bool MQSink::HandleData(FairMQMessagePtr &msg, int index)
{
   LOG(debug) << __func__ << " received ";
   return true;
}

//______________________________________________________________________________
bool MQSink::HandleMultipartData(FairMQParts &msgParts, int index)
{
   // LOG(debug) << __func__ << " received " << NewStatePending();
   // auto& m = msgParts.At(0);
   // auto p0 = reinterpret_cast<uint32_t*>(m->GetData());
   // auto p1 = p0 + m->GetSize()/sizeof(uint32_t);
   // std::for_each(p0, p1, HexDump());

   return true;
}

//______________________________________________________________________________
void MQSink::Init() {}

//______________________________________________________________________________
void MQSink::InitTask()
{
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
   LOG(debug) << " InitTask: input channel = " << fInputChannelName;

   fRecvCounter = 0;
   fRecvBytes = 0;

   bool isMultipart = checkFlag(opt::Multipart);
   if (isMultipart) {
      LOG(warn) << "InitTask: set multipart data handler";
      OnData(fInputChannelName, &MQSink::HandleMultipartData);
   } else {
      LOG(warn) << "InitTask: set data handler";
      OnData(fInputChannelName, &MQSink::HandleData);
   }
}

//______________________________________________________________________________
void MQSink::PreRun()
{
   if (fConfig->Count(opt::RunId.data())) {
      fRunId = fConfig->GetProperty<int32_t>(opt::RunId.data());
      LOG(debug) << " RUN ID = " << fRunId;
   }
}

//______________________________________________________________________________
// void MQSink::Run()
//{
//   while (!NewStatePending()) {
//      FairMQParts parts;
//      Receive(parts, fInputChannelName, 0, 2000);
//      sleep(1);
//      LOG(debug) << __func__ << " received " << parts.Size();
//   }
//   LOG(debug) << __func__;
//   ChangeState(fair::mq::Transition::Stop);
//}