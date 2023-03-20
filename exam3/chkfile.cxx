/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
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



bool CheckData(char *buf)
{
	Filter::Header *pflt = reinterpret_cast<Filter::Header *>(buf);
	//uint64_t size = pflt->length;

	if (pflt->magic == Filter::Magic) {
		std::cout << "#FLT Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  pflt->magic
			<< " len: " << std::dec << std::setw(8) <<  pflt->length
			<< " N trigs: " << std::setw(8) <<  pflt->numTrigs
			<< " Id: " << std::setw(8) << pflt->workerId
			<< " elapse: " << std::dec <<  pflt->elapseTime
			<< std::endl;
	} else {
		std::cout << "#E Not FLT Header : " << pflt->magic << std::endl;
		return false;
	}

	TimeFrame::Header *ptf = reinterpret_cast<TimeFrame::Header *>(
		buf + sizeof(struct Filter::Header));
	if (ptf->magic == TimeFrame::Magic) {
		std::cout << "#TF Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  ptf->timeFrameId
			<< " Nsource: " << std::setw(8) << std::setfill('0') <<  ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;
	} else {
		std::cout << "#E Not TF Header : " << ptf->magic << std::endl;
		return false;
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

int reader(char *file)
{
	char *fileheader = new char[512];
	char *top = new char[1024*1024];
	struct Filter::Header *flt = reinterpret_cast<struct Filter::Header *>(top);
	char *data = top + sizeof(struct Filter::Header);
	std::ifstream ifs(file);

	ifs.read(fileheader, sizeof(class nestdaq::FileSinkHeaderBlock));	
	while (true) {
		ifs.read(top, sizeof(struct Filter::Header));
		if (!ifs) break;
		ifs.read(data, (flt->length - sizeof(struct Filter::Header)));
		if (ifs.eof()) break;
		if (ifs.gcount() == 0) break;
		if (!ifs) break;

		CheckData(top);
	}

	delete [] fileheader;
	delete [] top;

	return 0;
}

int main(int argc, char* argv[])
{
	
	reader(argv[1]);

	return 0;
}
