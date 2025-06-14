/*
 *
 *
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>

#include <string.h>
#include <assert.h>

#include "UnpackTdc.h"
#include "SubTimeFrameHeader.h"
#include "TriggerMap.cxx"


struct HBFIndex {
	int msg_index;
	uint32_t timeFrameId;
	uint32_t femType;
	uint32_t femId;
	unsigned char* data;
	int size;
};


class Trigger
{
public:
	Trigger();
	virtual ~Trigger();
	void InitParam();
	bool SetTimeRegion(int);
	void CleanUpTimeRegion();
	uint32_t *GetTimeRegion();
	uint32_t GetTimeRegionSize();
	void Entry(uint32_t, int, int);
	void ClearEntry();
	bool CheckEntryFEM(uint32_t);
	void Mark(unsigned char *, int, int, uint32_t);
	std::vector<uint32_t> *Scan();
	std::vector<uint32_t> *Exec(std::vector<struct HBFIndex> &);
	void SetMarkLen(int val) {fMarkLen = val;};
	int GetMarkLen() {return fMarkLen;};
	//void SetLogic(int);
	void MakeTable(std::string &);
	void SetTriggerType(uint32_t tt){fTriggerType = tt;};
	uint32_t GetTriggerType(){return fTriggerType;};
	static int sOid;
protected:
private:
	//std::vector<struct CoinCh> fEntry;
	std::map< uint32_t, std::vector<int> > fEntryCh;
	std::map< uint32_t, std::vector<int> > fEntryChDelay;
	std::map< uint32_t, std::vector<uint32_t> > fEntryChBit;
	int fEntryCounts = 0;
	uint32_t fEntryMask = 0;

	int fNentry = 0;
	uint32_t fTimeRegionSize;
	uint32_t *fTimeRegion = nullptr;
	//int fMarkCount = 0;
	//uint32_t fMarkMask = 0;
	std::vector<uint32_t> fHits;
	int fMarkLen = 5;

	TriggerMap fTMap;

	uint32_t fTriggerType = 0;
	int fId = 0;
};

int Trigger::sOid = 0;

Trigger::Trigger()
{
	fId = sOid++;
	return;
}

Trigger::~Trigger()
{
	if (fTimeRegion != nullptr) {
		delete[] fTimeRegion;
		fTimeRegion = nullptr;
	}
	return;
}

//void Trigger::SetLogic(int nsignal, std::string formula)
void Trigger::MakeTable(std::string & formula)
{
	fTMap.MakeTable(formula);
	return;
}

void Trigger::InitParam()
{
	//fMarkCount = 0;
	//fMarkMask = 0;
	fHits.clear();
	fHits.resize(0);
	if (fTimeRegion != nullptr) {
		memset(fTimeRegion, 0, fTimeRegionSize * sizeof(uint32_t));
	}

	return;
}

bool Trigger::SetTimeRegion(int size)
{
	fTimeRegionSize = size;
	if (fTimeRegion != nullptr) {
		delete[] fTimeRegion;
		fTimeRegion = nullptr;
	}
	fTimeRegion = new uint32_t[size];

	return true;
}

void Trigger::CleanUpTimeRegion()
{
	for (uint32_t i = 0 ; i < fTimeRegionSize ; i++) fTimeRegion[i] = 0;
	return;
}

uint32_t *Trigger::GetTimeRegion()
{
	return fTimeRegion;
}

uint32_t Trigger::GetTimeRegionSize()
{
	return fTimeRegionSize;
}


//if (Trig::CheckEntryFEM(lsubtimeframe.femId)) Trig::Mark(pdata);

#if 0
void Trigger::Entry(uint64_t fem, int ch)
{
	bool match = false;
	for (auto ent ; fEntry) {
		if (ent.femId == fem) {
			fEntry.Ch.emplace_back(ch);
			match = true;
			fNentry++;
		}
	} 
	if (match) {
		struct CoinCh newentry;
		newentry.fenId = fem;
		newentry.Ch.emplace_back(ch);
		fEntry.emplace_back(newentry);
		fNentry++;
	}

	return;
}

bool Trigger::CheckEntryFEM(uint64_t fem)
{
	bool rval = false;
	for (auto ent ; fEntry) if (ent.femId == fem) rval = true;
	return rval;
}

#endif

void Trigger::Entry(uint32_t fem, int ch, int offset)
{

	fEntryCh[fem].emplace_back(ch);
	fEntryChDelay[fem].emplace_back(offset);
	fEntryChBit[fem].emplace_back(0x0000001 << fEntryCounts);
	fEntryMask |= 0x00000001 << fEntryCounts;
	fEntryCounts++;

	#if 0
	std::cout << "#D Trig Entry : Module: " << fem << " Ch: " << ch << std::endl;
	#endif
	if (static_cast<unsigned int>(fEntryCounts) > (sizeof(uint32_t) * 8)) {
		std::cerr << "Entry Ch. exceed " << sizeof(uint32_t) * 8<< std::endl;
	}
	assert(fEntryCounts <= static_cast<int>(sizeof(uint32_t) * 8));

	return;
}

void Trigger::ClearEntry()
{
	fEntryCh.clear();
	fEntryChDelay.clear();
	fEntryChBit.clear();
	fEntryMask = 0x00000000;
	fEntryCounts = 0;

	return;
}

bool Trigger::CheckEntryFEM(uint32_t fem)
{
	bool rval = false;
	if (fEntryCh.count(fem) >= 1) rval = true;
	return rval;
}

void Trigger::Mark(unsigned char *pdata, int len, int fem, uint32_t type)
{
	if (fEntryCh.count(fem) >= 1) {
		uint64_t *tdcval;
		tdcval = reinterpret_cast<uint64_t *>(pdata);

		for (unsigned int i = 0 ; i < fEntryCh[fem].size() ; i++) {
			int ch = fEntryCh[fem][i];
			int delay = fEntryChDelay[fem][i];
			uint32_t markbit = fEntryChBit[fem][i];

			#if 0
			std::cout << "#DD Trigger::Mark " 
				<< " FEM: " << std::hex << fem
				<< " Ch: " << std::dec << ch
				<< " MarkBit: " << markbit
				<< " Data len: " << std::dec << len << " / " << (len / sizeof(uint64_t))
				<< " Type: " << std::dec << type
				<< "(" << SubTimeFrame::TDC64H << "/" << SubTimeFrame::TDC64L
				<< "," << SubTimeFrame::TDC64H_V3 << "/" << SubTimeFrame::TDC64L_V3 << ")"	
				<< std::endl;
			#endif

			for (unsigned int j = 0 ; j < (len / sizeof(uint64_t)) ; j++) {

				if (type == SubTimeFrame::TDC64H) {
					struct TDC64H::tdc64 tdc;
					if (TDC64H::Unpack(tdcval[j], &tdc) == TDC64H::T_TDC) {
						if (tdc.ch == ch) {
							uint32_t hit = tdc.tdc4n + delay;

							#if 0		
							std::cout << "#D TDC64H Mark"
								<< " FEM: " << std::hex << fem
								<< " Ch: " << std::dec << ch
								<< " Hit: " << hit << std::endl;
							#endif

							if (hit < fTimeRegionSize - (fMarkLen/2)) {
								for (int k = -1 * (fMarkLen/2) ; k < ((fMarkLen/2) + 1) ; k++) {
									if ((hit + k) < fTimeRegionSize) {
										fTimeRegion[hit + k] |= markbit;
									} else if ((static_cast<int>(hit) + k) >= 0) {
										std::cout << "#E Over range hit!"
											<< " FEM: " << std::hex << fem
											<< " Ch: " << std::dec << ch
											<< " Hit: " << hit
											<< std::endl;
									}
								}
							}
						}
					}
				} else
				if (type == SubTimeFrame::TDC64L) {
					struct TDC64L::tdc64 tdc;
					if (TDC64L::Unpack(tdcval[j], &tdc) == TDC64L::T_TDC) {
						if (tdc.ch == ch) {
							uint32_t hit = tdc.tdc4n + delay;

							#if 0
							std::cout << "#D TDC64L Mark"
								<< " FEM: " << std::hex << fem
								<< " Ch: " << std::dec << ch
								<< " Hit: " << hit << std::endl;
							#endif

							if (hit < fTimeRegionSize - (fMarkLen/2)) {
								for (int k = -1 * (fMarkLen/2) ; k < ((fMarkLen/2) + 1) ; k++) {
									if ((hit + k) < fTimeRegionSize) {
										fTimeRegion[hit + k] |= markbit;
									} else if ((static_cast<int>(hit) + k) >= 0) {
										std::cout << "#E Over range hit!"
											<< " FEM: " << std::hex << fem
											<< " Ch: " << std::dec << ch
											<< " Hit: " << hit
											<< std::endl;
									}
								}
							}
						}
					}
				} else

				if (type == SubTimeFrame::TDC64H_V3) {
					struct TDC64H_V3::tdc64 tdc;
					if (TDC64H_V3::Unpack(tdcval[j], &tdc) == TDC64H_V3::T_TDC) {
						if (tdc.ch == ch) {
							uint32_t hit = tdc.tdc4n + delay;

							#if 0
							std::cout << "#D TDC64H_V3 Mark"
								<< " FEM: " << std::hex << fem
								<< " Ch: " << std::dec << ch
								<< " Hit: " << hit
								<< std::endl;
							#endif

							if (hit < fTimeRegionSize - (fMarkLen/2)) {
								for (int k = -1 * (fMarkLen/2) ; k < ((fMarkLen/2) + 1) ; k++) {
									if ((hit + k) < fTimeRegionSize) {
										fTimeRegion[hit + k] |= markbit;
									} else if ((static_cast<int>(hit) + k) >= 0) {
										std::cout << "#E Over range hit!"
											<< " FEM: " << std::hex << fem
											<< " Ch: " << std::dec << ch
											<< " Hit: " << hit
											<< " Mark: " << static_cast<int>(hit) + k
											<< std::endl;
									}
								}
							}
						}
					}
				} else
				if (type == SubTimeFrame::TDC64L_V3) {
					struct TDC64L_V3::tdc64 tdc;
					if (TDC64L_V3::Unpack(tdcval[j], &tdc) == TDC64L::T_TDC) {
						if (tdc.ch == ch) {
							uint32_t hit = tdc.tdc4n + delay;

							#if 0
							std::cout << "#D TDC64L_V3 Mark"
								<< " FEM: " << std::hex << fem
								<< " Ch: " << std::dec << ch
								<< " Hit: " << hit
								<< std::endl;
							#endif

							if (hit < fTimeRegionSize - (fMarkLen/2)) {
								for (int k = -1 * (fMarkLen/2) ; k < ((fMarkLen/2) + 1) ; k++) {
									if ((hit + k) < fTimeRegionSize) {
										fTimeRegion[hit + k] |= markbit;
									} else if ((static_cast<int>(hit) + k) >= 0) {
										std::cout << "#E Over range hit!"
											<< " FEM: " << std::hex << fem
											<< " Ch: " << std::dec << ch
											<< " Hit: " << hit
											<< std::endl;
									}
								}
							}
						}
					}
				}

			}

			//fMarkMask |= (0x1 << fMarkCount);
			//fMarkCount++;
			//std::cout << "#D Trig Mark entry : Module: " << fem << " Ch: " << ch << std::endl;
			//if (static_cast<unsigned int>(fMarkCount) > sizeof(uint32_t)) {
			//	std::cerr << "Entry Ch. exceed " << sizeof(uint32_t) << std::endl;
			//}
			//assert(fMarkCount <= static_cast<int>(sizeof(uint32_t)));
		}
	}


	return;
}

std::vector<uint32_t> *Trigger::Scan()
{
	//std::cout << "#D Scan fMarkMask: " << std::hex << fMarkMask << std::endl;
	//std::cout << "#D Scan fEntryMask: " << std::hex << fEntryMask << std::endl;
	fHits.clear();
	fHits.resize(0);

	for (unsigned int i = 0 ; i < fTimeRegionSize - 1; i++) {
		#if 0
		if (((fEntryMask & fTimeRegion[i]) != fEntryMask)
		&& ((fEntryMask & fTimeRegion[i + 1]) == fEntryMask)) {
			fHits.emplace_back(i + 1);
		}
		#else
		if ((! fTMap.LookUp(fTimeRegion[i]))
		 && (fTMap.LookUp(fTimeRegion[i + 1]))) {
			fHits.emplace_back(i + 1);
		}
		#endif

		#if 0
		if (fTimeRegion[i] != 0) {
			std::cout << "#D Scan Time: " << std::dec << i
				<< " Bits: " << std::hex << fTimeRegion[i]
				//<< " Mask: " << fMarkMask << std::endl;
				<< " Mask: " << fEntryMask << std::endl;
		}
		#endif
	}

	return &fHits;
}

std::vector<uint32_t> *Trigger::Exec(std::vector<struct HBFIndex> &hbf)
{
	Trigger::CleanUpTimeRegion();

	for (auto &seg : hbf) {
		//fTrig->Mark(
		//	reinterpret_cast<unsigned char *>(inParts[mindex].GetData()),
		//	inParts[mindex].GetSize(),
		//	vfemid, dbl->Type);

		#if 0
		std::cout << "#DT seg.size: " << seg.size << ",  femId: " << seg.femId
			<< ", femType: " << seg.femType << std::endl;
		#endif

		Trigger::Mark(seg.data, seg.size, seg.femId, seg.femType);

		#if 0
		//uint32_t *tr = fTrig->GetTimeRegion();
		uint32_t *tr = Trigger::GetTimeRegion();
		std::cout << "####DDDD Hit TimeRegion: ";
		//for (uint32_t ii = 0 ; ii < fTrig->GetTimeRegionSize() ; ii++) {
		for (uint32_t ii = 0 ; ii < Trigger::GetTimeRegionSize() ; ii++) {
			if (tr[ii] != 0) {
			std::cout << " " << std::dec << ii << ":"
				<< std::hex << std::setw(4) << std::setfill('0')
				<< tr[ii];
			}
		}
		std::cout << std::endl;
		#endif

		#if 0
		std::cout << "# HB: " << std::dec << i;
		for (size_t iifem = 0 ; iifem < block_map.size() ; iifem++) {
			struct DataBlock *dbl = &block_map[iifem][i];
			uint64_t vfemid = dbl->femId;
			uint64_t vhbframe = dbl->HBFrame;
			//std::cout << "# HB: " << std::dec << i
			//<< " FEM: " << std::hex << vfemid
			std::cout << " " << std::dec << (vfemid  & 0xff) << ":" << vhbframe;
		}
		std::cout << std::endl;
		#endif

	}

	return Trigger::Scan();
}




#ifdef TEST_MAIN_TRIG
int main(int argc, char* argv[])
{
	Trigger trig;

	static char cbuf[16];
	uint64_t *pdata = reinterpret_cast<uint64_t *>(cbuf);

	uint64_t *buf = new uint64_t[1024*1024*8];


	int time_region = 1024 * 256; //18 bit
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
	//int len = i * sizeof(uint64_t);
	std::cout << std::endl;

	unsigned char *pcurr = reinterpret_cast<unsigned char *>(buf);
	unsigned char *pend = reinterpret_cast<unsigned char *>(buf + i);
	unsigned char *pnext = nullptr;
	while (true) {
		int len = TDC64H::GetHBFrame(pcurr, pend, &pnext);
		if (len <= 0) break;

		std::cout << "#D HB frame size: " << std::dec << len
			<< " curr: " << std::hex
			<< reinterpret_cast<uintptr_t>(pcurr)
			<< " next: "
			<< reinterpret_cast<uintptr_t>(pnext) << std::endl;

		pdata = reinterpret_cast<uint64_t *>(pcurr);
		#if 0
		for (unsigned int j = 0 ; j < len / sizeof(uint64_t) ; j++) {
			tdc64h_dump(pdata[j]);
		}
		#endif

		trig.InitParam();
		trig.Mark(reinterpret_cast<unsigned char *>(pdata), len, 1, 0);
		std::vector<uint32_t> *nhits = trig.Scan();
		std::cout << "# Hits: ";
		for (auto hit : *nhits) std::cout << " " << hit;
		std::cout << std::endl;
		std::cout << "Nhits: " << std::dec << nhits->size()
			<< " / " << time_region << std::endl;

		pcurr = pnext;
	}


	#if 0
	trig.Mark(reinterpret_cast<unsigned char *>(buf), len, 1);
	std::vector<uint32_t> *nhits = trig.Scan();
	std::cout << "# ";
	for (auto hit : *nhits) std::cout << " " << hit;
	std::cout << std::endl;
	std::cout << "Nhits: " << std::dec << nhits->size()
		<< " / " << time_region << std::endl;
	#endif

	return 0;
}
#endif
