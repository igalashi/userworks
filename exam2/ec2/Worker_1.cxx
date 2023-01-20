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
{
    OnData("data-in", &Worker::HandleData);

	g_sw = new mStopWatch();
	g_sw->start();
}


Worker::~Worker()
{
}

std::atomic<int> gQdepth = 0;

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

