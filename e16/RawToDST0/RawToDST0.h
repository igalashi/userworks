#ifndef E16DAQ_MQ_RawToDST0_h
#define E16DAQ_MQ_RawToDST0_h

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include <FairMQDevice.h>

namespace boost::program_options {
class options_description;
}

namespace e16::daq {
class TaskProcessorMT;
class UnpackerManagerDST0;

class RawToDST0 : public FairMQDevice {
public:
   const std::string fClassName;

   struct OptionKey {
      static constexpr std::string_view NThreads{"n-threads"};
      static constexpr std::string_view InputDataChannelName{"in"};
      static constexpr std::string_view BeginRunDataChannelName{"begin-run"};
      static constexpr std::string_view OutputDataChannelName{"out"};
      static constexpr std::string_view InsertMQHeader{"insert-mq-header"};

      static constexpr std::string_view Timeout{"timeout"};
      static constexpr std::string_view InProcMQLength{"inproc-mq-length"};
   };

   RawToDST0() : FairMQDevice(), fClassName(__func__) {}
   RawToDST0(const RawToDST0 &) = delete;
   RawToDST0 &operator=(const RawToDST0 &) = delete;
   ~RawToDST0() override = default;

private:
   bool CheckBeginRunData(FairMQParts &msgParts, int index);
   FairMQParts Convert(FairMQParts &msgParts, int index);
   bool HandleData(FairMQParts &msgParts, int index);
   bool HandleDataMT(FairMQParts &msgParts, int index);
   void InitTask() override;
   void PostRun() override;
   void PreRun() override;
   bool SendData(FairMQParts &msgParts, int index);

   // input channel
   std::string fInputDataChannelName;
   std::string fBeginRunDataChannelName;

   // output channel
   std::string fOutputDataChannelName;
   // std::string fDqmDataChannelName;
   bool fInsertMQHeader;

   std::unordered_map<uint64_t, std::vector<std::vector<char>>> fBeginRunData;

   std::vector<std::unique_ptr<UnpackerManagerDST0>> fUnpackers;

   int fMQTimeoutMS;
   int fInProcMQLength;

   std::size_t fNReceived{0};
   std::size_t fNSend{0};
   int fNThreads;
   std::unique_ptr<TaskProcessorMT> fWorker;
};
} // namespace e16::daq

#endif