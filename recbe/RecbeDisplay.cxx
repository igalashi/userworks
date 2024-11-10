/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <arpa/inet.h>

//#include "TSystem.h"
//#include "TApplication.h"

#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"
#include "UnpackTdc.h"
#include "KTimer.cxx"
#include "uhbook.cxx"
#include "recbe.h"
#include "ghistogram.cxx"

#define USE_THREAD



namespace bpo = boost::program_options;

struct RecbeDisplay : fair::mq::Device
{
	struct OptionKey {
		static constexpr std::string_view InputChannelName  {"in-chan-name"};
		static constexpr std::string_view RedisUrl  {"redis-uri"};
	};

	RecbeDisplay()
	{
		// register a handler for data arriving on "data" channel
		// OnData("in", &RecbeDisplay::HandleData);
		// LOG(info) << "Constructer Input Channel : " << fInputChannelName;
	}

	void InitTask() override
	{
		using opt = OptionKey;

		// Get the fMaxIterations value from the command line options (via fConfig)
		fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
		fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
		LOG(info) << "InitTask Input Channel : " << fInputChannelName;
		fRunNumber   = fConfig->GetProperty<std::string>("run_number");

		static bool atFirst = true;
		if (atFirst) {
			atFirst = false;

			#if 0
			fId          = fConfig->GetProperty<std::string>("id");
			#endif
			fServiceName = fConfig->GetProperty<std::string>("service-name");
			fTopPrefix   = fConfig->GetProperty<std::string>("top-prefix");
			fSeparator   = fConfig->GetProperty<std::string>("separator");

			fRedisUrl    = fConfig->GetProperty<std::string>(opt::RedisUrl.data());
			fDb = std::make_unique<RedisDataStore>(fRedisUrl);

			std::cout << "#D ServiceName: " << fServiceName << std::endl;
			std::cout << "#D Id: " << fId << std::endl;
			std::cout << "#D Separator: " << fSeparator << std::endl;
			std::cout << "#D TopPrefix: " << fTopPrefix << std::endl;
			std::cout << "#D nunNumber: " << fRunNumber << std::endl;
			std::cout << "#D RedisUrl: " << fRedisUrl << std::endl;


			//LOG(debug) << "serverUri: " << serverUri;
			//data_store = new RedisDataStore(serverUri);
			//fdevId = fConfig->GetProperty<std::string>("id");
			//LOG(debug) << "fdevId: " << fdevId;
			//fTopPrefix   = "scaler";
			//fSeparator   = fConfig->GetProperty<std::string>("separator");

			gHistInit(fRedisUrl, fId);
			fKeyPrefix = "dqm" + fSeparator
				+ fServiceName + fSeparator
				+ fId + fSeparator;
		}

		fKt1.SetDuration(100);
		fKt2.SetDuration(5000);
		fKt3.SetDuration(3000);

		gHistReset();

		//OnData(fInputChannelName, &RecbeDisplay::HandleData);
	}


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
	
private:
	bool CheckData(fair::mq::MessagePtr& msg);
	void BookData(fair::mq::MessagePtr& msg);

	uint64_t fMaxIterations = 0;
	uint64_t fNumIterations = 0;

	std::string fInputChannelName;

	std::string fServiceName;
	//std::string fId;
	std::string fSeparator;
	std::string fTopPrefix;
	std::string fRunNumber;
	std::string fKeyPrefix;

	std::string fRedisUrl;
	std::unique_ptr<RedisDataStore> fDb;

	struct STFBuffer {
		FairMQParts parts;
		std::chrono::steady_clock::time_point start;
	};

	std::unordered_map<uint32_t, std::vector<STFBuffer>> fTFBuffer;
	std::unordered_set<uint64_t> fDiscarded;
	int fNumSource = 0;
	int fFeType = 0;
	uint32_t fFEMId = 0;
	int fPrescale = 10;

	KTimer fKt1;
	KTimer fKt2;
	KTimer fKt3;
};

