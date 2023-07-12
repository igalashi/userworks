#include <iostream>
#include <algorithm>
#include "utility/HexDump.h"

struct nulstd
{
	int a;
	virtual void f(){}
};

int main(int argc, char* argv[])
{
	uint32_t data[] = {
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};

	struct nulstd ns;
	ns.a = 1;

	//std::cout << "# " << HexDump::cast<uint32_t>(ns) << std::endl;
	std::cout << "# " << HexDump::cast<uint32_t>(data[5]) << std::endl;


	std::for_each(
		&data[0],
		&data[16],
		HexDump(4));

	return 0;
}
