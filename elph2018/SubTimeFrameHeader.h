#ifndef SubTimeFrameHeader_h
#define SubTimeFrameHeader_h

#include <cstdint>

namespace highp::e50::SubTimeFrame {

// This format is temporary and should be updated.
inline
namespace v0 {

// "DAEH-FTS" : little endian of "STF-HEAD"
constexpr uint64_t Magic {0x444145482d465453};

struct Header {
  uint64_t magic        {Magic};
  //uint64_t timeFrameId  {0}; 
  uint32_t timeFrameId  {0}; 
  uint32_t reseved      {0};
  uint64_t FEMId        {0};
  uint32_t length       {0};
  uint32_t numMessages  {0};
};

} // namespace v0

} // namespace highp::e50::SubTimeFrame

#endif
