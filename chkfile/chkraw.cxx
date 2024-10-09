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
#include <filesystem>
#include <vector>

#include <cstring>

//#include "HulStrTdcData.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"
#include "HeartbeatFrameHeader.h"
#include "UnpackTdc.h"

//#include "FileSinkHeaderBlock.h"
#include "FileSinkHeader.h"
#include "FileSinkTrailer.h"


int CheckData(uint64_t dword, uint32_t fe_type, uint32_t fe_id)
{
	//char *pdata = reinterpret_cast<char *>(&dword);
	unsigned int dtype = (dword & 0xfc00'0000'0000'0000) >> 58;

	std::cout << "# " << std::hex << std::setw(2) << std::setfill('0')
		<< dword << " : ";

	if ((dtype == TDC64H::T_TDC_L) || (dtype == TDC64H::T_TDC_T)
	 || (dtype == TDC64H::T_THR1_START) || (dtype == TDC64H::T_THR1_END)
	 || (dtype == TDC64H::T_THR2_START) || (dtype == TDC64H::T_THR2_END)) {
		if (dtype == TDC64H::T_TDC_L)     std::cout << "TDC_L ";
		if (dtype == TDC64H::T_TDC_T)     std::cout << "TDC_T ";
		if (dtype == TDC64H::T_THR1_START) std::cout << "THR1S ";
		if (dtype == TDC64H::T_THR1_END)   std::cout << "THR1E ";
		if (dtype == TDC64H::T_THR2_START) std::cout << "THR2S ";
		if (dtype == TDC64H::T_THR2_END)   std::cout << "THR2E ";
		if (fe_type == SubTimeFrame::TDC64H) {
			struct TDC64H::tdc64 tdc;
			TDC64H::Unpack(dword, &tdc);
			std::cout << "H :"
				<< " FE: " << std::dec << std::setw(3) << (fe_id & 0xff)
				<< " CH: " << std::dec << std::setw(3) << tdc.ch
				<< " TDC: " << std::setw(8) << tdc.tdc
				<< " ToT: " << std::setw(7) << tdc.tot << std::endl;
		} else
		if (fe_type == SubTimeFrame::TDC64L) {
			struct TDC64L::tdc64 tdc;
			TDC64L::Unpack(dword, &tdc);
			std::cout << "L :"
				<< " FE: " << std::dec << std::setw(3) << (fe_id & 0xff)
				<< " CH: " << std::dec << std::setw(3) << tdc.ch
				<< " TDC: " << std::setw(8) << tdc.tdc
				<< " ToT: " << std::setw(7) << tdc.tot << std::endl;
		} else {
			std::cout << "UNKNOWN"<< std::endl;
		}

	} else
	if (dtype == TDC64H::T_HB) {
		std::cout << "Heartbeat ";

		struct TDC64H::tdc64 tdc;
		TDC64H::Unpack(dword, &tdc);
		int hbflag = tdc.flag;
	        if (hbflag > 0) {
			if ((hbflag & 0x200) == 0x200)
				std::cout << "#E HB Data lost ";
			if ((hbflag & 0x100) == 0x100)
				std::cout << "#E HB Data confiliction ";
			if ((hbflag & 0x080) == 0x080)
				std::cout << "#E HB LFN mismatch ";
			if ((hbflag & 0x040) == 0x040)
				std::cout << "#E HB GFN mismatch ";
			if ((hbflag & 0x020) == 0x020)
				std::cout << "#E HB IThr Tyep1 ";
			if ((hbflag & 0x010) == 0x010)
				std::cout << "#E HB IThr Tyep2 ";
			if ((hbflag & 0x008) == 0x008)
				std::cout << "#E HB OThr ";
			if ((hbflag & 0x004) == 0x004)
				std::cout << "#E HB HBF Thr ";
       		}
		std::cout << std::endl;

	} else
	if (dtype == TDC64H::T_SPL_START) {
		std::cout << "SPILL Start" << std::endl;
	} else
	if (dtype == TDC64H::T_SPL_END) {
		std::cout << "SPILL End" << std::endl;
	} else
	if (dtype == TDC64H_V3::T_HB2) {
		std::cout << "Heartbeat2 ";
		struct TDC64H_V3::tdc64 tdc;
		TDC64H_V3::Unpack(dword, &tdc);
		int gsize = tdc.genesize;
		int tsize = tdc.transize;
		if (gsize > 0) std::cout << " GenSize: " << std::dec << gsize;
		if (tsize > 0) std::cout << " TransSize: " << std::dec << tsize;
		std::cout << std::endl;
	} else {
		std::cout << "Unknown data" << std::endl;
	}


	return 0;
}

bool CheckBlock(char *buf, int size)
{
	uint64_t *pdata = reinterpret_cast<uint64_t *>(buf);
	//Filter::Header *pflt = reinterpret_cast<Filter::Header *>(buf);
	//TimeFrame::Header *ptf = reinterpret_cast<TimeFrame::Header *>(buf);
	//SubTimeFrame::Header *pstf = reinterpret_cast<SubTimeFrame::Header *>(buf);

	//uint32_t N_src = 0;
	if (*pdata == TimeFrame::MAGIC) {
		TimeFrame::Header *ptf = reinterpret_cast<TimeFrame::Header *>(pdata);
		//std::string smagic = std::string(
		//	reinterpret_cast<char *>(&(ptf->magic))).substr(0,8);
		char cmagic[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
		for (int i = 0 ; i < 8 ; i++)
			cmagic[i] = (reinterpret_cast<char *>(&(ptf->magic)))[i];
		std::cout << "TF  "
			<< cmagic << "("
			<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< ")"
			<< " id: " << std::setw(6) << std::setfill('0') <<  ptf->timeFrameId
			<< " Nsrc: " << std::setw(4) << std::setfill('0') <<  ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;
		//N_src = ptf->numSource;
		pdata = reinterpret_cast<uint64_t *>(
			reinterpret_cast<char *>(pdata) + sizeof(struct TimeFrame::Header));
	} else {
		std::cout << "#E Not TF Header : " << std::hex << *pdata << std::endl;
		//return false;
	}


	if (*pdata == Filter::MAGIC) {
		Filter::Header *pflt = reinterpret_cast<Filter::Header *>(pdata);
		//std::string smagic = std::string(
		//	reinterpret_cast<char *>(&(pflt->magic))).substr(0,8);
		char cmagic[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
		for (int i = 0 ; i < 8 ; i++)
			cmagic[i] = (reinterpret_cast<char *>(&(pflt->magic)))[i];
		std::cout << "FLT "
			<< cmagic << "("
			<< std::hex << std::setw(16) << std::setfill('0') << pflt->magic
			<< ")"
			<< " len: " << std::dec << std::setw(6) <<  pflt->length
			<< " Ntrig: " << std::setw(4) <<  pflt->numTrigs
			<< " Id: " << std::setw(4) << pflt->workerId
			<< " elapse: " << std::dec << std::setw(6) << pflt->elapseTime
			<< std::endl;
		pdata = reinterpret_cast<uint64_t *>(
			reinterpret_cast<char *>(pdata) + sizeof(struct Filter::Header));
		
	} else {
		//std::cout << "#E Not FLT Header : " << std::hex << *pdata << std::endl;
		//return false;
	}


	uint32_t fe_type = 0;
	uint32_t fe_id = 0;
	while (true) {
		if (*pdata == SubTimeFrame::MAGIC) {
			SubTimeFrame::Header *pstf
				= reinterpret_cast<SubTimeFrame::Header *>(pdata);
			char cmagic[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
			for (int i = 0 ; i < 8 ; i++)
				cmagic[i] = (reinterpret_cast<char *>(&(pstf->magic)))[i];
			std::cout << "STF "
				<< cmagic << "("
				<< std::hex << std::setw(16) << std::setfill('0') << pstf->magic
				<< ")"
				<< " id: " << std::setw(6) << std::setfill('0') <<  pstf->timeFrameId
				//<< " res: " << std::setw(8) << std::setfill('0') <<  pstf->reserved
				<< " Type: " << std::setw(4) << std::setfill('0') <<  pstf->femType
				<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->femId
				//<< std::endl << "# "
				<< " len: " << std::dec <<  pstf->length
				<< " nMsg: " << std::dec <<  pstf->numMessages
				//<< std::endl << "# "
				//<< " Ts: " << std::dec << pstf->time_sec
				//<< " Tus: " << std::dec << pstf->time_usec
				<< std::endl;
	
			fe_type = pstf->femType;
			fe_id = pstf->femId;
			pdata = reinterpret_cast<uint64_t *>(
				reinterpret_cast<char *>(pdata) + sizeof(struct SubTimeFrame::Header));
	
		} else
		if (*pdata == HeartbeatFrame::MAGIC) {
			HeartbeatFrame::Header *phbf
				= reinterpret_cast<HeartbeatFrame::Header *>(pdata);
			char cmagic[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
			for (int i = 0 ; i < 8 ; i++)
				cmagic[i] = (reinterpret_cast<char *>(&(phbf->magic)))[i];
			std::cout << "HBF " << cmagic
				<< "(" << std::hex << std::setw(16) << std::setfill('0')
				<< phbf->magic
				<< ")"
				<< " Len: " << phbf->length
				<< " hLen: " << phbf->hLength
				<< " Type: " << phbf->type
				<< std::endl;
			pdata = reinterpret_cast<uint64_t *>(
				reinterpret_cast<char *>(pdata) + sizeof(struct HeartbeatFrame::Header));

		} else {
			CheckData(*pdata, fe_type, fe_id);
			pdata++;
		}

		///chec


		if ((reinterpret_cast<char *>(pdata) - buf) > size) {
			std::cout << "--------" << std::endl;
			break;
		}
	}

#if 0
	unsigned int msize = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());
	uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(pdata));


	if (msg_magic == Filter::MAGIC) {
		Filter::Header *pflt
			= reinterpret_cast<Filter::Header *>(pdata);
		std::cout << "#FLT Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  pflt->magic
			<< " len: " << std::dec << std::setw(8) <<  pflt->length
			<< " N trigs: " << std::setw(8) <<  pflt->numTrigs
			<< " Id: " << std::setw(8) << pflt->workerId
			<< " elapse: " << std::dec <<  pflt->elapseTime
			<< std::endl;

	} else if (msg_magic == TimeFrame::MAGIC) {
		TimeFrame::Header *ptf
			= reinterpret_cast<TimeFrame::Header *>(pdata);
		std::cout << "#TF Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  ptf->timeFrameId
			<< " Nsource: " << std::setw(8) << std::setfill('0') <<  ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;

	} else if (msg_magic == SubTimeFrame::MAGIC) {
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

			if        ((pdata[j + 7] & 0xfc) == (TDC64H::T_TDC << 2)) {
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
				std::cout << "Heart beat" << std::endl;

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

int reader(char *file)
{
	char *fileheader = new char[sizeof(struct FileSinkHeader::Header)];
	char *filetrailer = new char[sizeof(struct FileSinkTrailer::Trailer)];
	struct FileSinkHeader::Header *fsh
		= reinterpret_cast<struct FileSinkHeader::Header *>(fileheader);

	bool exist_fsh = false;
	std::ifstream ifs(file);
	ifs.read(fileheader, sizeof(struct FileSinkHeader::Header));	
	if (!ifs) return 1;
	std::cout << "File: " << file << " Header Size: " << ifs.gcount() << std::endl;
	std::uintmax_t file_size = std::filesystem::file_size(file);
	std::cout << "File size is " << file_size << " bytes." << std::endl;

	if (fsh->magic == FileSinkHeader::MAGIC) {
		char cmagic[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
		for (int i = 0 ; i < 8 ; i++) cmagic[i] = (reinterpret_cast<char *>(&(fsh->magic)))[i];
		std::cout << "MAGIC: " << cmagic
			<< "(" << std::hex << fsh->magic << std::dec << ")" << std::endl
			<< " Size: " << fsh->size
			<< " Type: " << fsh->fairMQDeviceType
			<< " Run: " << fsh->runNumber
			<< std::endl
			<< " Start: " << ctime(&(fsh->startUnixtime))
			<< " Stop : " << ctime(&(fsh->stopUnixtime));;;;
		std::cout << "Comment: " << fsh->comments << std::endl;
		exist_fsh = true;
	} else {
		exist_fsh = false;
	}

	struct FileSinkTrailer::Trailer *fst
		= reinterpret_cast<struct FileSinkTrailer::Trailer *>(filetrailer);
	std::filesystem::path path_to_file(file);

	ifs.seekg(-1 * sizeof(struct FileSinkTrailer::Trailer), std::ios_base::end);
	ifs.read(filetrailer, sizeof(struct FileSinkTrailer::Trailer));	
	if (!ifs) return 1;

	if (fst->magic == FileSinkTrailer::MAGIC) {
		char cmagic[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
		for (int i = 0 ; i < 8 ; i++) cmagic[i] = (reinterpret_cast<char *>(&(fst->magic)))[i];
		std::cout << "MAGIC: " << cmagic
			<< "(" << std::hex << fst->magic << std::dec << ")" << std::endl
			<< " Size: " << fst->size
			<< " Type: " << fst->fairMQDeviceType
			<< " Run: " << fst->runNumber
			<< std::endl
			<< " Start: " << ctime(&(fst->startUnixtime))
			<< " Stop : " << ctime(&(fst->stopUnixtime));
		std::cout << "Comment: " << fst->comments << std::endl;
	} else {
		std::cout << "No file trailer" << std::endl;
	}

	if (exist_fsh) {
		ifs.seekg(sizeof(struct FileSinkHeader::Header), std::ios_base::beg);
	} else {
		ifs.seekg(0, std::ios_base::beg);
	}

	unsigned int bufsize = 1024*1024;
	std::vector<char> buf;
	buf.resize(bufsize);
	char *top = buf.data();
	struct Filter::Header *pflt = reinterpret_cast<struct Filter::Header *>(top);
	struct TimeFrame::Header *ptf = reinterpret_cast<struct TimeFrame::Header *>(top);
	struct SubTimeFrame::Header *pstf = reinterpret_cast<struct SubTimeFrame::Header *>(top);
	struct FileSinkTrailer::Trailer *pfst = reinterpret_cast<struct FileSinkTrailer::Trailer *>(top);
	char *body = nullptr;

	while (true) {
		unsigned int tlength = 0;
		unsigned int hlength = 0;
		ifs.read(top, sizeof(uint64_t));
		if (!ifs) break;
		if (*(reinterpret_cast<uint64_t *>(top)) == Filter::MAGIC) {
			hlength = sizeof(struct Filter::Header);
			ifs.read(top + sizeof(uint64_t),
				 hlength - sizeof(uint64_t));
			body = top + hlength;
			tlength = pflt->length;
		} else
		if (*(reinterpret_cast<uint64_t *>(top)) == TimeFrame::MAGIC) {
			hlength = sizeof(struct TimeFrame::Header);
			ifs.read(top + sizeof(uint64_t),
				 hlength - sizeof(uint64_t));
			body = top + hlength;
			tlength = ptf->length;
		} else
		if (*(reinterpret_cast<uint64_t *>(top)) == SubTimeFrame::MAGIC) {
			hlength = sizeof(struct SubTimeFrame::Header);
			ifs.read(top + sizeof(uint64_t),
				 hlength - sizeof(uint64_t));
			body = top + hlength;
			tlength = pstf->length;
		} else
		if (*(reinterpret_cast<uint64_t *>(top)) == FileSinkTrailer::MAGIC) {
			hlength = sizeof(struct FileSinkTrailer::Trailer);
			ifs.read(top + sizeof(uint64_t),
				 hlength - sizeof(uint64_t));
			body = top + hlength;
			tlength = pfst->size;
		} else {
			std::cout << "Unknown Header "
				<< std::hex << *(reinterpret_cast<uint64_t *>(top))
				<< std::endl;
			break;
		}
		//ifs.read(top, sizeof(struct Filter::Header));
		if (!ifs) break;
		if (tlength > bufsize) {
			std::vector<char> t(buf.begin(),
				buf.begin() + hlength);
			bufsize = (tlength / 1024) * 2048;
			buf.resize(bufsize);
			std::copy(t.begin(),
				t.begin() + hlength,
				buf.begin());
			top = buf.data();
			body = top + hlength;
			//flt = reinterpret_cast<struct Filter::Header *>(top);
		}
		ifs.read(body, tlength - hlength);
		int readsize = ifs.gcount();
		if (ifs.eof()) break;
		if (readsize == 0) break;
		if (!ifs) break;

		if (! CheckBlock(top, readsize)) break;
	}

	delete [] fileheader;
	delete [] filetrailer;

	return 0;
}

int readstdin()
{
	const int WORDSIZE = 8;
	static char buf[128];
	static char trashbox[128];

	uint32_t fe_id = 0;
	uint32_t fe_type = SubTimeFrame::TDC64L;

	while(true) {
		std::cin.read(buf, WORDSIZE);
		if (std::cin.eof()) break;
		uint64_t head = *reinterpret_cast<uint64_t *>(buf);
		if (head == FileSinkHeader::MAGIC) {
			std::cin.read(trashbox, sizeof(FileSinkHeader::Header) - WORDSIZE);
			std::cout << "FS" << std::endl;
			continue;
		}
		if (head == SubTimeFrame::MAGIC) {
			std::cin.read(&buf[WORDSIZE], sizeof(SubTimeFrame::Header) - WORDSIZE);
			SubTimeFrame::Header *pstf
				= reinterpret_cast<SubTimeFrame::Header *>(buf);
			char cmagic[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
			for (int i = 0 ; i < 8 ; i++)
				cmagic[i] = (reinterpret_cast<char *>(&(pstf->magic)))[i];
			std::cout << "STF "
				<< cmagic << "("
				<< std::hex << std::setw(16) << std::setfill('0') << pstf->magic
				<< ")"
				<< " id: " << std::setw(6) << std::setfill('0') <<  pstf->timeFrameId
				<< " Type: " << std::setw(4) << std::setfill('0') <<  pstf->femType
				<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->femId
				<< " len: " << std::dec <<  pstf->length
				<< " nMsg: " << std::dec <<  pstf->numMessages
				//<< " Ts: " << std::dec << pstf->time_sec
				//<< " Tus: " << std::dec << pstf->time_usec
				<< std::endl;
	
			fe_type = pstf->femType;
			fe_id = pstf->femId;

			continue;
		}
		if (head == TimeFrame::MAGIC) {
			std::cin.read(trashbox, sizeof(TimeFrame::Header) - WORDSIZE);
			std::cout << "TF" << std::endl;
			continue;
		}
		if (head == Filter::MAGIC) {
			std::cin.read(trashbox, sizeof(Filter::Header) - WORDSIZE);
			std::cout << "FLT" << std::endl;
			continue;
		}
		if (head == HeartbeatFrame::MAGIC) {
			std::cin.read(trashbox, sizeof(HeartbeatFrame::Header) - WORDSIZE);
			std::cout << "HBF" << std::endl;
			continue;
		}

		uint64_t dword = *reinterpret_cast<uint64_t *>(buf);
		CheckData(dword, fe_type, fe_id);

	}


	return 0;
}

int main(int argc, char* argv[])
{

	if (argc > 1) {
		reader(argv[1]);
	} else {
		readstdin();
	}

	return 0;
}
