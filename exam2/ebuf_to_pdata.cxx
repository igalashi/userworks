/*
 *
 */

int evbuf_to_pdata(
	char *buf,
	std::vector<struct ch_header *> &pchh,
	std::vector<uint32_t *> &pdata)
{
	struct event_header *ev = reinterpret_cast<struct event_header*>(buf);
	int total_size = ev->size;
	struct ch_header *ch_top = reinterpret_cast<struct ch_header *>(buf + sizeof(event_header));
	struct ch_header *ch = ch_top;
	char *cch_top = reinterpret_cast<char *>(ch_top);
	char *cch = reinterpret_cast<char *>(ch);

	int nch = 0;
	while (cch - cch_top < total_size) {
		pchh.push_back(ch);
		uint32_t *pd = reinterpret_cast<uint32_t *>(cch + sizeof(struct ch_header));
		pdata.push_back(pd);
		#if 0
		std::cout << "#" << nch
			<< " ch size: " << ch->size
			<< " accum size: " << (cch - cch_top) << std::endl;
		#endif
		cch += ch->size + sizeof(ch_header);
		ch = reinterpret_cast<struct ch_header *>(cch);

		nch++;
	}

	return nch;
}