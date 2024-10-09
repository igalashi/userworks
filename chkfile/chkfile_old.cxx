/********************************************************************************
 *                                                                              *
 ********************************************************************************/

//#include <fairmq/Device.h>
//#include <fairmq/runDevice.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

#include "HulStrTdcData.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"
#include "UnpackTdc.h"

#include "FileSinkHeaderBlock.h"


bool g_flag_dump_module = false;

int DecodeModule(char* pdata, int size, int type)
{
	uint64_t *p6data = reinterpret_cast<uint64_t *>(pdata);

	for (int i = 0 ; i < size ; i += sizeof(uint64_t)) {
		std::cout << "# "
			<< std::setw(6) << std::setfill(' ') << i << " : "
			<< std::hex << std::setw(16) << std::setfill('0')
			<< p6data[(i / sizeof(uint64_t))] << " : ";

		if	((pdata[i + 7] & 0xfc) == (TDC64H::T_TDC << 2)) {
			std::cout << "TDC ";
			uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[i]));
			if (type == SubTimeFrame::TDC64H) {
				struct TDC64H::tdc64 tdc;
				TDC64H::Unpack(*dword, &tdc);
				std::cout << "H :" << std::setfill(' ')
					<< " CH: " << std::dec << std::setw(3) << tdc.ch
					<< " TDC: " << std::setw(9) << tdc.tdc << std::endl;
			} else
			if (type == SubTimeFrame::TDC64L) {
				struct TDC64L::tdc64 tdc;
				TDC64L::Unpack(*dword, &tdc);
				std::cout << "L :" << std::setfill(' ')
					<< " CH: " << std::dec << std::setw(3) << tdc.ch
					<< " TDC: " << std::setw(9) << tdc.tdc << std::endl;
			} else {
				std::cout << "UNKNOWN"<< std::endl;
			}

		} else if ((pdata[i + 7] & 0xfc) == (TDC64H::T_HB << 2)) {
			std::cout << "Hart beat" << std::endl;

			uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[i]));
			struct TDC64H::tdc64 tdc;
			TDC64H::Unpack(*dword, &tdc);
			int hbflag = tdc.flag;
			if (hbflag > 0) {
				if ((hbflag & 0x200) == 0x200)
					std::cout << "#E HB Data lost" << std::endl;
				if ((hbflag & 0x100) == 0x100)
					std::cout << "#E HB Data confiliction" << std::endl;
				if ((hbflag & 0x080) == 0x080)
					std::cout << "#E HB LFN mismatch" << std::endl;
				if ((hbflag & 0x040) == 0x040)
					std::cout << "#E HB GFN mismatch" << std::endl;
			}

		} else if ((pdata[i + 7] & 0xfc) == (TDC64H::T_S_START << 2)) {
			std::cout << "SPILL Start" << std::endl;
		} else if ((pdata[i + 7] & 0xfc) == (TDC64H::T_S_END << 2)) {
			std::cout << "SPILL End" << std::endl;
		} else {
			std::cout << std::endl;
		}
	}

	return 0;
}

