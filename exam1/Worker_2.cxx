/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <iostream>
#include <string>
#include <atomic>

#include "Worker.h"
#include "runFairMQDevice.h"

#include "mstopwatch.cxx"
mStopWatch *g_sw;

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& /*options*/)
{
}

FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/)
{
    return new Worker();
}


Worker::Worker()
  : fId(0)
{
    OnData("data-in", &Worker::HandleData);
}


Worker::~Worker()
{
}

void Worker::Init()
{
	std::string name = fConfig->GetProperty<std::string>("id");
	std::string snum = name.substr(name.rfind("-") + 1);
	std::istringstream ss(snum);
	ss >> fId;

	g_sw = new mStopWatch();
	g_sw->start();
}

#include "event.h"
std::atomic<int> gQdepth = 0;

bool Worker::HandleData(FairMQMessagePtr& msg, int /* index */)
{

	char *inbuf = static_cast<char *>(msg->GetData());
	int inbuf_size = msg->GetSize();
	struct event_header *ieheader
		= reinterpret_cast<struct event_header *>(inbuf);
	struct node_header *inheader
		= reinterpret_cast<struct node_header *>(
		inbuf + sizeof(struct event_header));
	char *icontainer = inbuf
		+ sizeof(struct event_header) + ieheader->node_header_size;
	
	int outbuf_size = inbuf_size + sizeof(struct node_header);
	char *outbuf = new char[outbuf_size];
	gQdepth++;

	struct event_header *oeheader = reinterpret_cast<struct event_header *>(outbuf);
	memcpy(oeheader, ieheader, sizeof(struct event_header));
	oeheader->node_header_size += sizeof(struct node_header);

	struct node_header *onheader = reinterpret_cast<struct node_header *>(
		outbuf + sizeof(struct event_header));
	memcpy(onheader, inheader, ieheader->node_header_size);
	onheader = reinterpret_cast<struct node_header *>(
		outbuf + sizeof(struct event_header) + ieheader->node_header_size);
	onheader->type = 0x57524b52;
	onheader->id_number = fId;

	char *ocontainer = outbuf
		+ sizeof(struct event_header) + oeheader->node_header_size;
	memcpy(ocontainer, icontainer, ieheader->container_size);

	#if 0
	std::cout << "# fId: " << fId << " : " << index << std::endl;
	unsigned int *data = reinterpret_cast<unsigned int*>(inbuf);
        for (int i = 0 ; i < 32 ; i++) {
		if ((i % 8) == 0) std::cout << std::endl << "#i ";
		std::cout << " " << std::setfill('0') << std::setw(8) << std::hex << data[i];
	}
	data = reinterpret_cast<unsigned int*>(outbuf);
        for (int i = 0 ; i < 32 ; i++) {
		if ((i % 8) == 0) std::cout << std::endl << "#o ";
		std::cout << " " << std::setfill('0') << std::setw(8) << std::hex << data[i];
	}
	std::cout << std::dec << std::endl;
	#endif

    // create message object with a pointer to the data buffer,
    // its size,
    // custom deletion function (called when transfer is done),
    // and pointer to the object managing the data buffer

	FairMQMessagePtr msg2(
		NewMessage(
		outbuf,
		outbuf_size,
		[](void* pdata, void*) {
			char *p = reinterpret_cast<char *>(pdata);
			delete p;
			gQdepth--;
		},
		nullptr));

    // Send out the output message
	if (Send(msg2, "data-out") < 0) {
		return false;
	}

#if 1
	static int counter = 0;
	counter++;
	int elapse = g_sw->elapse();
	if (elapse > 10*1000) {
		double freq = static_cast<double>(counter) / elapse;
		LOG(info) << "Freq. " << freq << " kHz, "
			<< "Q " << gQdepth;
		counter = 0;
		g_sw->restart();
	}
#endif

	//usleep(1);

	return true;
}

#if 0
bool Worker::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
    // LOG(info) << "Received data, processing...";


    // Modify the received string
    std::string* text = new std::string(static_cast<char*>(msg->GetData()), msg->GetSize());
    *text += " (modified by " + fId + ")";
	gQdepth++;

    //std::cout << "# " << *text << " #" << std::endl;

    // create message object with a pointer to the data buffer,
    // its size,
    // custom deletion function (called when transfer is done),
    // and pointer to the object managing the data buffer
    FairMQMessagePtr msg2(
        NewMessage(const_cast<char*>(text->c_str()),
        text->length(),
        [](void* /*data*/, void* object) {
            delete static_cast<std::string*>(object);
            gQdepth--;
	},
        text));

    // Send out the output message
    if (Send(msg2, "data-out") < 0) {
        return false;
    }

#if 1
	static int counter = 0;
	counter++;
	int elapse = g_sw->elapse();
	if (elapse > 10*1000) {
		double freq = static_cast<double>(counter) / elapse;
		LOG(info) << "Freq. " << freq << " kHz, "
			<< "Q " << gQdepth;
		counter = 0;
		g_sw->restart();
	}
#endif

    return true;
}
#endif
