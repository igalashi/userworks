/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *									      *
 *	      This software is distributed under the terms of the	     *
 *	      GNU Lesser General Public Licence (LGPL) version 3,	     *
 *		  copied verbatim in the file "LICENSE"		       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>

#include <unordered_map>
#include <unordered_set>

#include "MessageUtil.h"

#include "HulStrTdcData.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"

#include "trigger.cxx"


//std::atomic<int> gQdepth = 0;

namespace bpo = boost::program_options;

struct FltCoin : fair::mq::Device
{
	struct OptionKey {
		static constexpr std::string_view InputChannelName  {"in-chan-name"};
		static constexpr std::string_view OutputChannelName  {"out-chan-name"};
		static constexpr std::string_view DataSupress  {"data-supress"};
	};

	FltCoin()
	{
		// register a handler for data arriving on "data" channel
		//OnData("in", &FltCoin::HandleData);

		fTrig = new Trigger();
	}

	void InitTask() override;
#if 0
	void InitTask() override
	{
		using opt = OptionKey;

		// Get the fMaxIterations value from the command line options (via fConfig)
		fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
		fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
		fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());
		LOG(info) << "InitTask: Input Channel : " << fInputChannelName
			<< " Output Channel : " << fOutputChannelName;

		//OnData(fInputChannelName, &FltCoin::HandleData);
	}
#endif

#if 0
	bool HandleData(fair::mq::MessagePtr& msg, int)
	{
		LOG(info) << "Received: \""
			<< std::string(static_cast<char*>(msg->GetData()), msg->GetSize())
			<< "\"";

		if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
			LOG(info) << "Configured maximum number of iterations reached."
				<< " Leaving RUNNING state.";
			return false;
		}

		// return true if you want the handler to be called again
		// (otherwise return false go to the Ready state)
		return true;
	}
#endif

	
	bool ConditionalRun() override;
	void PostRun() override;

	bool CheckData(fair::mq::MessagePtr&);

private:
	int IsHartBeat(uint64_t, uint32_t);

	std::string fInputChannelName;
	std::string fOutputChannelName;
	std::string fName;
	uint32_t fId {0};
	Trigger *fTrig;
	bool fIsDataSupress = true;

	#if 0
	uint64_t fMaxIterations = 0;
	uint64_t fNumIterations = 0;

	struct STFBuffer {
		FairMQParts parts;
		std::chrono::steady_clock::time_point start;
	};
	std::unordered_map<uint32_t, std::vector<STFBuffer>> fTFBuffer;
	std::unordered_set<uint64_t> fDiscarded;
	int fNumSource = 0;
	#endif


};


void FltCoin::InitTask()
{
	using opt = OptionKey;

	// Get the fMaxIterations value from the command line options (via fConfig)
	//fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
	fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
	fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());

	LOG(info) << "InitTask: Input Channel : " << fInputChannelName
		<< " Output Channel : " << fOutputChannelName;

        fName = fConfig->GetProperty<std::string>("id");
        std::istringstream ss(fName.substr(fName.rfind("-") + 1));
        ss >> fId;

	std::string sIsDataSupress = fConfig->GetValue<std::string>(opt::DataSupress.data());
	if (sIsDataSupress == "true") {
		fIsDataSupress = true;
	} else {
		fIsDataSupress = false;
	}
	LOG(info) << "InitTask: DataSupress : " << fIsDataSupress;

	fTrig->SetTimeRegion(1024 * 256);
	fTrig->Entry(0xc0a802a8, 2, 0);
	fTrig->Entry(0xc0a802a8, 4, 0);

}

