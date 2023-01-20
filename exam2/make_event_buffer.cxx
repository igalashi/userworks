/*
 *
 */

#include <assert.h>

#include "dtdc_event.h"

char* make_event_buffer(
	std::vector< std::vector<uint32_t> > &data,
	int id, int index, int type, int *buf_size)
{
	struct event_header *evh;
	struct ch_header *chh;

	int event_size = sizeof(struct event_header);
	for (size_t i = 0 ; i < data.size() ; i++) {
		event_size += sizeof(struct ch_header);
		event_size += (data[i].size()) * sizeof(uint32_t);
	}
	char *buf = new char[event_size];

	evh = reinterpret_cast<struct event_header *>(buf);

	evh->magic = EV_MAGIC;
	evh->id = id;
	evh->hb_number = index;
	evh->type = type;
	evh->size = 0;

	char *pdata = buf + sizeof(struct event_header);
	for (size_t i = 0 ; i < data.size() ; i++) {
		chh = reinterpret_cast<struct ch_header *>(pdata);
		chh->name = 0xaa000000 | (i & 0xffff);
		chh->type = sizeof(uint32_t);
		chh->size = (data[i].size() * sizeof(uint32_t));
		pdata += sizeof(struct ch_header);
		memcpy(pdata, (data[i]).data(), chh->size);
		pdata += chh->size;
	}

	evh->size = (pdata - buf - sizeof(struct event_header));

	assert(event_size == (pdata - buf));

	*buf_size = event_size;
	
	return buf;
}
