/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

//#include <fairmq/devices/Splitter.h>
#include "Splitter.h"
//#include <fairmq/runDevice.h>
#include <runFairMQDevice.h>

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("in-channel", bpo::value<std::string>()->default_value("data-in"), "Name of the input channel")
        ("out-channel", bpo::value<std::string>()->default_value("data-out"), "Name of the output channel")
        ("multipart", bpo::value<bool>()->default_value(true), "Handle multipart payloads");
}

//std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/)
{
    //return std::make_unique<fair::mq::Splitter>();
    //return new fair::mq::Splitter();
    return new Splitter();
}

//namespace fair::mq
//{

void Splitter::InitTask()
{
    fMultipart = fConfig->GetProperty<bool>("multipart");
    fInChannelName = fConfig->GetProperty<std::string>("in-channel");
    fOutChannelName = fConfig->GetProperty<std::string>("out-channel");
    fNumOutputs = fChannels.at(fOutChannelName).size();
    fDirection = 0;

    if (fMultipart) {
        OnData(fInChannelName, &Splitter::HandleData<FairMQParts>);
    } else {
        OnData(fInChannelName, &Splitter::HandleData<FairMQMessagePtr>);
    }
}

template<typename T>
bool Splitter::HandleData(T& payload, int)
{
    Send(payload, fOutChannelName, fDirection);

    if (++fDirection >= fNumOutputs) {
        fDirection = 0;
    }

    return true;
}

//} //namespace fair::mq