bool FltCoin::CheckData(fair::mq::MessagePtr &msg)
{
	unsigned int msize = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());
	uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(pdata));

	static int fe_type = 0;

	std::cout << "#Msg Top(8B): " << std::hex << msg_magic
		<< " Size: " << std::dec << msize << std::endl;

	if (msg_magic == TimeFrame::Magic) {
		TimeFrame::Header *ptf = reinterpret_cast<TimeFrame::Header *>(pdata);
		std::cout << "#TF Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  ptf->timeFrameId
			<< " Nsource: " << std::setw(8) << std::setfill('0') <<  ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;
		#if 0
		for (unsigned int i = 0 ; i < ptf->length ; i++) {
			if ((i % 16) == 0) {
				std::cout << std::endl
					<< "#" << std::setw(8) << std::setfill('0')
					<< i << " : ";
			}
			std::cout << " "
				<< std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<unsigned int>(pdata[i]);
		}
		std::cout << std::endl;
		#endif
		
	} else if (msg_magic == SubTimeFrame::Magic) {
		SubTimeFrame::Header *pstf
				= reinterpret_cast<SubTimeFrame::Header *>(pdata);
		std::cout << "#STF Header "
			<< std::hex << std::setw(8) << std::setfill('0') <<  pstf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  pstf->timeFrameId
			<< " Type: " << std::setw(8) << std::setfill('0') <<  pstf->Type
			<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->FEMId
			<< std::endl << "# "
			<< " len: " << std::dec <<  pstf->length
			<< " nMsg: " << std::dec <<  pstf->numMessages
			<< std::endl << "# "
			<< " Ts: " << std::dec << pstf->time_sec
			<< " Tus: " << std::dec << pstf->time_usec
			<< std::endl;

		fe_type = pstf->Type;

		//// toriaezu debug no tameni ireru. atodekesukoto
		//fe_type = 1;
		//pstf->Type = 1;
		//pstf->FEMId = 1234;
		////

	} else {
	       #if 1
		//for (unsigned int j = 0 ; j < msize ; j += 8) {
		for (unsigned int j = 0 ; j < 8 ; j += 8) {
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

			if ((pdata[j + 7] & 0xfc) == (TDC64H::T_TDC << 2)) {
				std::cout << "TDC ";
				uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[j]));
				if (fe_type == 0) {
					struct TDC64H::tdc64 tdc;
					TDC64H::Unpack(*dword, &tdc);
					std::cout << "H :" 
						<< " CH: " << std::dec << std::setw(3) << tdc.ch
						<< " TDC: " << std::setw(7) << tdc.tdc << std::endl;
				} else
				if (fe_type == 1) {
					struct TDC64L::tdc64 tdc;
					TDC64L::Unpack(*dword, &tdc);
					std::cout << "L :" 
						<< " CH: " << std::dec << std::setw(3) << tdc.ch
						<< " TDC: " << std::setw(7) << tdc.tdc << std::endl;
				} else {
					std::cout << "UNKNOWN Type :0x" << std::hex << fe_type << std::endl;
				}

			} else if ((pdata[j + 7] & 0xfc) == (TDC64H::T_HB << 2)) {
				std::cout << "Hart beat" << std::endl;
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


	return true;
}

//int FltCoin::Trigger(FairMQParts &inParts, FairMQPatts &outParts)
//{
//	return 0;
//}

int FltCoin::IsHartBeat(uint64_t val, uint32_t type)
{
	int hbframe = -1;
	if (type == SubTimeFrame::TDC64H) {
		struct TDC64H::tdc64 tdc;
		if (TDC64H::Unpack(val, &tdc) == TDC64H::T_HB) {
			hbframe = tdc.hartbeat;
		}
	} else
	if (type == SubTimeFrame::TDC64L) {
		struct TDC64L::tdc64 tdc;
		if (TDC64L::Unpack(val, &tdc) == TDC64L::T_HB) {
			hbframe = tdc.hartbeat;
		}
	} else {
		std::cout << "Unknown device : " << std::hex << type << std::endl;
	}

	return hbframe;
}

