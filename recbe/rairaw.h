/*
 *
 */

#ifndef INC_RAIRAW
#define INC_RAIRAW

namespace RaiRaw {
inline namespace v0 {

constexpr uint64_t MAGIC {0xffff0160};

constexpr int T_TDC_LEADING   = 0xcc;
constexpr int T_TDC_TRAILING  = 0xcd;
constexpr int T_ADC           = 0x0a;

struct Header {
	uint32_t magic;
	uint32_t length; //word size unit
	uint16_t event_count;
	uint16_t reserve;
};


} // namespace v0
} // namespace RaiRaw
#endif
