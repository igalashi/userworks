/*
 *
 *
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>

#include <assert.h>

#include "UnpackTdc.h"

#if 0
struct CoinCh {
	uint64_t FEMId;
	std::vector<uint32_t> Ch;
};
#endif


class Trigger
{
public:

//struct CoinCh {
//	uint64_t FEMId;
//	std::vector<uint32_t> Ch;
//};

	Trigger();
	virtual ~Trigger();
	void InitParam();
	bool SetTimeRegion(int);
	void Entry(uint64_t, int, int);
	bool CheckEntryFEM(uint64_t);
	void Mark(unsigned char *, int, int);
	std::vector<uint32_t> *Scan();
protected:
private:
	//std::vector<struct CoinCh> fEntry;
	std::map<uint64_t, std::vector<int>> fEntryCh;
	std::map<uint64_t, std::vector<int>> fEntryChDelay;
	int fNentry = 0;
	uint32_t fTimeRegionSize;
	uint32_t *fTimeRegion = nullptr;
	int fMarkCount = 0;
	uint32_t fMarkMask = 0;
	std::vector<uint32_t> fHits;
};

Trigger::Trigger()
{
	return;
}

Trigger::~Trigger()
{
	if (fTimeRegion != nullptr) delete fTimeRegion;
	return;
}

void Trigger::InitParam()
{
	fMarkCount = 0;
	fMarkMask = 0;
	fHits.clear();
	fHits.resize(0);

	return;
}

bool Trigger::SetTimeRegion(int size)
{
	fTimeRegionSize = size;
	fTimeRegion = new uint32_t[size];

	return true;
}


//if (Trig::CheckEntryFEM(lsubtimeframe.FEMId)) Trig::Mark(pdata);

#if 0
void Trigger::Entry(uint64_t fem, int ch)
{
	bool match = false;
	for (auto ent ; fEntry) {
		if (ent.FEMId == fem) {
			fEntry.Ch.emplace_back(ch);
			match = true;
			fNentry++;
		}
	} 
	if (match) {
		struct CoinCh newentry;
		newentry.FEMId = fem;
		newentry.Ch.emplace_back(ch);
		fEntry.emplace_back(newentry);
		fNentry++;
	}

	return;
}

bool Trigger::CheckEntryFEM(uint64_t fem)
{
	bool rval = false;
	for (auto ent ; fEntry) if (ent.FEMId == fem) rval = true;
	return rval;
}

#endif

void Trigger::Entry(uint64_t fem, int ch, int offset)
{

	fEntryCh[fem].emplace_back(ch);
	fEntryChDelay[fem].emplace_back(offset);

	return;
}

bool Trigger::CheckEntryFEM(uint64_t fem)
{
	bool rval = false;
	if (fEntryCh.count(fem) >= 1) rval = true;
	return rval;
}

void Trigger::Mark(unsigned char *pdata, int len, int fem)
{
	if (fEntryCh.count(fem) >= 1) {


		uint64_t *tdcval;
		tdcval = reinterpret_cast<uint64_t *>(pdata);
		//for (auto ch : fEntryCh[fem]) {
		for (unsigned int i ; i < fEntryCh[fem].size() ; i++) {
			int ch = fEntryCh[fem][i];
			int delay = fEntryChDelay[fem][i];
			for (unsigned int i = 0 ; i < (len / sizeof(uint64_t)) ; i++) {
				struct TDC64H::tdc64 tdc;
				if (TDC64H::Unpack(tdcval[i], &tdc) == TDC64H::T_TDC) {
					if (tdc.ch == ch) {
						uint32_t hit = tdc.tdc2u + delay;

						std::cout << "#D Ch: " << ch
							<< " Hit: " << hit << std::endl;

						if (hit < fTimeRegionSize) {
							if (hit > 1) {
							fTimeRegion[hit - 1] |= (0x1 << fMarkCount);
							}
							fTimeRegion[hit] |= (0x1 << fMarkCount);
							if (tdc.tdc <
								static_cast<int>(fTimeRegionSize) - 1) {
							fTimeRegion[hit + 1] |= (0x1 << fMarkCount);
							}
						}
					}
				}
			}
			fMarkMask |= (0x1 << fMarkCount);
			fMarkCount++;
			assert(fMarkCount <= static_cast<int>(sizeof(uint32_t)));
		}
	}


	return;
}

std::vector<uint32_t> *Trigger::Scan()
{
	std::cout << "#D fMarkMask: " << std::hex << fMarkMask << std::endl;
	fHits.clear();
	fHits.resize(0);
	for (unsigned int i = 0 ; i < fTimeRegionSize ; i++) {
		if ((fMarkMask & fTimeRegion[i]) == fMarkMask) {
			if ((i == 0) || (fHits.size() == 0)) {
				fHits.emplace_back(i);
			} else 
			if ((fHits.size() > 0) && (fHits.back() != (i - 1))) {
				fHits.emplace_back(i);
			}
		}
		if (fTimeRegion[i] != 0) {
			std::cout << "#D " << i
				<< std::hex << " " << fTimeRegion[i]
				<< std::dec << " " << fTimeRegion[i]
				<< " " << fMarkMask << std::endl;
		}
	}

	return &fHits;
}


int main(int argc, char* argv[])
{
	Trigger trig;

	static char cbuf[16];
	uint64_t *pdata = reinterpret_cast<uint64_t *>(cbuf);

	uint64_t *buf = new uint64_t[1024*1024*8];


	int time_region = 1024 * 256; //18 bit
	trig.InitParam();
	trig.SetTimeRegion(time_region);

	trig.Entry(1, 26, 10);
	trig.Entry(1, 29, 0);


	//struct TDC64H::tdc64 tdc;
	int i = 0;
	while (true) {
		std::cin.read(cbuf, 8);
		//std::cin >> *pdata; 
		if (std::cin.eof()) break;

		buf[i++] = *pdata;
		std::cout << "\r "  << i << ": " << *pdata << "  " << std::flush;
	}
	int len = i * sizeof(uint64_t);
	std::cout << std::endl;

	trig.Mark(reinterpret_cast<unsigned char *>(buf), len, 1);
	std::vector<uint32_t> *nhits = trig.Scan();
	std::cout << "# ";
	for (auto hit : *nhits) std::cout << " " << hit;
	std::cout << std::endl;
	std::cout << "Nhits: " << std::dec << nhits->size()
		<< " / " << time_region << std::endl;

	return 0;
}
