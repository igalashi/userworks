/*
 *
 */

#ifndef FileHeader_h
#define Fileheader_h

namespace File {
constexpr uint64_t Magic {0x444145482d534640};

struct Header {
	uint64_t size;
	uint64_t magic;
	uint32_t data[];
};

}

#endif
