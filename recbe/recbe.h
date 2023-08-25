/*
 *
 */

#ifndef INC_RECBE
#define INC_RECBE

#define T_RAW		0x01
#define T_SUPPRESS	0x02
#define T_BOTH		0x03
#define T_RAW_OLD	0x22
#define T_SUPPRESS_OLD	0x20

// network byte order
struct recbe_header {
	unsigned char  type;
	unsigned char  id;
	unsigned short sent_num;
	unsigned short time;
	unsigned short len;
	unsigned int   trig_count;
};

#endif