bool FltCoin::ConditionalRun()
{
	//Receive
	FairMQParts inParts;

	FairMQMessagePtr msg_header(fTransportFactory->CreateMessage());
	struct Filter::Header fltheader;
	struct TimeFrame::Header tfheader;

	std::chrono::system_clock::time_point sw_start, sw_end;


	std::cout << "#DDDDD Trigger region size: " << std:: dec << fTrig->GetTimeRegionSize() << std::endl;;

	if (Receive(inParts, fInputChannelName, 0, 1000) > 0) {
		assert(inParts.Size() >= 2);
		std::cout << "# Nmsg: " << inParts.Size() << std::endl;

		sw_start = std::chrono::system_clock::now();

		struct DataBlock {
			uint32_t FEMId;
			uint32_t Type;
			int HBFrame;
			int index;
			int msg_index;
			int nTrig;
		};

		std::vector<struct DataBlock> blocks;
		std::vector< std::vector<struct DataBlock> > block_map;

		std::vector<int> hbblocks;
		std::vector< std::vector<int> > hbblock_map;
		std::vector<struct SubTimeFrame::Header> stf;

		uint64_t femid = 0;
		uint64_t devtype = 0;
		int hbframe = 0;
		int iblock = 0;
		int ifem = 0;

		//struct TimeFrame::Header tfHeader_keep;

		std::vector<bool> flag_sending;

		//for(auto& vmsg : inParts) {
		for(int i = 0 ; i < inParts.Size() ; i++) {
			flag_sending.push_back(true);
			CheckData(inParts.At(i));

			auto tfHeader = reinterpret_cast<struct TimeFrame::Header *>(inParts[i].GetData());
			auto stfHeader = reinterpret_cast<struct SubTimeFrame::Header *>(inParts[i].GetData());
			struct DataBlock dblock;

			if (tfHeader->magic == TimeFrame::Magic) {
				iblock = 0;
				ifem = -1;
				stf.clear();
				stf.resize(0);
			} else
			if (stfHeader->magic == SubTimeFrame::Magic) {
				femid = stfHeader->FEMId;
				devtype = stfHeader->Type;
				stf.push_back(*stfHeader);
				if (blocks.size() > 0) block_map.push_back(blocks);
				if (hbblocks.size() > 0) hbblock_map.push_back(hbblocks);
				iblock = 0;
				dblock.index = -1;
				dblock.msg_index = -1;
				blocks.clear();
				blocks.resize(0);
				ifem++;
			} else {
				// make block map;

				uint64_t *data = reinterpret_cast<uint64_t *>(inParts[i].GetData());

				hbframe = IsHartBeat(data[0], devtype);
				if (hbframe < 0) {
					dblock.FEMId = femid;
					dblock.Type = devtype;
					dblock.index = iblock;
					dblock.msg_index = i;
					dblock.nTrig = 0;
				} else {
					//data ga nakattatokimo push_back

					dblock.HBFrame = hbframe;
					blocks.push_back(dblock);
					hbblocks.push_back(iblock);

					dblock.FEMId = 0;
					dblock.Type = 0;
					dblock.index = -1;
					dblock.msg_index = -1;
					dblock.nTrig = 0;
				}
				iblock++;
			}
		} /// end of the for loop
		block_map.push_back(blocks);
		hbblock_map.push_back(hbblocks);

		std::cout << "blocks: " << blocks.size() << std::endl;
		int totalhits = 0;
		for (size_t i = 0 ; i < blocks.size() ; i++) {

			fTrig->CleanUpTimeRegion();

			/// mark Hits
			for (size_t iifem = 0 ; iifem < block_map.size() ; iifem++) {
				struct DataBlock *dbl = &block_map[iifem][i];
				uint64_t vfemid = dbl->FEMId;
				int mindex = dbl->msg_index;

				std::cout << "#D Mark FEM ID: "
					<< std::dec << vfemid
					<< " Type: " << dbl->Type
					<< " HBFrame: " << dbl->HBFrame << std::endl;

				if (mindex > 0) {
					fTrig->Mark(
						reinterpret_cast<unsigned char *>(inParts[mindex].GetData()),
						inParts[mindex].GetSize(),
						vfemid, dbl->Type);
				}
			}

			uint32_t *tr = fTrig->GetTimeRegion();
			std::cout << "####DDDD Hit TimeRegion: ";
			for (uint32_t ii = 0 ; ii < fTrig->GetTimeRegionSize() ; ii++) {
				if (tr[ii] != 0) {
				std::cout << " " << std::dec << i << ":"
					<< std::hex << std::setw(4) << std::setfill('0')
					<< tr[ii];
				}
			}
			std::cout << std::endl;

			/// check coincidence
			int nhits = fTrig->Scan()->size();


			std::vector<uint32_t> *hits = fTrig->Scan();
			std::cout << "#DD Hits : ";
			for (unsigned int ii = 0 ; ii < hits->size() ; ii++) {
				std::cout << " " << std::dec << (*hits)[ii];
				if (ii > 10) {
					std::cout << "...";
					break;
				}
			}
			std::cout << std::endl;


			for (size_t iifem = 0 ; iifem < block_map.size() ; iifem++) {
				block_map[iifem][i].nTrig = nhits;
				if (nhits == 0) {
					int mindex = block_map[iifem][i].msg_index;
					flag_sending[mindex] = false;
					flag_sending[mindex + 1] = false;
				}
			}

			totalhits += nhits;
			std::cout << "# " << i << " nhits: " << nhits << std::endl;

		}

		std::cout << "#block_map size: " << block_map.size() << std::endl;
		std::cout << "#blocks size: ";
		for (unsigned int i = 0 ; i < block_map.size() ; i++) {
			std::cout << " " << i << " : " << blocks.size();
		}
		std::cout << std::endl;


		std::cout << "#Flag: ";
		for (const auto& v : flag_sending) std::cout << " " << v; 
		std::cout << std::endl;


		#if 0
		gQdepth++;
		FairMQMessagePtr fltheadermsg(
			NewMessage(
			reinterpret_cast<char *>(&fltHeader),
			sizeof(fltHeader),
			[](void* pdata, void*) {
				char *p = reinterpret_cast<char *>(pdata);
				delete p;
				//gQdepth--;
			},
			nullptr));
		outParts.AddPart(std::move(fltheadermsg));
		#endif


		#if 0
		auto tfHeader = reinterpret_cast<TimeFrame::Header*>(inParts.At(0)->GetData());
		auto stfHeader = reinterpret_cast<SubTimeFrame::Header*>(inParts.At(0)->GetData());
		auto stfId     = stfHeader->timeFrameId;

		if (fDiscarded.find(stfId) == fDiscarded.end()) {
			// accumulate sub time frame with same STF ID
			if (fTFBuffer.find(stfId) == fTFBuffer.end()) {
				fTFBuffer[stfId].reserve(fNumSource);
			}
			fTFBuffer[stfId].emplace_back(
				STFBuffer {std::move(inParts),
				std::chrono::steady_clock::now()});
		} else {
			// if received ID has been previously discarded.
			LOG(warn) << "Received part from an already discarded timeframe with id " << stfId;
		}
		#endif

		FairMQParts outParts;

		sw_end = std::chrono::system_clock::now();
		//double elapse = std::chrono::duration_cast<std::chrono::microseconds>(
		uint32_t elapse = std::chrono::duration_cast<std::chrono::microseconds>(
			sw_end - sw_start).count();
		std::cout << "#elapse: " << elapse << std::endl;

		//make header message
		auto fltHeader = std::make_unique<struct Filter::Header>();
		fltHeader->magic = Filter::Magic;
		fltHeader->length = sizeof(struct Filter::Header);
		fltHeader->numTrigs = totalhits;
		fltHeader->workerId = fId;
		fltHeader->elapseTime = elapse;
		fltHeader->processTime.tv_sec = 0;
		fltHeader->processTime.tv_usec = 0;
		outParts.AddPart(MessageUtil::NewMessage(*this, std::move(fltHeader)));

		//Copy
		unsigned int msg_size = inParts.Size();
		for (unsigned int ii = 0 ; ii < msg_size ; ii++) {
			if (flag_sending[ii] || (! fIsDataSupress)) {
				FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
				msgCopy->Copy(inParts.AtRef(ii));
				outParts.AddPart(std::move(msgCopy));
			}
		}
	
		//Send
		while (Send(outParts, fOutputChannelName) < 0) {
			// timeout
			if (GetCurrentState() != fair::mq::State::Running) {
				LOG(info) << "Device is not RUNNING";
				break;
			}
			//LOG(error) << "Failed to queue time frame : TF = " << h->timeFrameId;
			LOG(error) << "Failed to queue time frame";
		}

	}

	return true;
}

