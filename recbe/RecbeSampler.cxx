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
#include <string>
#include <chrono>
#include <cstdint>

#include <thread> // this_thread::sleep_for
// #include "Header.h"

#include <arpa/inet.h>
#include <sys/time.h>

#include "SubTimeFrameHeader.h"
#include "recbe.h"
#include "CliSock.cxx"
#include "RBCP.cxx"
#include "KTimer.cxx"
#include "hexdump.cxx"


namespace bpo = boost::program_options;

class RecbeSampler : public fair::mq::Device
{
public:
	struct OptionKey {
		static constexpr std::string_view DeviceIp	    {"ip"};
		static constexpr std::string_view DataPort	    {"port"};
		static constexpr std::string_view Timeout_ms        {"timeout"};
		static constexpr std::string_view ControlPort       {"control-port"};
		static constexpr std::string_view OutputChannelName {"out-chan-name"};
		static constexpr std::string_view DQMChannelName    {"dqm-chan-name"};
		static constexpr std::string_view Mode              {"mode"};
		static constexpr std::string_view PollTimeout       {"poll-timeout"};
	};

	RecbeSampler() : fair::mq::Device() {};
	RecbeSampler(const RecbeSampler&)	    = delete;
	RecbeSampler& operator=(const RecbeSampler&) = delete;
	~RecbeSampler() = default;

protected:
	void Init() override;
	void InitTask() override;
	bool ConditionalRun() override;
	void PostRun() override;
	void PreRun() override;
	//bool PreRun() override;
	void Run() override;
	bool CheckRecbeHeader(char*);

private:
	uint64_t fNumIterations = 0;
	unsigned int fNumDestination = 0;
	unsigned int fPollTimeoutMS = 0;
	CliSock fSock;
	std::string fOutputChannelName;
	std::string fDQMChannelName;
	std::string fDeviceIp     {"000.000.000.000"};
	unsigned int fDataPort   = 24;
	unsigned int fControlPort = 4660;
	unsigned int fDeviceType  = 0x100;
	unsigned int fTimeout_ms = 0;
	unsigned int fMode = 0;

	KTimer fKt1;
};


void addCustomOptions(bpo::options_description& options)
{
	using opt = RecbeSampler::OptionKey;
	options.add_options()
		("max-iterations",
			bpo::value<uint64_t>()->default_value(5),
			"Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)")
		(opt::OutputChannelName.data(),
			bpo::value<std::string>()->default_value("out"),
			"Name of the output channel")
		(opt::DQMChannelName.data(),
			bpo::value<std::string>()->default_value("dqm"),
			"Name of the data quality monitoring channel")
		(opt::DeviceIp.data(),
			bpo::value<std::string>()->default_value("192.168.10.16"),
			"IP-address of the front-end device")
		(opt::DataPort.data(),
			bpo::value<std::string>()->default_value("24"),
			"Data port of the front-end device")
		(opt::ControlPort.data(),
			bpo::value<std::string>()->default_value("4660"),
			"Control port of the front-end deive")
		(opt::Timeout_ms.data(),
			bpo::value<std::string>()->default_value("1000"),
			"Timeout of the front-end deive")
		(opt::Mode.data(),
			bpo::value<std::string>()->default_value("1"),
			"Recbe run mode")
		(opt::PollTimeout.data(),
			bpo::value<std::string>()->default_value("1"),
			"Timeout of polling (in msec)")
		;
}


std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
	return std::make_unique<RecbeSampler>();
}


void PrintConfig(const fair::mq::ProgOptions* config, std::string_view name, std::string_view funcname)
{
	auto c = config->GetPropertiesAsStringStartingWith(name.data());
	std::ostringstream ss;
	ss << funcname << "\n\t " << name << "\n";
	for (const auto &[k, v] : c) {
		ss << "\t key = " << k << ", value = " << v << "\n";
	}
	LOG(debug) << ss.str();
}


void RecbeSampler::Init()
{
	LOG(debug) << __FUNCTION__;
}


