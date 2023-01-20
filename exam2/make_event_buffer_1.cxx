/*
 *
 */

#include "dtdc_event.h"

int make_event_buffer(
	std::vector< std::vector<uint32_t> > &data,
	int index, char *buf)
{
	struct event_header *evh;
	struct ch_header *chh;

	evh = reinterpret_cast<struct event_header *>(buf);

	evh->magic = EV_MAGIC;
	evh->node = 0x10000;
	evh->hb_number = index;
	evh->type = 0x0;
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

	return pdata - buf;
}
