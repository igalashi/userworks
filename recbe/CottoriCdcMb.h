/*
 *
 */

#ifndef INC_CottriCdcMb
#define INC_CottriCdcMb

namespace CottoriCdcMb {
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

constexpr int N_CH = 48;


// network byte order
struct Header {
	unsigned short magic;
	unsigned char  id;
	unsigned char  mode;
	unsigned char  n_sample;
	unsigned char  delay;
	unsigned short trig_counts_u;
	unsigned short trig_counts_l;
	unsigned short internal_trig_u;
	unsigned short internal_trig_l;
	unsigned short magic2;
//	unsigned char[120] data;
};

constexpr int N_DATA_BYTES = 150;
constexpr uint32_t MAGIC = 0xaaaa;
constexpr uint32_t MAGIC2 = 0x5555;


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
