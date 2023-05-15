#include <iostream>
#include <iomanip>
#include <fstream>

#include "FileHeader.h"
#include "FilterHeader.h"

int checkFilterHeader(char *file)
{
	char header[128];
	struct Filter::Header *fh = reinterpret_cast<Filter::Header *>(header);
	struct File::Header *fileheader = reinterpret_cast<File::Header *>(header);
	int bsize = 1024;
	char *body = new char[bsize];

	std::ifstream fin(file);

	while (true) {
		fin.read(header, sizeof(struct Filter::Header));
		if (fin.eof()) break;
		if (fin.gcount() == 0) break;

		if (fileheader->magic == File::Magic) {
			char cmagic[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
			uint64_t *u6magic = reinterpret_cast<uint64_t *>(cmagic);
			*u6magic = fileheader->magic;
			//std::cout << "Magic: " << std::hex << fileheader->magic
			std::cout << "Magic: " << cmagic
				<< " Len: " << fileheader->size
				<< std::endl;
			fin.read(body, fileheader->size - sizeof(struct Filter::Header));
			if (fin.eof()) break;
			continue;
		}


		char cmagic[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		uint64_t *u6magic = reinterpret_cast<uint64_t *>(cmagic);
		*u6magic = fh->magic;
		//std::cout << "Magic: " << std::hex << fh->magic
		std::cout << "Magic: " << cmagic
			<< " Len: " << std::dec << std::setw(4) << fh->length
			<< " Trig: " << std::setw(2) << fh->numTrigs
			<< " Id: " << std::setw(2) << fh->workerId
			<< " elapse: " << std::setw(5) << fh->elapseTime
			//<< " Sec: " << fh->processTime.tv_sec
			//<< " uSed: " << fh->processTime.tv_usec
			<< std::endl;

		int len = fh->length - sizeof(struct Filter::Header);
		if (len > bsize) {
			bsize = ((len + 64) / 64) * 64;
			delete[] body;
			body = new char[bsize];
			
		}
		fin.read(body, len);
		if (fin.eof()) break;
	}

	delete[] body;

	return 0;
}

int main(int argc, char *argv[])
{
	checkFilterHeader(argv[1]);
	return 0;
}