bool RecbeDisplay::CheckData(fair::mq::MessagePtr& msg)
{
	unsigned int msize = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());
	uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(pdata));

	#if 0
	std::cout << "#Msg TopData(8B): " << std::hex << msg_magic
		<< " Size: " << std::dec << msize << std::endl;
	#endif

	//if (msg_magic == Filter::Magic) {
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

	//} else if (msg_magic == TimeFrame::Magic) {
	} else if (msg_magic == TimeFrame::MAGIC) {
		TimeFrame::Header *ptf
			= reinterpret_cast<TimeFrame::Header *>(pdata);
		std::cout << "#TF Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  ptf->timeFrameId
			<< " Nsource: " << std::setw(8) << std::setfill('0') <<  ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;

		fFeType = 0;
		fFEMId  = 0;

	//} else if (msg_magic == SubTimeFrame::Magic) {
	} else if (msg_magic == SubTimeFrame::MAGIC) {
		SubTimeFrame::Header *pstf
			= reinterpret_cast<SubTimeFrame::Header *>(pdata);
		std::cout << "#STF Header "
			<< std::hex << std::setw(8) << std::setfill('0') <<  pstf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  pstf->timeFrameId
			//<< " res: " << std::setw(8) << std::setfill('0') <<  pstf->reserved
			//<< " Type: " << std::setw(8) << std::setfill('0') <<  pstf->FEMType
			//<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->FEMId
			<< " Type: " << std::setw(8) << std::setfill('0') <<  pstf->femType
			<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->femId
			//<< std::endl << "# "
			<< " len: " << std::dec <<  pstf->length
			<< " nMsg: " << std::dec <<  pstf->numMessages
			//<< std::endl << "# "
			//<< " Ts: " << std::dec << pstf->time_sec
			//<< " Tus: " << std::dec << pstf->time_usec
			<< " Ts: " << std::dec << pstf->timeSec
			<< " Tus: " << std::dec << pstf->timeUSec
			<< std::endl;

		//fFeType = pstf->FEMType;
		//fFEMId  = pstf->FEMId;
		fFeType = pstf->femType;
		fFEMId  = pstf->femId;

	} else if ((msg_magic & 0x0000'0000'0000'00ff) == 0x0000'0000'0000'0022) {
		struct Recbe::Header *recbe;
		recbe = reinterpret_cast<Recbe::Header *>(pdata);
		int sent_num = ntohs(recbe->sent_num);
		int ttime = ntohs(recbe->time);
		int len = ntohs(recbe->len);
		int trig_count = ntohl(recbe->trig_count);
		std::cout << "#Recbe Header " << std::hex
			<< std::setw(2) << std::setfill('0') << static_cast<unsigned int>(recbe->type)
			<< " id: "
			<< std::setw(2) << std::setfill('0') << static_cast<unsigned int>(recbe->id)
			<< std::dec
			<< " Sent: " << sent_num
			<< " Time: " << ttime
			<< " Len: " << len
			<< " Trig: " << trig_count
			<< std::endl;

		fFeType = static_cast<unsigned int>(recbe->type);
		fFEMId  = static_cast<unsigned int>(recbe->id);

	} else {
		std::cout << "#Unknown Header " << std::hex << msg_magic << std::endl;
		std::cout << "#FE " << fFeType << " id: " << fFEMId << std::endl;

		#if 1
		//for (unsigned int j = 0 ; j < msize ; j++) {
		unsigned int ndump = (msize > 128) ? 128 : msize;
		for (unsigned int j = 0 ; j < ndump ; j++) {
			std::cout << std::hex << std::setfill('0');
			if ((j % 16) == 0) {
				if (j != 0)  std::cout << std::endl;
				std::cout << "# " << std::setw(8) << j << " :";
			}
			std::cout << " " << std::setw(2) << static_cast<unsigned int>(pdata[j]);
		}
		std::cout << std::endl;
		#endif

		#if 0
		unsigned int i = 0;
		int tic = 0;
		while (i < msize) {
			std::cout << "ADC[" << tic << "]";
			for (int j = 0 ; j < 48 ; j++) {
				int val = ntohs(
					*(reinterpret_cast<uint16_t *>(
						(pdata + ((tic * 48 * 2) + j) * sizeof(uint16_t))
					)));
				std::cout << " " << val;
				i += sizeof(uint16_t);
			}
			std::cout << std::endl;
			std::cout << "TDC[" << tic << "]";
			for (int j = 0 ; j < 48 ; j++) {
				int val = ntohs(
					*(reinterpret_cast<uint16_t *>(
						(pdata + ((tic * 48 * 2 + 48) + j) * sizeof(uint16_t))
					)));
				std::cout << " " << val;
				i += sizeof(uint16_t);
			}
			std::cout << std::endl;
			tic++;
		}
		#endif

		#if 0
		for (unsigned int i = 0 ; i < msize ; i += 48 * 2 * sizeof(uint16_t)) {
			std::cout << "ADC[" << i << "] ";
			for (unsigned int j = 0 ; j < 48 ; j++) {
				int val = ntohs(
					*(reinterpret_cast<unsigned short int *>(
						(pdata + (j * 2))
					)));
				std::cout << " " << val;
			}
			std::cout << std::endl;
			std::cout << "TDC[" << i << "] ";
			for (unsigned int j = 0 ; j < 48 ; j++) {
				int val = ntohs(
					*(reinterpret_cast<unsigned short int *>(
						(pdata + (j * 2) + (48 * sizeof(uint16_t)))
					)));
				std::cout << " " << val;
			}
			std::cout << std::endl;
		}
		#endif

	}

	#if 0
	for (unsigned int i = 0 ; i < msize; i++) {
		if ((i % 16) == 0) {
			if (i != 0) std::cout << std::endl;
			std::cout << "#" << std::setw(8) << std::setfill('0')
				<< i << " : ";
		}
		std::cout << " "
			<< std::hex << std::setw(2) << std::setfill('0')
			<< static_cast<unsigned int>(pdata[i]);
	}
	std::cout << std::endl;
	#endif

	return true;
}

