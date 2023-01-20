#ifndef HeartbeatFrameHeader_h
#define HeartbeatFrameHeader_h

namespace highp::e50::HeartbeatFrame {


// This formaat is temporary and should be updated.
inline
namespace v0 {

// "DAEH-FBH" : little endian of "HBF-HEAD"
constexpr uint64_t Magic {0x444145482d464248};

struct Header {
  uint64_t magic   {Magic};
  uint64_t frameId {0};
  uint64_t time {0};
};

} // namespace v0
} // namespace highp::e50::HeartbeatFrame

#endif