bool CheckData(char *buf)
{
	class nestdaq::FileSinkHeaderBlock *fh
		= reinterpret_cast<class nestdaq::FileSinkHeaderBlock *>(buf);
	if (fh->fMagic == nestdaq::FileSinkHeaderBlock::kMagic) {
		char cmagic[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		uint64_t *u6magic = reinterpret_cast<uint64_t *>(cmagic);
		*u6magic = fh->fMagic;
		//std::cout << "FILE : " << std::hex << fh->fMagic
		std::cout << "FILE : " << cmagic
			<< " Len: " <<  std::dec << fh->fFileSinkHeaderBlockSize
			<< " Run: " << std::dec << fh->fRunNumber
			<< std::endl;
		std::cout << "Start: " << ctime(&(fh->fStartUnixtime))
			<< " Stop: " << ctime(&(fh->fStopUnixtime));
		std::cout << "Comment: " << fh->fComments << std::endl;

		return true;
	}
	
	int data_size = -1;
	bool isFilter = false;
	Filter::Header *pflt = reinterpret_cast<Filter::Header *>(buf);
	if (pflt->magic == Filter::Magic) {
		isFilter = true;
		char cmagic[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		uint64_t *u6magic = reinterpret_cast<uint64_t *>(cmagic);
		*u6magic = pflt->magic;
		std::cout << "FLT Header "
			//<< std::hex << std::setw(16) << std::setfill('0') <<  pflt->magic
			<< cmagic
			<< std::setfill(' ')
			<< " len: " << std::dec << std::setw(8) <<  pflt->length
			<< " Ntrigs: " << std::setw(4) <<  pflt->numTrigs
			<< " Id: " << std::setw(4) << pflt->workerId
			<< " elapse: " << std::dec <<  pflt->elapseTime
			<< std::endl;
		data_size = pflt->length;
	} else {
		std::cout << "#E Not FLT Header : " << pflt->magic << std::endl;
		return false;
	}

	bool isTimeFrame = false;
	TimeFrame::Header *ptf = reinterpret_cast<TimeFrame::Header *>(buf
		+ (isFilter ? sizeof(struct Filter::Header) : 0));
	if (ptf->magic == TimeFrame::Magic) {
		isTimeFrame = true;
		char cmagic[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		uint64_t *u6magic = reinterpret_cast<uint64_t *>(cmagic);
		*u6magic = ptf->magic;
		std::cout << "TF  Header "
			//<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< cmagic
			<< " id: " << std::hex << std::setw(8) << std::setfill('0') << ptf->timeFrameId
			<< std::dec << std::setfill(' ')
			<< " Nsource: " << std::setw(4) << ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;
		if (data_size < 0) data_size = ptf->length;
	} else {
		std::cout << "#E Not TF Header : " << ptf->magic << std::endl;
		return false;
	}

	int istf = 0;
	char *pdata = buf
		+ (isFilter ? sizeof(struct Filter::Header) : 0)
		+ (isTimeFrame ? sizeof(struct TimeFrame::Header) : 0);
	while (true) {

		SubTimeFrame::Header *pstf = reinterpret_cast<SubTimeFrame::Header *>(pdata);
		char cmagic[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		uint64_t *u6magic = reinterpret_cast<uint64_t *>(cmagic);
		*u6magic = pstf->magic;
		if (pstf->magic == SubTimeFrame::Magic) {
			std::cout << "STF Header " << std::dec
				<< std::setw(2) << std::setfill('0') << istf << " " 
				//<< std::hex << std::setw(8) << std::setfill('0') <<  pstf->magic
				<< cmagic
				<< " id: " << std::setw(6) << std::setfill('0') <<  pstf->timeFrameId
				//<< " res: " << std::setw(8) << std::setfill('0') <<  pstf->reserved
				<< " Type: " << std::setw(4) << std::setfill('0') <<  pstf->Type
				<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->FEMId
				//<< std::endl << "  "
				<< " Len: " << std::dec <<  pstf->length
				<< " Nmsg: " << std::dec <<  pstf->numMessages
				// << std::endl << "  "
				// << " Ts: " << std::dec << pstf->time_sec
				// << " Tus: " << std::dec << pstf->time_usec
				<< std::endl;

			char *pbody =pdata + sizeof(struct SubTimeFrame::Header);
			int bsize = pstf->length - sizeof(struct SubTimeFrame::Header);
			int type = pstf->Type;
			if (g_flag_dump_module) DecodeModule(pbody, bsize, type);

			pdata += pstf->length;
		} else {
			//std::cout << "# Not STF Header : " << cmagic << std::endl;
			//std::cout << "# Not STF Header : "
			//	<< std::hex << std::setw(16) << std::setfill('0') << ptf->magic
			//	<< std::endl;
			break;
		}
		if ((pdata - buf) > data_size) {
			std::cout << "#E Data Size exceed : "
				<< pdata - buf << " " << data_size << std::endl;
			break;
		}
		istf++;
	}



#if 0
	unsigned int msize = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());
	uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(pdata));


	if (msg_magic == Filter::Magic) {
		Filter::Header *pflt
			= reinterpret_cast<Filter::Header *>(pdata);
		std::cout << "#FLT Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  pflt->magic
			<< " len: " << std::dec << std::setw(8) <<  pflt->length
			<< " N trigs: " << std::setw(8) <<  pflt->numTrigs
			<< " Id: " << std::setw(8) << pflt->workerId
			<< " elapse: " << std::dec <<  pflt->elapseTime
			<< std::endl;

	} else if (msg_magic == TimeFrame::Magic) {
		TimeFrame::Header *ptf
			= reinterpret_cast<TimeFrame::Header *>(pdata);
		std::cout << "#TF Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  ptf->timeFrameId
			<< " Nsource: " << std::setw(8) << std::setfill('0') <<  ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;

	} else if (msg_magic == SubTimeFrame::Magic) {
		SubTimeFrame::Header *pstf
			= reinterpret_cast<SubTimeFrame::Header *>(pdata);
		std::cout << "#STF Header "
			<< std::hex << std::setw(8) << std::setfill('0') <<  pstf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  pstf->timeFrameId
			//<< " res: " << std::setw(8) << std::setfill('0') <<  pstf->reserved
			<< " Type: " << std::setw(8) << std::setfill('0') <<  pstf->Type
			<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->FEMId
			<< std::endl << "# "
			<< " len: " << std::dec <<  pstf->length
			<< " nMsg: " << std::dec <<  pstf->numMessages
			<< std::endl << "# "
			<< " Ts: " << std::dec << pstf->time_sec
			<< " Tus: " << std::dec << pstf->time_usec
			<< std::endl;

		fFe_type = pstf->Type;

	} else {

		#if 1
		for (unsigned int j = 0 ; j < msize ; j += 8) {
			std::cout << "# " << std::setw(8) << j << " : "
				<< std::hex << std::setw(2) << std::setfill('0')
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 7]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 6]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 5]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 4]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 3]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 2]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 1]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 0]) << " : ";

			if	((pdata[j + 7] & 0xfc) == (TDC64H::T_TDC << 2)) {
				std::cout << "TDC ";
				uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[j]));
				if (fFe_type == SubTimeFrame::TDC64H) {
					struct TDC64H::tdc64 tdc;
					TDC64H::Unpack(*dword, &tdc);
					std::cout << "H :"
						<< " CH: " << std::dec << std::setw(3) << tdc.ch
						<< " TDC: " << std::setw(7) << tdc.tdc << std::endl;
				} else
				if (fFe_type == SubTimeFrame::TDC64L) {
					struct TDC64L::tdc64 tdc;
					TDC64L::Unpack(*dword, &tdc);
					std::cout << "L :"
						<< " CH: " << std::dec << std::setw(3) << tdc.ch
						<< " TDC: " << std::setw(7) << tdc.tdc << std::endl;
				} else {
					std::cout << "UNKNOWN"<< std::endl;
				}

			} else if ((pdata[j + 7] & 0xfc) == (TDC64H::T_HB << 2)) {
				std::cout << "Hart beat" << std::endl;

				uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[j]));
				struct TDC64H::tdc64 tdc;
				TDC64H::Unpack(*dword, &tdc);
				int hbflag = tdc.flag;
				if (hbflag > 0) {
					if ((hbflag & 0x200) == 0x200)
						std::cout << "#E HB Data lost" << std::endl;
					if ((hbflag & 0x100) == 0x100)
						std::cout << "#E HB Data confiliction" << std::endl;
					if ((hbflag & 0x080) == 0x080)
						std::cout << "#E HB LFN mismatch" << std::endl;
					if ((hbflag & 0x040) == 0x040)
						std::cout << "#E HB GFN mismatch" << std::endl;
				}

			} else if ((pdata[j + 7] & 0xfc) == (TDC64H::T_S_START << 2)) {
				std::cout << "SPILL Start" << std::endl;
			} else if ((pdata[j + 7] & 0xfc) == (TDC64H::T_S_END << 2)) {
				std::cout << "SPILL End" << std::endl;
			} else {
				std::cout << std::endl;
			}
		}

		#else
		std::cout << "#Unknown Header " << std::hex << msg_magic << std::endl;
		#endif

	}
