/*
 *
 */
#include <iostream>
#include <iomanip>

int check_hit(
	std::vector<struct ch_event *> &pch,
	std::vector<uint32_t> &hit,
	int width)
{
	std::vector<int> ch0;
	for (size_t i = 0 ; i < (pch[0]->size / pch[0]->type) ; i++) {
		ch0.push_back(pch[0]->data[i]);
	}

	std::cout << "# Nch0 : " << ch0.size()
		<< " Nhit : " << hit.size() << std::endl;

	std::vector<int> match;
	int skip = 0;
	for (size_t i = 0; i < hit.size() ; i++) {
		for (size_t j = skip ; j < ch0.size() ; j++) {
			if (abs(ch0[j] - hit[i]) <= width) {
				match.push_back(hit[i]);
				skip = j;
				break;
			}
		}
	}
	std::cout << "# Nmatch : " << match.size() << std::endl;

	std::vector<int> unmatch;
	skip = 0;
	for (size_t i = 0; i < ch0.size() ; i++) {
		bool is_found = false;
		for (size_t j = skip ; j < hit.size() ; j++) {
			if (abs(ch0[i] - hit[j]) <= width) {
				skip = j;
				is_found = true;
				break;
			}
		}
		if (!is_found) {
				unmatch.push_back(ch0[i]);
				std::cout << "# Unmatch : " << ch0[i] << std::endl;
		}
	}
	std::cout << "# N unmatch : " << unmatch.size() << std::endl;
	
	return static_cast<int>(match.size());
}
