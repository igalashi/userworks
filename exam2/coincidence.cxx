/*
 *
 */
#include <iostream>
#include <iomanip>
#include <vector>

int coincidence(char *buf, int width, std::vector<uint32_t> &hit)
{
	std::vector<struct ch_event *> pch;
	int nch = evbuf_to_pch(buf, pch);

	if (nch > static_cast<int>(sizeof(uint32_t) * 8)) {
		std::cerr << "#E " << __FILE__ << ":" << __LINE__
			<< " nch exceeed. : nch : " << nch << std::endl;
		exit(0);
	}
	uint32_t region[FULL];
	for (int i = 0 ; i < FULL ; i++) region[i] = 0;
	// Mark hit times
	for (int i = 0 ; i < nch ; i++)	{
		for (size_t j = 0 ; j < (pch[i]->size / pch[i]->type) ; j++) {
			for (int k = 0 ; k < width ; k++) {
				int index = pch[i]->data[j] + k - (width / 2);
				if ((index >= 0) && (index < FULL)) {
					region[index] |= (0x1 << i);
					#if 0
					std::cout << "#D " << i << " " << j << " : "
						<< index << "/" << FULL
						<< " : " << pch[i]->data[j] << " 0x"
						<< std::hex << region[index]
						<< std::dec << std::endl;
					#endif
				}
			}
		}
	}
	uint32_t mask = 0;
	for (int i = 0 ; i < nch ; i++) mask |= (0x1 << i);

	#if 0
	std::cout << "#D mask : " << std::hex << mask
		<< " Nch : " << std::dec << nch << " " << pch.size() << std::endl;
	#endif

	#if 0
	std::cout << "#D region : ";
	for (int i = 0 ; i < 128 ; i++) {
		std::cout << " " << std::hex << region[i];
	}
	std::cout << std::dec << std::endl;
	#endif
	#if 0
	std::cout << "#D region idx : ";
	for (int i = 0 ; i < FULL ; i++) {
		//if ((region[i] & 0xffffffff) == 0xffffffff) std::cout << " " << i;
		if ((region[i] & 0xf) == 0xf) std::cout << " " << i;
	}
	std::cout << std::endl;
	#endif

	int nhit = 0;
	for (int i = 0 ; i < FULL ; i++) {
		if (   ((region[i - 1] & mask) != mask)
			&& ((region[i] & mask) == mask)) {
				hit.push_back(i);
				nhit++;
		}
	}

	return nhit;
}
