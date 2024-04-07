/*
 *
 *
 */
#ifndef FltTdc_h
#define FltTdc_h

namespace FltTdc {

inline namespace v0 {
constexpr uint64_t MAGIC {0x00454d49'54475254};

struct Header {
    uint64_t magic       {MAGIC};
    uint32_t length      {0};
    uint16_t hLength     {0x10};
    uint16_t type        {0};
};

struct TrgTime {
    uint32_t type;
    uint32_t time;
}

} // namespace v0
} // namespace Chunk

#endif