void RecbeDisplay::BookData(fair::mq::MessagePtr& msg)
{
	unsigned int msize = msg->GetSize();

	if (msize <= 0) {
		std::cout << "#E Zero-size message" << std::endl;
		return;
	}

	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());
	uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(pdata));

	#if 0
	std::cout << "#D Msize: " << msize << " : " << std::endl;
	if (msg_magic == TimeFrame::Magic) {
		std::cout << "TF  :";
	} else if (msg_magic == SubTimeFrame::Magic) {
		std::cout << "STF :";
	} else {
		std::cout << "UNK :";
	}
	std::cout << std::hex;

	int nval = msize;
	if (msize > 32) nval = 32;
	for (int i = 0 ; i < nval ; i++) {
		std::cout << " " << std::setw(2) << (static_cast<unsigned int>(pdata[i]) & 0xff);
	}
	std::cout << std::dec << std::endl;
	#endif

	//if (msg_magic == Filter::Magic) {
	if (msg_magic == Filter::MAGIC) {
		#if 0
		Filter::Header *pflt
			= reinterpret_cast<Filter::Header *>(pdata);
		std::cout << "#FLT Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  pflt->magic
			<< " len: " << std::dec << std::setw(8) <<  pflt->length
			<< " N trigs: " << std::setw(8) <<  pflt->numTrigs
			<< " Id: " << std::setw(8) << pflt->workerId
			<< " elapse: " << std::dec <<  pflt->elapseTime
			<< std::endl;
		#endif

	//} else if (msg_magic == TimeFrame::Magic) {
	} else if (msg_magic == TimeFrame::MAGIC) {
		#if 0
		TimeFrame::Header *ptf
			= reinterpret_cast<TimeFrame::Header *>(pdata);
		std::cout << "#TF Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  ptf->timeFrameId
			<< " Nsource: " << std::setw(8) << std::setfill('0') <<  ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;
		#endif


		static int trig_prev = 0;
		static int trig_diff = 0;
		TimeFrame::Header *ptf
			= reinterpret_cast<TimeFrame::Header *>(pdata);
		int trig_now = ptf->timeFrameId;
		#if 0
		if (trig_diff != (trig_now - trig_prev)) {
			std::cout << "#W Strange Trigger Number : " << trig_now
				<< " / " << trig_prev << " : " << trig_diff << std::endl;
			trig_diff = trig_now - trig_prev;
		}
		trig_prev = trig_now;
		#else
		// std::cout << "#D Trigger Number : " << trig_now << std::endl;
		#endif
		

		fFEMId  = 0;
		fFeType = 0;

	//} else if (msg_magic == SubTimeFrame::Magic) {
	} else if (msg_magic == SubTimeFrame::MAGIC) {
		SubTimeFrame::Header *pstf
			= reinterpret_cast<SubTimeFrame::Header *>(pdata);
		#if 0
		std::cout << "#STF Header "
			<< std::hex << std::setw(8) << std::setfill('0') <<  pstf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  pstf->timeFrameId
			//<< " res: " << std::setw(8) << std::setfill('0') <<  pstf->reserved
			<< " Type: " << std::setw(8) << std::setfill('0') <<  pstf->FEMType
			<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->FEMId
			<< std::endl << "# "
			<< " len: " << std::dec <<  pstf->length
			<< " nMsg: " << std::dec <<  pstf->numMessages
			<< std::endl << "# "
			<< " Ts: " << std::dec << pstf->time_sec
			<< " Tus: " << std::dec << pstf->time_usec
			<< std::endl;
		#endif

		//fFEMId  = pstf->FEMId;
		//fFeType = pstf->FEMType;
		fFEMId  = pstf->femId;
		fFeType = pstf->femType;

	} else if ((msg_magic & 0x0000'0000'0000'00ff) == 0x0000'0000'0000'0022) {
		struct Recbe::Header *recbe;
		recbe = reinterpret_cast<Recbe::Header *>(pdata);

		#if 0
		int sent_num = ntohs(recbe->sent_num);
		int ttime = ntohs(recbe->time);
		int len = ntohs(recbe->len);
		int trig_count = ntohl(recbe->trig_count);

		std::cout << "#Recbe Header " << std::hex
			<< std::setw(2) << std::setfill('0') << static_cast<unsigned int>(recbe->type)
			<< " id: "
			<< std::setw(2) << std::setfill('0') << static_cast<unsigned int>(recbe->id)
			<< std::dec
			<< " Sent: " << sent_num
			<< " Time: " << ttime
			<< " Len: " << len
			<< " Trig: " << trig_count
			<< std::endl;
		#endif

		fFeType = static_cast<unsigned int>(recbe->type);
		fFEMId  = static_cast<unsigned int>(recbe->id);

		if (fFeType == 0x22) {
			gHistBook(msg, fFEMId, fFeType);
		} else {
			std::cout << "#E " << "unknown FE type : " << fFeType << std::endl;
		}

	} else {
		#if 0
		if (fFeType == 0x22) {
			gHistBook(msg, fFEMId, fFeType);
		} else {
			std::cout << "#E " << "unknown FE type : " << fFeType << std::endl;
		}
		#endif

		std::cout << "#Unknown Header " << std::hex << msg_magic << std::endl;
		std::cout << "#FE " << fFeType << " id: " << fFEMId << std::endl;
		std::cout << "#size : " << msize << std::endl;

		//for (unsigned int j = 0 ; j < msize ; j++) {
		unsigned int ndump = (msize > 128) ? 128 : msize;
		for (unsigned int j = 0 ; j < ndump ; j++) {
			std::cout << std::hex << std::setfill('0');
			if ((j % 16) == 0) {
				if (j != 0)  std::cout << std::endl;
				std::cout << "# " << std::setw(8) << j << " :";
			}
			std::cout << " " << std::setw(2) << static_cast<unsigned int>(pdata[j]);
		}
		std::cout << std::endl;
	}

	return;
}

