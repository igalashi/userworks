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
#include <string>
#include <chrono>
#include <cstdint>

#include <thread> // this_thread::sleep_for
#include "Header.h"

#include <arpa/inet.h>
#include <sys/time.h>

#include "SubTimeFrameHeader.h"
#include "recbe.h"
#include "CliSock.cxx"


namespace bpo = boost::program_options;

class RecbeSampler : public fair::mq::Device
{
public:
	struct OptionKey {
		static constexpr std::string_view DeviceIp          {"ip"};
		static constexpr std::string_view DataPort          {"port"};
		static constexpr std::string_view Timeout_ms        {"timeout"};
		static constexpr std::string_view ControlPort       {"control-port"};
		static constexpr std::string_view OutputChannelName {"out-chan-name"};
	};

	RecbeSampler() : fair::mq::Device() {};
	RecbeSampler(const RecbeSampler&)            = delete;
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
	CliSock fSock;
	std::string fOutputChannelName;
	std::string fDeviceIp     {"000.000.000.000"};
	unsigned int fDataPort   = 24;
	unsigned int fControlPort = 4660;
	unsigned int fDeviceType  = 0x100;
	unsigned int fTimeout_ms = 0;
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
			bpo::value<std::string>()->default_value("0"),
			"Timeout of the front-end deive")
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
	fDeviceIp          = fConfig->GetProperty<std::string>(opt::DeviceIp.data());
	fDataPort          = std::stoi(fConfig->GetProperty<std::string>(opt::DataPort.data()));
	fControlPort       = std::stoi(fConfig->GetProperty<std::string>(opt::ControlPort.data()));
	LOG(info) << "IP Address: " << fDeviceIp << ", Port: " << fDataPort << ", " << fControlPort;
	//fDeviceType        = std::stoi(fConfig->GetProperty<std::string>("DeviceType"));
	//LOG(info) << "Device Type: " << fDeviceType;
	fTimeout_ms        = std::stoi(fConfig->GetProperty<std::string>(opt::Timeout_ms.data()));

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

	fair::mq::Parts parts;

	parts.AddPart(NewMessage(sizeof(SubTimeFrame::Header)));
	auto &msgSTFHeader = parts[0];

	int hsize = sizeof(struct Recbe::Header);
	parts.AddPart(NewMessage(hsize));
	auto &msgRHeader = parts[1];
	int flag;
	int nread = fSock.Receive(
		reinterpret_cast<char *>(msgRHeader.GetData()), hsize, flag);
	if (nread != hsize) {
		LOG(error) << "irregal data size" << nread << "/" << hsize;
		if (flag == EAGAIN) LOG(error) << "Timeout";
	}
	struct Recbe::Header *pheader;
	pheader = reinterpret_cast<struct Recbe::Header *>(msgRHeader.GetData());
	int bodysize = static_cast<int>(ntohs(pheader->len));
	int trig = static_cast<int>(ntohl(pheader->trig_count));

	#if 1
	std::cout << std::hex
		<< "#D Type: " << (static_cast<int>(pheader->type) & 0xff)
		<< " ID: " << (static_cast<int>(pheader->id) & 0xff)
		<< std::endl;
	std::cout << "#D len: " << std::dec << bodysize
		<< " Sent: " << std::dec << ntohs(pheader->sent_num)
		<< " Trig: " << trig << " Time: " << ntohs(pheader->time)
		<< std::endl;;
	#endif

	parts.AddPart(NewMessage(bodysize));
	auto &msgRBody = parts[2];
	nread = fSock.Receive(
		reinterpret_cast<char *>(msgRBody.GetData()), bodysize, flag);
	if (nread != bodysize) {
		LOG(error) << "irregal data size" << nread << "/" << bodysize;
		if (flag == EAGAIN) LOG(error) << "Timeout";
	}

	struct SubTimeFrame::Header *pstfheader
		 = reinterpret_cast<struct SubTimeFrame::Header *>(msgSTFHeader.GetData());
	pstfheader->magic = SubTimeFrame::Magic;
	pstfheader->timeFrameId = static_cast<uint32_t>(trig);
	pstfheader->FEMType = static_cast<uint32_t>(pheader->type) & 0xff;
	pstfheader->FEMId = static_cast<uint32_t>(pheader->id) & 0xff;
	pstfheader->length = sizeof(struct SubTimeFrame::Header) + sizeof(struct Recbe::Header) + bodysize;
	pstfheader->numMessages = 3;
	struct timeval now;
	gettimeofday(&now, nullptr);
	pstfheader->time_sec = now.tv_sec;
	pstfheader->time_usec = now.tv_usec;

	#if 1
	#endif


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

	Send(parts, fOutputChannelName);

	#if 0
	// Go out of the sending loop if the stopFlag was sent.
	if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
		LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
		return false;
	}
	#else
	++fNumIterations;
	#endif

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
