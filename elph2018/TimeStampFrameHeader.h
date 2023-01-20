#ifndef TimeStampFrameHeader_h
#define TimeStampFrameHeader_h

#include <cstdint>

namespace highp::e50::TimeStampFrame {


inline
namespace v0 {

// "DEAH-DTS" : little endian of "STD-HEAD"
constexpr uint64_t Magic {0x444145482d445453};

struct Header {
    uint64_t magic{Magic};
    uint64_t time {0};
};
}

} // namespace highp::e50::TimeStampFrame

#endif
