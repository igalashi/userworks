/*
 *
 *
 */
#ifndef ChunkHeader_h
#define ChunkHeader_h

namespace Chunk {

inline namespace v0 {
constexpr uint64_t MAGIC {0x00000000'00000000};


struct Header {
    uint64_t magic       {MAGIC};
    uint32_t length      {0};
    uint16_t hLength     {0x10};
    uint16_t type        {0};
};

} // namespace v0
} // namespace Chunk

#endif
