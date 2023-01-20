#ifndef E16DAQ_TagBroadcaster_h
#define E16DAQ_TagBroadcaster_h

// Broadcasting dummy tag data for an event-build test

#include <cstdint>
#include <string>
#include <string_view>

#include <FairMQDevice.h>

namespace e16::daq {

class TagBroadcaster : public FairMQDevice {
public:
   const std::string fClassName;
   struct OptionKey {
      static constexpr std::string_view TagDataChannelName{"tag"};
      static constexpr std::string_view NEventsMean{"n-events-mean"};
      static constexpr std::string_view NEventsStddev{"n-events-stddev"};
   };

   TagBroadcaster() : FairMQDevice(), fClassName(__func__) {}
   TagBroadcaster(const TagBroadcaster &) = delete;
   TagBroadcaster &operator=(const TagBroadcaster &) = delete;
   ~TagBroadcaster() override = default;

private:
   bool ConditionalRun() override;
   void InitTask() override;
   void PostRun() override;
   void PreRun() override;

   std::string fTagDataChannelName;
   uint32_t fEventId;
   uint16_t fSpillId;
   uint32_t fEventsInSpill;
   uint32_t fMaxEventsInSpill;
};

} // namespace e16::daq

#endif