void RecbeSampler::InitTask()
{
	LOG(debug) << __FUNCTION__;

	PrintConfig(fConfig, "channel-config", __PRETTY_FUNCTION__);
	PrintConfig(fConfig, "chans.", __PRETTY_FUNCTION__);

	using opt = OptionKey;
	fOutputChannelName = fConfig->GetProperty<std::string>(opt::OutputChannelName.data());
	fDQMChannelName    = fConfig->GetValue<std::string>(opt::DQMChannelName.data());

	fDeviceIp          = fConfig->GetProperty<std::string>(opt::DeviceIp.data());
	fDataPort          = std::stoi(fConfig->GetProperty<std::string>(opt::DataPort.data()));
	fControlPort       = std::stoi(fConfig->GetProperty<std::string>(opt::ControlPort.data()));
	fTimeout_ms        = std::stoi(fConfig->GetProperty<std::string>(opt::Timeout_ms.data()));
	fMode              = std::stoi(fConfig->GetProperty<std::string>(opt::Mode.data()));

	fPollTimeoutMS     = std::stoi(fConfig->GetProperty<std::string>(opt::PollTimeout.data()));
	fNumDestination    = GetNumSubChannels(fOutputChannelName);

	LOG(info) << "FEM IP Address: " << fDeviceIp
		<< ", Port: " << fDataPort << ", Control Port: " << fControlPort
		<< "  Number of destinations: " << fNumDestination;

	//fDeviceType	= std::stoi(fConfig->GetProperty<std::string>("DeviceType"));
	//LOG(info) << "Device Type: " << fDeviceType;

	RBCP rbcp;
	rbcp.Open(fDeviceIp.c_str(), fControlPort);
	char val[8]; val[1] = 0x00;
	if (fMode != 0) {
		val[0] = fMode;
		if (rbcp.Write(val, Recbe::R_MODE, 1) > 0) {
			LOG(info) << "Run Mode: " << fMode;
		} else {
			LOG(error) << "RBCP err. IP: " << fDeviceIp << " Port: " << fControlPort;
		}
	}
	rbcp.Close();

	fKt1.SetDuration(5000);
}


bool RecbeSampler::CheckRecbeHeader(char *buf)
{
	int htype[] = {
		Recbe::T_RAW, Recbe::T_SUPPRESS, Recbe::T_BOTH,
		Recbe::T_RAW_OLD, Recbe::T_SUPPRESS_OLD
	};
	
	struct Recbe::Header *h = reinterpret_cast<Recbe::Header *>(buf);
	bool ret = false;
	for (auto i : htype) {
		if (h->type == i) {
			ret = true;
			break;
		}
	}

	return ret;
}


