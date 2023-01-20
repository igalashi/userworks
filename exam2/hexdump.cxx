#include <iostream>
#include <iomanip>

void hexdump(char *data, int size)
{
	std::cout << std::hex;
	for (int i = 0 ; i < size ; i++) {
		if ((i % 16) == 0) {
			if (i != 0) std::cout << std::endl;
			std::cout
				<< std::setw(4) << std::setfill('0') << i
				<< " : ";
		}
		std::cout << " "
		<< std::setw(2) << std::setfill('0')
		<< (static_cast<unsigned int>(data[i]) & 0xff);
	}
	std::cout << std::dec << std::endl;

	return;
}

