#ifndef E16_UTILITY_MCS_PARSER_H_
#define E16_UTILITY_MCS_PARSER_H_

#include <cstdint>
#include <string_view>
#include <vector>

namespace e16 {
namespace mcs {
enum class Type : std::size_t {
   Data,
   EndOfFile,
   ExtendedSegmentAddress,
   SegmentAddress,
   ExtendedLinearAddress,
   LinearAddress
};

struct Record {
   uint8_t byteCount;
   uint16_t address;
   Type type;
   std::vector<uint8_t> data;
   uint8_t checkSum;

   void Print() const;
};

std::vector<Record> Parse(std::string_view filename);

} // namespace mcs
} // namespace e16

#endif
