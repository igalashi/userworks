#include <chrono>
#include <iostream>
#include <random>

#include <boost/algorithm/string/case_conv.hpp>

#include <runFairMQDevice.h>

#include "unpacker/GlobalTag.h"
#include "mq/Sampler/TagBroadcaster.h"

namespace bpo = boost::program_options;
using opt = e16::daq::TagBroadcaster::OptionKey;

std::random_device rnd_device;
std::mt19937_64 mt64_gen(rnd_device()); // 64-bit Mersenne twister engine
using dist_type = std::normal_distribution<>;
dist_type dist;

//_____________________________________________________________________________
void addCustomOptions(bpo::options_description &options)
{
   options.add_options()
      //
      (opt::TagDataChannelName.data(), bpo::value<std::string>()->default_value(opt::TagDataChannelName.data()),
       "Tag data channel name")
      //
      (opt::NEventsMean.data(), bpo::value<std::string>()->default_value("2500"), "Number of events per spill (mean)")
      //
      (opt::NEventsStddev.data(), bpo::value<std::string>()->default_value("20"),
       "Number of events per spill (stddev)");
}

//_____________________________________________________________________________
FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
   return new e16::daq::TagBroadcaster;
}

namespace e16::daq {
//_____________________________________________________________________________
bool TagBroadcaster::ConditionalRun()
{
   auto msg = NewMessage(sizeof(GlobalTag));
   auto &tag = *reinterpret_cast<GlobalTag *>(msg->GetData());
   uint64_t t =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
         .count();

   SetGlobalTag(tag, t, fSpillId, fEventId);
   Send(msg, fTagDataChannelName);

   if (fEventsInSpill == fMaxEventsInSpill) {
      while (true) {
         fMaxEventsInSpill = std::nearbyint(dist(mt64_gen));
         if (fMaxEventsInSpill > 0) {
            break;
         }
      }
      fEventsInSpill = 0;
      LOG(debug) << " event id = " << fEventId << ", spill id = " << fSpillId;
      ++fSpillId;
   } else {
      ++fEventsInSpill;
   }
   ++fEventId;

   return true;
}

//_____________________________________________________________________________
void TagBroadcaster::InitTask()
{
   using opt = OptionKey;
   auto get = [this](auto name) -> std::string {
      if (fConfig->Count(name.data()) < 1) {
         std::cout << " variable: " << name << " not found" << std::endl;
         return "";
      }
      return fConfig->GetProperty<std::string>(name.data());
   };

   // auto checkFlag = [this, &get](auto name) {
   //  std::string s = get(name);
   //  s = boost::to_lower_copy(s);
   //  return (s == "1") || (s == "true") || (s == "yes");
   //};

   fTagDataChannelName = get(opt::TagDataChannelName);
   auto mean = std::stod(get(opt::NEventsMean));
   auto stddev = std::stod(get(opt::NEventsStddev));

   dist_type::param_type param(mean, stddev);
   dist.param(param);
}

//_____________________________________________________________________________
void TagBroadcaster::PostRun() {}

//_____________________________________________________________________________
void TagBroadcaster::PreRun()
{
   fEventId = 0;
   fSpillId = 0;
   fEventsInSpill = 0;
   while (true) {
      fMaxEventsInSpill = std::nearbyint(dist(mt64_gen));
      if (fMaxEventsInSpill > 0) {
         break;
      }
   }
}

} // namespace e16::daq