void FltCoin::PostRun()
{
	LOG(info) << "Post Run";
	return;
}


#if 0
bool FltCoin::HandleData(fair::mq::MessagePtr& msg, int val)
{
	(void)val;
	#if 0
	LOG(info) << "Received: \""
		<< std::string(static_cast<char*>(msg->GetData()), msg->GetSize())
		<< "\"";
	#endif

	#if 0
	LOG(info) << "Received: " << msg->GetSize() << " : " << val;
	#endif

	if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
		LOG(info) << "Configured maximum number of iterations reached."
			<< " Leaving RUNNING state.";
		return false;
	}

	CheckData(msg);
	

	// return true if you want the handler to be called again
	// (otherwise return false go to the Ready state)
	return true;
}
#endif


void addCustomOptions(bpo::options_description& options)
{
	using opt = FltCoin::OptionKey;

	options.add_options()
		("max-iterations", bpo::value<uint64_t>()->default_value(0),
		"Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)")
		(opt::InputChannelName.data(),
			bpo::value<std::string>()->default_value("in"),
			"Name of the input channel")
		(opt::OutputChannelName.data(),
			bpo::value<std::string>()->default_value("out"),
			"Name of the output channel")
		(opt::DataSupress.data(),
			bpo::value<std::string>()->default_value("true"),
			"Data supression enable")
    		;

}


std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
	return std::make_unique<FltCoin>();
}
