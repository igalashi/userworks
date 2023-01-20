#ifndef RAWDataHeader_h
#define RAWDataHeader_h

#include <cstdint>

namespace highp::e50::RAW {

// This header format is temporary definition and should be updated.

inline 
namespace v0 {

// "DAEH-WAR" : little endian of "RAW-HEAD"
constexpr uint64_t Magic {0x444145482d574152};

struct Header {
  uint64_t word0 {Magic}; 
  uint64_t word1 {0};
  uint64_t word2 {0};
  uint64_t word3 {0};
}; // namespace v0 

} 

} // namespace highp::e50::RAW

#endif
