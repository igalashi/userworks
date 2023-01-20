/*
 *
 */

int evbuf_to_pch(
	char *buf,
	std::vector<struct ch_event *> &chevent)
{
	struct event_header *ev = reinterpret_cast<struct event_header*>(buf);
	int total_size = ev->size;
	struct ch_event *ch_top = reinterpret_cast<struct ch_event *>(buf + sizeof(event_header));
	struct ch_event *ch = ch_top;
	char *cch_top = reinterpret_cast<char *>(ch_top);
	char *cch = reinterpret_cast<char *>(ch);

	int nch = 0;
	while (cch - cch_top < total_size) {
		chevent.push_back(ch);
		#if 0
		std::cout << "#" << nch
			<< " ch size: " << ch->size
			<< " accum size: " << (cch - cch_top) << std::endl;
		#endif
		cch += ch->size + sizeof(ch_header);
		ch = reinterpret_cast<struct ch_event *>(cch);

		nch++;
	}

	return nch;
}