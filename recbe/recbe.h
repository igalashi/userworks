/*
 *
 */

#ifndef INC_RECBE
#define INC_RECBE

namespace Recbe {
inline namespace v0 {

#if 0
#define T_RAW		0x01
#define T_SUPPRESS	0x02
#define T_BOTH		0x03
#define T_RAW_OLD	0x22
#define T_SUPPRESS_OLD	0x20
#else
constexpr int T_RAW	         = 0x01;
constexpr int T_SUPPRESS     = 0x02;
constexpr int T_BOTH         = 0x03;
constexpr int T_RAW_OLD	     = 0x22;
constexpr int T_SUPPRESS_OLD = 0x20;
#endif

// network byte order
struct Header {
	unsigned char  type;
	unsigned char  id;
	unsigned short sent_num;
	unsigned short time;
	unsigned short len;
	unsigned int   trig_count;
};

} // namespace v0
} // namespace Recbe
#endif