#endif

	return true;
}


int reader(const char *file)
{
	char *fileheader = new char[512];
	int bsize = 1024 * 8;
	char *top = new char[bsize];
	struct Filter::Header *flt = reinterpret_cast<struct Filter::Header *>(top);
	char *data = top + sizeof(struct Filter::Header);
	std::ifstream ifs(file);

	ifs.read(fileheader, sizeof(class nestdaq::FileSinkHeaderBlock));	
	class nestdaq::FileSinkHeaderBlock *fh
		= reinterpret_cast<class nestdaq::FileSinkHeaderBlock *>(
			fileheader);
	if (fh->fMagic == nestdaq::FileSinkHeaderBlock::kMagic) {
		char cmagic[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		uint64_t *u6magic = reinterpret_cast<uint64_t *>(cmagic);
		*u6magic = fh->fMagic;
		//std::cout << "FILE : " << std::hex << fh->fMagic
		std::cout << "FILE : " << cmagic
			<< " Len: " <<  std::dec << fh->fFileSinkHeaderBlockSize
			<< " Run: " << std::dec << fh->fRunNumber
			<< std::endl;
		std::cout << "Start: " << ctime(&(fh->fStartUnixtime))
			<< "Stop : " << ctime(&(fh->fStopUnixtime));
		std::cout << "Comment: " << fh->fComments << std::endl;
	} else {
		std::cout << "#E FileSinkHadder Err. "
			<< std::hex << fh->kMagic << std::endl;
	}
	
	while (true) {
		ifs.read(top, sizeof(struct Filter::Header));
		if (!ifs) break;

		if (flt->length > (bsize - sizeof(struct Filter::Header))) {
			bsize = (((flt->length) + sizeof(struct Filter::Header) + 128) / 128) * 128;
			char *new_top  = new char[bsize];
			memcpy(new_top, top, sizeof(struct Filter::Header));
			delete [] top;
			top = new_top;
			data = top + sizeof(struct Filter::Header);
			flt = reinterpret_cast<struct Filter::Header *>(top);
		}

		ifs.read(data, (flt->length - sizeof(struct Filter::Header)));
		if (!ifs) break;
		if (ifs.eof()) break;
		if (ifs.gcount() == 0) break;

		CheckData(top);
	}

	delete [] fileheader;
	delete [] top;

	return 0;
}

int main(int argc, char* argv[])
{
	std::string filename;
	for (int i = 1 ; i < argc ; i++) {
		std::string ss(argv[i]);
		if (ss == "--dump-module") {
			g_flag_dump_module = true;
		} else
		if (ss[0] != '-') {
			filename = ss;
		}
	}
	
	reader(filename.c_str());

	return 0;
}
