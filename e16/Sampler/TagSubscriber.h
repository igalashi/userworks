#ifndef E16DAQ_TagSubscriber_h
#define E16DAQ_TagSubscriber_h

// Subscribing dummy tag data

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <FairMQDevice.h>

namespace e16::daq {
class TagSubscriber : public FairMQDevice {
public:
   const std::string fClassName;

   struct OptionKey {
      static constexpr std::string_view RunNumber{"run_number"};
      // sub socket is assumed.
      static constexpr std::string_view TagInputChannelName{"tag-in"};
      // rep (reply) socket is assumed.
      static constexpr std::string_view BeginRunDataChannelName{"begin-run-data"};
      // push socket is assumed.
      static constexpr std::string_view DataChannelName{"data"};
      static constexpr std::string_view LengthMean{"length-mean"};
      static constexpr std::string_view LengthStddev{"length-stddev"};
      static constexpr std::string_view Discard{"discard"};
      static constexpr std::string_view Timeout{"timeout-ms;"};
      static constexpr std::string_view PollTimeout{"poll-timeout"};
   };

   TagSubscriber() : FairMQDevice(), fClassName(__func__) {}
   TagSubscriber(const TagSubscriber &) = delete;
   TagSubscriber &operator=(const TagSubscriber &) = delete;
   ~TagSubscriber() override = default;

protected:
   void CreateBeginRunData();
   FairMQMessagePtr CreateHeader();
   void HandleRequest();
   bool HandleTag(FairMQMessagePtr &m, int index);
   void InitTask() override;
   void PostRun() override;
   void PreRun() override;

   std::string fTagInputChannelName;
   std::string fDataChannelName;
   std::string fBeginRunDataChannelName;
   bool fDiscard;
   int fTimeoutMS;
   int fPollTimeoutMS;

   uint32_t fNBeginRun;
   uint32_t fNSequence;
   uint64_t fHeaderSrcId{0};
   int64_t fRunNumber{-1};
   FairMQParts fBeginRunData;
};

} // namespace e16::daq

#endif