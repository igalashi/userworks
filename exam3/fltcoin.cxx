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
#include <unordered_map>
#include <unordered_set>

#include "MessageUtil.h"

#include "HulStrTdcData.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"

#include "trigger.cxx"


std::atomic<int> gQdepth = 0;

namespace bpo = boost::program_options;

struct FltCoin : fair::mq::Device
{
	struct OptionKey {
		static constexpr std::string_view InputChannelName  {"in-chan-name"};
		static constexpr std::string_view OutputChannelName  {"out-chan-name"};
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


	uint64_t fMaxIterations = 0;
	uint64_t fNumIterations = 0;

	std::string fInputChannelName;
	std::string fOutputChannelName;

	struct STFBuffer {
		FairMQParts parts;
		std::chrono::steady_clock::time_point start;
	};

	std::unordered_map<uint32_t, std::vector<STFBuffer>> fTFBuffer;
	std::unordered_set<uint64_t> fDiscarded;
	int fNumSource = 0;

	Trigger *fTrig;

};


void FltCoin::InitTask()
{
	using opt = OptionKey;

	// Get the fMaxIterations value from the command line options (via fConfig)
	fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
	fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
	fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());
	LOG(info) << "InitTask: Input Channel : " << fInputChannelName
		<< " Output Channel : " << fOutputChannelName;


	fTrig->Entry(1, 26, 0);
	fTrig->Entry(3, 32, 0);

}

