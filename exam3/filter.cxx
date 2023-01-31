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

#include "HulStrTdcData.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"

namespace bpo = boost::program_options;

struct Filter : fair::mq::Device
{
	struct OptionKey {
		static constexpr std::string_view InputChannelName  {"in-chan-name"};
		static constexpr std::string_view OutputChannelName  {"out-chan-name"};
	};

	Filter()
	{
		// register a handler for data arriving on "data" channel
		//OnData("in", &Filter::HandleData);
	}

	void InitTask() override
	{
		using opt = OptionKey;

		// Get the fMaxIterations value from the command line options (via fConfig)
		fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
		fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
		fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());
		LOG(info) << "InitTask: Input Channel : " << fInputChannelName
			<< " Output Channel : " << fOutputChannelName;

		//OnData(fInputChannelName, &Filter::HandleData);
	}


	bool CheckData(fair::mq::MessagePtr& msg);
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

};


bool Filter::CheckData(fair::mq::MessagePtr& msg)
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

					if        ((pdata[j + 4] & 0xf0) == 0x10) {
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

bool Filter::ConditionalRun()
{
	//Receive
	FairMQParts inParts;
	if (Receive(inParts, fInputChannelName, 0, 1000) > 0) {
		assert(inParts.Size() >= 2);

		std::cout << "# Nmsg: " << inParts.Size() << std::endl;
		for(auto& vmsg : inParts) CheckData(vmsg);


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



	//Copy
	FairMQParts outParts;
	unsigned int msg_size = inParts.Size();
	for (unsigned int ii = 0 ; ii < msg_size ; ii++) {
		FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
		msgCopy->Copy(outParts.AtRef(ii));
		outParts.AddPart(std::move(msgCopy));
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

	return true;
}

void Filter::PostRun()
{
	LOG(info) << "Post Run";
	return;
}


#if 0
bool Filter::HandleData(fair::mq::MessagePtr& msg, int val)
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
	using opt = Filter::OptionKey;

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
	return std::make_unique<Filter>();
}
