/*
 *
 */

struct event_header {
	uint32_t magic;
	uint32_t node_header_size;
	uint32_t container_size;
};
struct node_header {
	uint32_t type;
	uint32_t id_number;
};

const uint32_t EVENT_MAGIC = 0xffffffff;