bool FltCoin::CheckData(fair::mq::MessagePtr &msg)
{
	unsigned int msize = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());
	uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(pdata));

	std::cout << "#Msg MAGIC: " << std::hex << msg_magic
		<< " Size: " << std::dec << msize << std::endl;

	if (msg_magic == TimeFrame::Magic) {
		TimeFrame::Header *ptf = reinterpret_cast<TimeFrame::Header *>(pdata);

		std::cout << "#TF Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  ptf->timeFrameId
			<< " Nsource: " << std::setw(8) << std::setfill('0') <<  ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;

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
		
	} else if (msg_magic == SubTimeFrame::Magic) {
		SubTimeFrame::Header *pstf
				= reinterpret_cast<SubTimeFrame::Header *>(pdata);
		std::cout << "#STF Header "
			<< std::hex << std::setw(8) << std::setfill('0') <<  pstf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  pstf->timeFrameId
			<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->FEMId
			<< " len: " << std::dec <<  pstf->length
			<< " nMsg: " << std::dec <<  pstf->numMessages
			<< std::endl;

			#if 0
			for (unsigned int j = 0 ; j < pstf->length ; j += 5) {
				if ((pstfdata[j + 4] & 0xf0) != 0xd0) {
				std::cout << "# " << std::setw(8) << i << " : "
					<< std::hex << std::setw(2) << std::setfill('0')
					<< std::setw(2) << static_cast<unsigned int>(pdata[j + 4]) << " "
					<< std::setw(2) << static_cast<unsigned int>(pdata[j + 3]) << " "
					<< std::setw(2) << static_cast<unsigned int>(pdata[j + 2]) << " "
					<< std::setw(2) << static_cast<unsigned int>(pdata[j + 1]) << " "
					<< std::setw(2) << static_cast<unsigned int>(pdata[j + 0]) << " : ";

					if	((pdata[j + 4] & 0xf0) == 0x10) {
						std::cout << "SPILL Start" << std::endl;
					} else if ((pdata[j + 4] & 0xf0) == 0xf0) {
						std::cout << "Hart beat" << std::endl;
					} else if ((pdata[j + 4] & 0xf0) == 0x40) {
						std::cout << "SPILL End" << std::endl;
					} else {
						std::cout << std::endl;
					}
				}
			}
			#endif


	} else {
		std::cout << "# Unknown Header" << std::hex << msg_magic << std::endl;
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
		if (TDC64H::Unpack(val, &tdc) == TDC64H::T_TDC) {
			hbframe = tdc.hartbeat;
		}
	} else
	if (type == SubTimeFrame::TDC64L) {
		struct TDC64L::tdc64 tdc;
		if (TDC64L::Unpack(val, &tdc) == TDC64L::T_TDC) {
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
	FairMQParts outParts;

	FairMQMessagePtr msg_header(fTransportFactory->CreateMessage());
	struct Filter::Header fltheader;
	struct TimeFrame::Header tfheader;


	if (Receive(inParts, fInputChannelName, 0, 1000) > 0) {
		assert(inParts.Size() >= 2);
		std::cout << "# Nmsg: " << inParts.Size() << std::endl;

		struct DataBlock {
			uint32_t FEMId;
			uint32_t Type;
			int HBFrame;
			int index;
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

		struct TimeFrame::Header tfHeader_keep;

		//for(auto& vmsg : inParts) {
		for(int i = 0 ; i < inParts.Size() ; i++) {

			CheckData(inParts.At(i));

			auto tfHeader = reinterpret_cast<struct TimeFrame::Header *>(inParts[i].GetData());
			auto stfHeader = reinterpret_cast<struct SubTimeFrame::Header *>(inParts[i].GetData());
			struct DataBlock dblock;

			if (tfHeader->magic == TimeFrame::Magic) {
				//////////
				FairMQMessagePtr msg_tfheader(fTransportFactory->CreateMessage());
				msg_tfheader->Copy(inParts[i]);
				outParts.AddPart(std::move(msg_tfheader));
				//////////
				memcpy(reinterpret_cast<char *>(&tfHeader_keep),
					reinterpret_cast<char *>(tfHeader),
					sizeof(struct TimeFrame::Header));

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
				blocks.clear();
				blocks.resize(0);
				ifem++;
			} else {
				// make block map;

				//int stfframe = 0;
		
				uint64_t *data = reinterpret_cast<uint64_t *>(inParts[i].GetData());
				//int len = inParts[i].GetSize();

				hbframe = IsHartBeat(data[0], devtype);
				if (hbframe < 0) {
					dblock.FEMId = femid;
					dblock.Type = devtype;
					dblock.HBFrame = hbframe;
					dblock.index = iblock;
					dblock.nTrig = 0;
				} else {
					//data ga nakattatokimo push_back

					dblock.HBFrame = hbframe;
					blocks.push_back(dblock);
					hbblocks.push_back(iblock);

					femid = -1;
					devtype = -1;
				}
				iblock++;

			}
		} /// endof for loop


		for (size_t i = 0 ; i < blocks.size() ; i++) {

			/// mark Hits
			for (size_t iifem = 0 ; iifem < block_map.size() ; iifem++) {
				struct DataBlock *dbl = &block_map[iifem][i];
				uint64_t vfemid = dbl->FEMId;
				fTrig->Mark(
					reinterpret_cast<unsigned char *>(inParts[i].GetData()),
					inParts[i].GetSize(),
					vfemid);
			}

			/// check coincidence
			int nhit = fTrig->Scan()->size();
			for (size_t iifem = 0 ; iifem < block_map.size() ; iifem++) {
				block_map[iifem][i].nTrig = nhit;
			}
		}


		//make header message
		auto fltHeader = std::make_unique<struct Filter::Header>();
		//auto fltHeader = new Filter::Header;
		fltHeader->magic = Filter::Magic;
		fltHeader->length = 0;
		fltHeader->numTrigs = 0;
		fltHeader->filterId = 0;
		fltHeader->elapseTime = 0;
		fltHeader->processTime.tv_sec = 0;
		fltHeader->processTime.tv_usec = 0;

		auto u_tfHeader = std::make_unique<struct TimeFrame::Header>(tfHeader_keep);

		outParts.AddPart(MessageUtil::NewMessage(*this, std::move(fltHeader)));
		outParts.AddPart(MessageUtil::NewMessage(*this, std::move(u_tfHeader)));

		//outputmessageparts  wo tukuru



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

		//make payloads message




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
	}

	#if 0
	//Copy
	FairMQParts outParts;
	unsigned int msg_size = inParts.Size();
	for (unsigned int ii = 0 ; ii < msg_size ; ii++) {
		FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
		msgCopy->Copy(outParts.AtRef(ii));
		outParts.AddPart(std::move(msgCopy));
	}
	#endif

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
    		;

}


std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
	return std::make_unique<FltCoin>();
}
