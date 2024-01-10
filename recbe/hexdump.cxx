/*
 *
 *
 */

#include <iostream>
#include <iomanip>

void hexdump(char *pdata, int len)
{
	
	for (int i = 0 ; i < len ; i++) {
		if ((i % 16) == 0) {
			if (i != 0) std::cout << std::endl;
			std::cout << "# " << std::setw(4) << std::setfill('0')
				<< i << ":";
		}
		std::cout << " " << std::setw(2) << std::setfill('0')
			<< (static_cast<unsigned int>(pdata[i]) & 0xff);
	}
	std::cout << std::endl;

	return;
}
