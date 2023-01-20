/*
 *
 */
#ifndef DTDC_EVENT_H
#define DTDC_EVENT_H

struct event_header {
	uint32_t magic;
	uint32_t id;
	uint32_t hb_number;
	uint32_t type;
	uint32_t size; //Bytes without this header
};

struct ch_header {
	uint32_t name;
	uint32_t type; // The size of the data unit.
	uint32_t size; // Bytes without this header
};

struct ch_event {
	uint32_t name;
	uint32_t type; // The size of the data unit.
	uint32_t size; // Bytes without this header
	uint32_t data[];
};

#define EV_MAGIC 0xfffffffe

#endif