bool RecbeDisplay::ConditionalRun()
{

	//Receive
	FairMQParts inParts;
	if (Receive(inParts, fInputChannelName, 0, 1000) > 0) {
		//assert(inParts.Size() >= 2);

		static std::chrono::system_clock::time_point start
			= std::chrono::system_clock::now();
		static uint64_t counts = 0;
		static double freq = 0;

		const double kDURA = 10;
		auto now = std::chrono::system_clock::now();
		auto elapse = std::chrono::duration_cast<std::chrono::milliseconds>
			(now - start).count();
		if (elapse > (1000 * kDURA)) {
			freq = static_cast<double>(counts)
				/ static_cast<double>(elapse) * 1000;
			counts = 0;
			start = std::chrono::system_clock::now();
		}

		if (fKt3.Check()) {
			std::cout << "Nmsg: " << std::dec << inParts.Size();
			std::cout << "  Freq: " << freq << " el " << elapse
				<< " c " << counts  << std::endl;
		}

		#if 0
		static std::chrono::system_clock::time_point last;
		std::cout << " Elapsed time:"
			<< std::chrono::duration_cast<std::chrono::microseconds>
			(now - last).count();
		last = now;
		std::cout << std::endl;
		#endif

		if (inParts.Size() > 2) {
			#if 0
			for(auto& vmsg : inParts) CheckData(vmsg);
			#else
			for(auto& vmsg : inParts) BookData(vmsg);
			#endif
		}

		#if 1
		//std::cout << "#Nmsg: " << std::dec << inParts.Size() << std::endl;
		#else
		if ((counts % 100) == 0) std::cout << "." << std::flush;
		if ((counts % fPrescale) == 0) for(auto& vmsg : inParts) BookData(vmsg);
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

		counts++;
	}

	if (fKt2.Check()) gHistDraw();

	return true;
}

void RecbeDisplay::PostRun()
{
	LOG(info) << "Post Run";
	return;
}


#if 0
bool RecbeDisplay::HandleData(fair::mq::MessagePtr& msg, int val)
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
	using opt = RecbeDisplay::OptionKey;

	options.add_options()
		("max-iterations", bpo::value<uint64_t>()->default_value(0),
		"Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)")
		(opt::InputChannelName.data(),
			bpo::value<std::string>()->default_value("in"),
			"Name of the input channel")
		(opt::RedisUrl.data(),
			bpo::value<std::string>()->default_value("tcp://127.0.0.1:6379/3"),
			"URL of redis server and db number")
		;
}


std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
	//gHistInit();
	return std::make_unique<RecbeDisplay>();
}
