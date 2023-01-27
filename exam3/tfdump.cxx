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

#include "SubTimeFrameHeader.h"
#include "HulStrTdcData.h"

namespace bpo = boost::program_options;

struct TFdump : fair::mq::Device
{
	struct OptionKey {
		static constexpr std::string_view InputChannelName  {"in-chan-name"};
	};

	TFdump()
	{
		// register a handler for data arriving on "data" channel
		//OnData("in", &TFdump::HandleData);

		//LOG(info) << "Constructer Input Channel : " << fInputChannelName;
	}

	void InitTask() override
	{
		using opt = OptionKey;

		// Get the fMaxIterations value from the command line options (via fConfig)
		fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
		fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
		LOG(info) << "InitTask Input Channel : " << fInputChannelName;
		OnData(fInputChannelName, &TFdump::HandleData);

	}


	bool HandleData(fair::mq::MessagePtr& msg, int);
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

private:
	uint64_t fMaxIterations = 0;
	uint64_t fNumIterations = 0;
	std::string fInputChannelName;
};


bool TFdump::CheckData(fair::mq::MessagePtr& msg)
{
	unsigned int size = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());

	for (unsigned int i = 0 ; i < size ; i += 5) {
		#if 0
		std::cout << std::hex << std::setw(8) << std::setfill('0')
			<< i << " : "
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 0]) << " "
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 1]) << " "
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 2]) << " "
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 3]) << " "
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 4]) 
			<< std::dec << std::endl;
		#endif

		if ((pdata[i + 4] & 0xf0) != 0xd0) {
			std::cout << "# " << std::setw(8) << i << " : "
			<< std::hex << std::setw(2) << std::setfill('0')
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 4]) << " "
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 3]) << " "
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 2]) << " "
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 1]) << " "
			<< std::setw(2) << static_cast<unsigned int>(pdata[i + 0]) << " : ";

			if        ((pdata[i + 4] & 0xf0) == 0x10) {
				std::cout << "SPILL Start" << std::endl;
			} else if ((pdata[i + 4] & 0xf0) == 0xf0) {
				std::cout << "Hart beat" << std::endl;
			} else if ((pdata[i + 4] & 0xf0) == 0x40) {
				std::cout << "SPILL End" << std::endl;
			} else {
				std::cout << std::endl;
			}
		}
	}


	return true;
}


bool TFdump::HandleData(fair::mq::MessagePtr& msg, int val)
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


void addCustomOptions(bpo::options_description& options)
{
	using opt = TFdump::OptionKey;

	options.add_options()
		("max-iterations", bpo::value<uint64_t>()->default_value(0),
		"Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)")
		(opt::InputChannelName.data(),
			bpo::value<std::string>()->default_value("in"),
			"Name of the input channel");

}


std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
	return std::make_unique<TFdump>();
}
