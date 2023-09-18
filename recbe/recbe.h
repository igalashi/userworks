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
constexpr int T_RAW	     = 0x01;
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


//Register map
constexpr unsigned int R_VERSION        = 0x04;
constexpr unsigned int R_MODE           = 0x05; // 0x01: RAW, 0x02: PROC, 0x03: RAW and PROC
constexpr unsigned int R_WINDOW_SIZE    = 0x06;
constexpr unsigned int R_DELAY          = 0x07;
constexpr unsigned int R_ASUM_TH0       = 0x08;
constexpr unsigned int R_ASUM_TH1       = 0x09;
constexpr unsigned int R_ENA_MANCHESTER = 0x09;
constexpr unsigned int R_ENA_TOT        = 0x09;

constexpr unsigned int R_MODE_RAW       = 0x01;
constexpr unsigned int R_MODE_PROC      = 0x02;
constexpr unsigned int R_MODE_RAW_PROC  = 0x03;

} // namespace v0
} // namespace Recbe
#endif
