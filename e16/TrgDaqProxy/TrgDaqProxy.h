#ifndef E16DAQ_MQ_TrgDaqProxy_h
#define E16DAQ_MQ_TrgDaqProxy_h

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/iostreams/filtering_stream.hpp>

#include <FairMQDevice.h>

#include "unpacker/MQHeader.h"

namespace boost::program_options {
class options_description;
}

namespace e16::daq {
namespace io = boost::iostreams;

class Unpacker;

class TrgDaqProxy : public FairMQDevice {
public:
   struct OptionKey {
      static constexpr std::string_view Multipart{"multipart"};
      static constexpr std::string_view InputDataChannelName{"in"};
      static constexpr std::string_view OutputDataChannelName{"out"};

      static constexpr std::string_view InputDataFileName{"in-file"};
      static constexpr std::string_view TrgMrgIp{"trg-mrg-ip"};
      static constexpr std::string_view Ut3Ip{"ut3-ip"};

      static constexpr std::string_view UnpackerVersion{"version"};

      static constexpr std::string_view RunNumber{"run_number"};
   };

   const std::string fClassName;

   TrgDaqProxy() : FairMQDevice(), fClassName(__func__) {}
   TrgDaqProxy(const TrgDaqProxy &) = delete;
   TrgDaqProxy &operator=(const TrgDaqProxy &) = delete;
   ~TrgDaqProxy() override = default;

private:
   bool ConditionalRun() override;
   FairMQMessagePtr CreateHeader(uint32_t bodySize, uint32_t messageType);
   bool HandleMultipartData(FairMQParts &msgParts, int idnex);
   void HandleRequest();
   void InitTask() override;
   void PostRun() override;
   void PreRun() override;
   void SendBeginRunData();

   bool fMultipart;
   std::string fInputDataChannelName;
   std::string fOutputDataChannelName;

   std::string fInputDataFileName;
   std::string fUnpackerVersion;
   int64_t fRunNumber{0};
   uint64_t fHeaderSrcId{0};
   FairMQParts fBeginRunData;

   std::ifstream fInFile;
   std::unique_ptr<io::filtering_istream> fStream;

   std::vector<uint64_t> fTrgMrgIp;
   std::vector<uint64_t> fUt3Ip;
   // std::unordered_map<uint64_t, std::unique_ptr<Unpacker>> fUnpackers;
   std::vector<std::unique_ptr<Unpacker>> fUnpackers;
   std::vector<char> fBuffer;
   std::size_t fBufferSize{0};
   std::size_t fNumSequence;
};

} // namespace e16::daq

#endif