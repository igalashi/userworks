#ifndef FilterHeader_h
#define FilterHeader_h

#include <cstdint>
#include <sys/time.h>

namespace Filter {

namespace v0 {
// "NIOC-TLF" : little endian of "FLT-COIN"
constexpr uint64_t Magic {0x4e494f43'2d544c46};

struct Header {
    uint64_t magic       {Magic};
    uint64_t length      {0};
    uint32_t numTrigs    {0};
    uint32_t workerId    {0};
    uint32_t elapseTime  {0};
    struct timeval processTime {0, 0};
};
} //namespace v0

inline namespace v1 {
#pragma pack(4)

// " IGOLTLF" : little endian of "FLTLOGI "
constexpr uint64_t MAGIC {0x0049474f'4c544c46};

struct Header {
    uint64_t magic       {MAGIC};
    uint32_t length      {0};
    uint16_t hLength     {36};
    uint16_t type        {0};
    uint32_t timeFrameId {0};
    uint32_t numTrigs    {0};
    uint32_t workerId    {0};
    uint32_t numMessages {0};
    uint32_t elapseTime  {0};
    struct timeval processTime {0, 0};
};

// " EMITGRT"
constexpr uint64_t TDC_MAGIC {0x00454d49'54475254};
struct TrgTimeHeader {
    union {
        uint32_t u32data[4];
        struct {
            uint64_t magic       {TDC_MAGIC};
            uint32_t length      {0};
            uint16_t hLength     {0x14};
            uint16_t type        {0};
            //uint32_t timeFrameId {0};
        };
    };
};

constexpr unsigned char FLT_TDC_TYPE {0xaa};
struct TrgTime {
    union {
        unsigned char cdata[4]  {FLT_TDC_TYPE, 0, 0, 0};
        struct {
            uint32_t type :  8;
            uint32_t time : 24;
        };
    };
};

#pragma pack()
} //namespace v1

} // namespace Filter

#endif
