#ifndef SubTimeFrameHeaderLocal_h
#define SubTimeFrameHeaderLocal_h

#include <cstdint>

namespace SubTimeFrame {

// This format is temporary and should be updated.
namespace v0 {

constexpr uint32_t RECBE          {0x0000be00};
constexpr uint32_t RECBE_RAW      {0x0000be00};
constexpr uint32_t RECBE_SUPRESS  {0x0000be21};
constexpr uint32_t COTTORI_CDC_FE {0x0000cdcf};
constexpr uint32_t COTTORI_CDC_MB {0x0000cdcb};
constexpr uint32_t COTTORI_CTH_FE {0x0000ccfe};


} // namespace v0

inline namespace v1 {

constexpr uint32_t RECBE          {0x0000be00};
constexpr uint32_t RECBE_RAW      {0x0000be00};
constexpr uint32_t RECBE_SUPRESS  {0x0000be21};
constexpr uint32_t COTTORI_CDC_FE {0x0000cdcf};
constexpr uint32_t COTTORI_CDC_MB {0x0000cdcb};
constexpr uint32_t COTTORI_CTH_FE {0x0000ccfe};

} // namespace v1

} // namespace SubTimeFrame

#endif