bool RecbeSampler::ConditionalRun()
{
	#if 0
	example_multipart::Header header;
	header.stopFlag = 0;
	#endif

	#ifdef CHECK_FREQ 
	static int nevents = 0;
	static std::chrono::system_clock::time_point t_prev;
	std::chrono::system_clock::time_point t_now
		= std::chrono::system_clock::now();
	auto elapse
		= std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_prev);
	if (elapse.count() >= 5000) {
		double freq = static_cast<double>(nevents) / elapse.count() * 1000.0;
		std::cout << "#D freq.: " << freq << std::endl;
		t_prev = t_now;
		nevents = 0;
	}
	#endif

	fair::mq::Parts outParts;
	outParts.AddPart(NewMessage(sizeof(SubTimeFrame::Header)));
	auto &msgSTFHeader = outParts[0];

	struct Recbe::Header recbe_header;
	int hsize = sizeof(struct Recbe::Header);

	//parts.AddPart(NewMessage(hsize));
	//auto &msgRHeader = parts[1];

	int flag;
	//int nread = fSock.Receive(
	//	reinterpret_cast<char *>(msgRHeader.GetData()), hsize, flag);
	int nread = fSock.Receive(
		reinterpret_cast<char *>(&recbe_header), hsize, flag);
	if (nread < hsize) {
		LOG(warn) << "Irregal data size : " << nread << "/" << hsize;
		if (flag == EAGAIN) {
			 LOG(warn) << "Timeout and Retry";
			int nnread = fSock.Receive(
				reinterpret_cast<char *>(&recbe_header) + nread,
				hsize - nread, flag);
			nread += nnread;
			if (nread < hsize) {
				if (flag == EAGAIN) {
					LOG(warn) << "Timeout anagin...";
				} else {
					LOG(warn) << "Unknown error...";
				}
			}
		} else {
			return true;
		}
	}
	struct Recbe::Header *pheader;
	//pheader = reinterpret_cast<struct Recbe::Header *>(msgRHeader.GetData());
	pheader = &recbe_header;
	int bodysize = static_cast<int>(ntohs(pheader->len));
	int trig = static_cast<int>(ntohl(pheader->trig_count));

	#if 0
	if (fKt1.Check()) {
		std::cout << std::hex
			<< "#D Type: " << (static_cast<int>(pheader->type) & 0xff)
			<< " ID: " << (static_cast<int>(pheader->id) & 0xff)
			<< "  len: " << std::dec << bodysize
			<< " Sent: " << std::dec << ntohs(pheader->sent_num)
			<< " Trig: " << trig << " Time: " << ntohs(pheader->time)
			<< std::endl;;
	}
	#else
	//std::cout << "." << std::flush;
	#endif

	#if 1
	{
		static int l_id = 0;
		static int l_trig = 0;
		bool fdump = false;
		int id_now = static_cast<int>(pheader->id) & 0xff;
		if (l_id != id_now) {
			std::cout << "#W ID: " << std::hex
				<< id_now << " prev: " <<  l_id << std::endl;
			fdump = true;
		}
		if ((trig - l_trig) > 1) {
			std::cout << "#W Trig: " << std::hex
				<< trig << " prev: " << l_trig << std::endl;
			fdump = true;
		}
		l_id = id_now;
		l_trig = trig;
		if (fdump) {
			char *cpheader = reinterpret_cast<char *>(pheader);
			#if 1
			hexdump(cpheader, nread);
			#else
			std::cout << std::hex;
			for (int i = 0 ; i < hsize ; i++) {
				if ((i % 16) == 0) {
					if (i != 0) std::cout << std::endl;
					std::cout << "# " << std::setw(4) << i << ":";
				}
				std::cout << " " << std::setw(2) << std::setfill('0')
					<< (static_cast<unsigned int>(pdata[i]) & 0xff);
			}
			std::cout << std::endl;
			std::cout << std::dec;
			#endif
		}
	}
	#endif

	//parts.AddPart(NewMessage(bodysize));
	outParts.AddPart(NewMessage(hsize + bodysize));
	//auto &msgRBody = parts[2];
	auto &msg = outParts[1];
	char *cmsgbuf = reinterpret_cast<char *>(msg.GetData());
	memcpy(cmsgbuf, reinterpret_cast<char *>(pheader), sizeof(struct Recbe::Header));
	char *recbe_body = reinterpret_cast<char *>(msg.GetData()) + hsize;
	//nread = fSock.Receive(
	//	reinterpret_cast<char *>(msgRBody.GetData()), bodysize, flag);
	nread = fSock.Receive(recbe_body , bodysize, flag);
	if (nread < bodysize) {
		LOG(warn) << "Reading body: irregal data size " << nread << "/" << bodysize;
		if (flag == EAGAIN) {
			LOG(warn) << "Timeout and Retry";
			int nnread = fSock.Receive(
				recbe_body + nread,
				bodysize - nread, flag);
			nread += nnread;
			if (nread < bodysize) {
				if (flag == EAGAIN) {
					LOG(warn) << "Timeout anagin...";
					hexdump(recbe_body, nread);
				} else {
					LOG(warn) << "Unknown error...";
					hexdump(recbe_body, nread);
				}
			}
		} else {
			LOG(error) << "Unknown err. " << flag;
			hexdump(recbe_body, nread);
		}
	}
	#ifdef CHECK_FREQ 
	nevents++;
	#endif

	struct SubTimeFrame::Header *pstfheader
		 = reinterpret_cast<struct SubTimeFrame::Header *>(msgSTFHeader.GetData());
	pstfheader->magic = SubTimeFrame::Magic;
	pstfheader->timeFrameId = static_cast<uint32_t>(trig);
	pstfheader->FEMType = static_cast<uint32_t>(pheader->type) & 0xff;
	pstfheader->FEMId = static_cast<uint32_t>(pheader->id) & 0xff;
	pstfheader->length = sizeof(struct SubTimeFrame::Header) + sizeof(struct Recbe::Header) + bodysize;
	//pstfheader->numMessages = 3;
	pstfheader->numMessages = 2;
	struct timeval now;
	gettimeofday(&now, nullptr);
	pstfheader->time_sec = now.tv_sec;
	pstfheader->time_usec = now.tv_usec;

	#if 0
	// NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
	// Should only be used for small data because of the cost of an additional copy
	parts.AddPart(NewSimpleMessage(header));
	parts.AddPart(NewMessage(1000));

	// create more data parts, testing the fair::mq::Parts in-place constructor
	fair::mq::Parts auxData{ NewMessage(500), NewMessage(600), NewMessage(700) };
	assert(auxData.Size() == 3);
	parts.AddPart(std::move(auxData));
	assert(auxData.Size() == 0);
	assert(parts.Size() == 5);

	parts.AddPart(NewMessage());
		 from /home/nestdaq/nestdaq/src/userworks/recbe/RecbeSampler.cxx:9:
	assert(parts.Size() == 6);

	parts.AddPart(NewMessage(100));
	assert(parts.Size() == 7);

	LOG(info) << "Sending body of size: " << parts.At(1)->GetSize();
	#endif



	#if 1
	FairMQParts dqmParts;
	bool dqmSocketExists = fChannels.count(fDQMChannelName);
	if (dqmSocketExists) {
		for (auto & m : outParts) {
			FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
			msgCopy->Copy(*m);
			dqmParts.AddPart(std::move(msgCopy));
		}

		if (Send(dqmParts, fDQMChannelName) < 0) {
			if (NewStatePending()) {
				LOG(info) << "Device is not RUNNING";
			} else {
				LOG(error) << "Failed to enqueue dqm-channel";
			}
		} else {
			std::cout << "+" << std::flush;
		}
	} else {
		// std::cout << "NoDQM socket" << std::endl;
	}
	#endif



	#if 0
	Send(outParts, fOutputChannelName);
	#else

	int direction = trig % fNumDestination;
	auto poller = NewPoller(fOutputChannelName);
	while (!NewStatePending()) {
		poller->Poll(fPollTimeoutMS);
		if (poller->CheckOutput(fOutputChannelName, direction)) {
			if (Send(outParts, fOutputChannelName, direction) > 0) {
				// successfully sent
				break;
			} else {
				LOG(error) << "Failed to queue output-channel :"
					<< fOutputChannelName;
			}
		}
		if (fNumDestination == 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	#endif


	++fNumIterations;
	#if 0
	// Wait a second to keep the output readable.
	std::this_thread::sleep_for(std::chrono::seconds(1));
	#endif

	return true;
}


void RecbeSampler::PostRun()
{
	LOG(debug) << __FUNCTION__;
	fNumIterations = 0;
	fSock.Close();
}


//bool RecbeSampler::PreRun()
void RecbeSampler::PreRun()
{
	LOG(debug) << __FUNCTION__;
	if (fTimeout_ms > 0) fSock.SetTimeOut_ms(fTimeout_ms);
	int sock = fSock.Connect(fDeviceIp.c_str(), fDataPort);
	if (sock < 0) {
		LOG(error) << "Connection err. " << fDeviceIp << " " << fDataPort;
		//return false;
		return;
	}
	//return true;
	return;
}


void RecbeSampler::Run()
{
	LOG(debug) << __FUNCTION__;
}
