#include <iostream>
#include <iomanip>
#include <chrono>
#include <functional>
#include <thread>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <sstream>

#include <fairmq/Poller.h>
#include <fairmq/runDevice.h>

#include "utility/HexDump.h"
#include "utility/MessageUtil.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "TimeFrameBuilder.h"

//#include "AmQStrTdcData.h"

#if 0
#include "uhbook.cxx"
#include "RedisDataStore.h"
#include "Slowdashify.h"
#endif

namespace bpo = boost::program_options;

//______________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
    using opt = TimeFrameBuilder::OptionKey;
    options.add_options()
    (opt::BufferTimeoutInMs.data(),    bpo::value<std::string>()->default_value("10000"),     "Buffer timeout in milliseconds")
    (opt::InputChannelName.data(),     bpo::value<std::string>()->default_value("in"),        "Name of the input channel")
    (opt::OutputChannelName.data(),    bpo::value<std::string>()->default_value("out"),       "Name of the output channel") 
    (opt::DQMChannelName.data(),       bpo::value<std::string>()->default_value("dqm"),       "Name of the data quality monitoring channel")      
    (opt::PollTimeout.data(),          bpo::value<std::string>()->default_value("0"),         "Timeout (in msec) of polling")
    (opt::DecimatorChannelName.data(), bpo::value<std::string>()->default_value("decimator"), "Name of the decimated output channel")
    (opt::DecimationFactor.data(),     bpo::value<std::string>()->default_value("0"),         "Decimation factor for decimated output channel")
    (opt::DecimationOffset.data(),     bpo::value<std::string>()->default_value("0"),         "Decimation offset for decimated output channel")
    (opt::RedisUrl.data(),             bpo::value<std::string>()->default_value("tcp://127.0.0.1:6379/1"), "URL of redis server and db number")
    (opt::RedisObjUrl.data(),          bpo::value<std::string>()->default_value("tcp://127.0.0.1:6379/3"), "URL of redis server and db number for histogram")
    ;
}

//______________________________________________________________________________
std::unique_ptr<fair::mq::Device> getDevice(FairMQProgOptions&)
{
    return std::make_unique<TimeFrameBuilder>();
}

//______________________________________________________________________________
TimeFrameBuilder::TimeFrameBuilder()
    : fair::mq::Device()
{
}

//______________________________________________________________________________
bool TimeFrameBuilder::ConditionalRun()
{
    namespace STF = SubTimeFrame;
    namespace TF  = TimeFrame;

    static int build_success = 0;
    static int build_fail = 0;
    static std::chrono::system_clock::time_point t_prev;
    std::chrono::system_clock::time_point t_now
        = std::chrono::system_clock::now();
    auto elapse
        = std::chrono::duration_cast<std::chrono::seconds>(t_now - t_prev);

    if (elapse.count() >= 5) {
        auto elapse_m
            = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_prev);
        t_prev = t_now;
        double freq = static_cast<double>(build_success) / elapse_m.count() * 1000.0;

        std::string key_freq = fKeyPrefix + "Frequency";
        try {
            fDb->ts_add(key_freq.c_str(), "*", std::to_string(freq));
        } catch (const std::exception &e) {
            LOG(error) << __FUNCTION__ << " exception : " << e.what();
        } catch (...) {
            LOG(error) << __FUNCTION__ << " exception : unknown";
        }
        #if 0
        std::cout << "#D " << key_freq << " " << std::to_string(freq)
            << " elapse: " << elapse_m.count() << std::endl;
        #endif

        double build_ratio;
            if (build_fail + build_success > 0) {
            build_ratio
                = static_cast<double>(build_success)
                / static_cast<double>(build_fail + build_success)
                * 100;
        } else {
            build_ratio = 0;
        }
        std::string key = fKeyPrefix + "BuildRatio";
        //std::cout << "#D " << key.c_str() << " " << std::to_string(build_ratio) << std::endl;
        try {
            fDb->ts_add(key.c_str(), "*", std::to_string(build_ratio));
        } catch (const std::exception &e) {
            LOG(error) << __FUNCTION__ << " exception : " << e.what();
        } catch (...) {
            LOG(error) << __FUNCTION__ << " exception : unknown";
        }

        build_success = 0;
        build_fail = 0;

        std::string hidname = fKeyObjPrefix + "HitMap";
        fObjDb->write(hidname.c_str(), Slowdashify(*fHId));

    }


    // receive
    FairMQParts inParts;
    if (Receive(inParts, fInputChannelName, 0, 1) > 0) {
        assert(inParts.Size() >= 2);

        LOG(debug4) << " received message parts size = " << inParts.Size() << std::endl;

        auto stfHeader = reinterpret_cast<STF::Header*>(inParts.At(0)->GetData());
        auto stfId     = stfHeader->timeFrameId;

        LOG(debug4) << "stfId: "<< stfId;
        LOG(debug4) << "msg size: " << inParts.Size();

        //auto femId     = stfHeader->FEMId;
        auto femId     = stfHeader->femId;
	//fHId->Fill(static_cast<double>(femId & 0xff));
	fHId->Fill(static_cast<double>(femId & 0xff) + 0.5);
        // std::cout << "#D stfId: "<< femId << " ";

        #if 0
        auto fem     = stfHeader->FEMId;
        auto lastmsg = reinterpret_cast<uint64_t *>(inParts.At(inParts.Size() - 1)->GetData());
        unsigned int type = (lastmsg[0] & 0xfc00'0000'0000'0000) >> 58;
        if ((type == 0x1c) || (type == 0x18) || (type == 0x14)) {
        } else {
            LOG(warn) << "BAD delimitor " << std::hex << lastmsg[0]
                << " FEM: " << std::dec << (fem & 0xff);
        }
        #endif

        if (fTFBuffer.find(stfId) == fTFBuffer.end()) {
            fTFBuffer[stfId].reserve(fNumSource);
        }
        fTFBuffer[stfId].emplace_back(STFBuffer {std::move(inParts), std::chrono::steady_clock::now()});
    }

    // send
    if (!fTFBuffer.empty()) {

        bool dqmSocketExists = fChannels.count(fDQMChannelName);
        unsigned int decimatorNumSubChannels = 0;
        if (fDecimatorNumberOfConnectedPeers > 0) {
            decimatorNumSubChannels = GetNumSubChannels(fDecimatorChannelName);
        }


        // find time frame in ready
        for (auto itr = fTFBuffer.begin(); itr!=fTFBuffer.end();) {
            auto stfId  = itr->first;
            auto& tfBuf = itr->second;

            #if 0
            std::cout << "#D id: " << stfId << " Nbuf: "<< tfBuf.size()
                << " / " << fNumSource << std::endl;
            #endif

            if (tfBuf.size() == static_cast<long unsigned int>(fNumSource)) {

                LOG(debug4) << "All comes : " << tfBuf.size() << " stfId: "<< stfId ;

                // move ownership to complete time frame
                FairMQParts outParts;
                FairMQParts dqmParts;
                
                auto h = std::make_unique<TF::Header>();
                //h->magic       = TF::Magic;
                h->magic       = TF::MAGIC;
                h->timeFrameId = stfId;
                h->numSource   = fNumSource;
                h->length      = std::accumulate(tfBuf.begin(), tfBuf.end(), sizeof(TF::Header),
                    [](auto init, auto& stfBuf) {
                    return init + std::accumulate(stfBuf.parts.begin(), stfBuf.parts.end(), 0,
                        [] (auto jinit, auto& m) {
                        return (!m) ? jinit : jinit + m->GetSize();
                    });
                });

                // for dqm
                if (dqmSocketExists){
                    for (auto& stfBuf: tfBuf) {
                      for (auto& m: stfBuf.parts) {
                        FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
                        msgCopy->Copy(*m);
                        
                        dqmParts.AddPart(std::move(msgCopy));
                      }
                    }
                }
                // LOG(debug) << " length = " << h->length;
                outParts.AddPart(MessageUtil::NewMessage(*this, std::move(h)));
                for (auto& stfBuf: tfBuf) {
                    for (auto& m: stfBuf.parts) {
                        outParts.AddPart(std::move(m));
                    }
                }
                tfBuf.clear();

                // for decimator
                if ((fDecimatorNumberOfConnectedPeers > 0)
                    && (fDecimationFactor > 0)
                    && (fNumSend % fDecimationFactor == fDecimationOffset)) {

                    auto poller = NewPoller(fDecimatorChannelName);
                    poller->Poll(fPollTimeoutMS);
                    for (auto iSubChannel=0u; iSubChannel<decimatorNumSubChannels; ++iSubChannel) {
                        while (!NewStatePending()) {
                            if (poller->CheckOutput(fDecimatorChannelName, iSubChannel)) {
                                auto decimatorParts = MessageUtil::Copy(*this, outParts);
                                // decimator output ready
                                if (Send(decimatorParts, fDecimatorChannelName, iSubChannel) > 0) {
                                    // successfully sent
                                    // LOG(debug) << " successfully send to decimator "
                                    // << fNumSend << " " << fDecimationFactor;
                                    break;
                                } else {
                                    LOG(warn) << "Failed to enqueue to decimator output iSubChannel = "
                                        << iSubChannel << " : TF = " << h->timeFrameId;
                                }
                            }
                        }
                    }
                }

                if (dqmSocketExists) {
                  if (Send(dqmParts, fDQMChannelName) < 0) {
                    if (NewStatePending()) {
                      LOG(info) << "Device is not RUNNING";
                      return false;
                    }
                    LOG(error) << "Failed to enqueue TFB (DQM) ";
                  }
                }
                
                auto poller = NewPoller(fOutputChannelName);
                while (!NewStatePending()) {
                    poller->Poll(fPollTimeoutMS);
                    auto direction = fDirection % fNumDestination;
                    fDirection = direction + 1;
                    if (poller->CheckOutput(fOutputChannelName, direction)) {
                        // output ready

                        if (Send(outParts, fOutputChannelName, direction) > 0) {
                            // successfully sent
                            //LOG(debug) << "successfully sent to out " << direction << " " << fDirection;
                            ++fNumSend;
                            break;
                        } else {
                            LOG(warn) << "Failed to enqueue time frame : TF = " << h->timeFrameId;
                        }
                    }
                    if (fNumDestination==1) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }

                build_success++;

            } else {
                // discard incomplete time frame
                auto dt = std::chrono::steady_clock::now() - tfBuf.front().start;
                if (std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() > fBufferTimeoutInMs) {


                    #if 0
                    LOG(warn) << "Timeframe #" << std::hex << stfId << " incomplete after "
                            << std::dec << fBufferTimeoutInMs << " milliseconds, discarding";
                    //fDiscarded.insert(stfId);
                    #else
                    double build_rate
                        = static_cast<double>(build_success)
                        / static_cast<double>(build_fail + build_success)
                        * 100;
                    std::cout << "x "
                        << std::fixed << std::setprecision(2) << build_rate << "% " << std::flush;
                    #endif

                    #if 0 ////////// TFB Fail info. ///////////
                    ////// under debugging //////
                    std::vector<uint32_t> femid;
                    std::vector<uint32_t> expected = {
                        160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
                        170, 171, 172, 173, 174, 175, 176, 177, 178, 179
                    };
                    std::vector<uint64_t> hb;
                    for (auto & stfBuf : tfBuf) {
                        auto & msg = stfBuf.parts[0];
                        SubTimeFrame::Header *stfheader
                            = reinterpret_cast<SubTimeFrame::Header *>(msg.GetData());
                        //std::cout << " ID" << std::hex << stfheader->FEMId << std::dec;
                        femid.push_back(stfheader->FEMId);
                        for (auto it = expected.begin() ; it != expected.end() ;) {
                            if (*it == (stfheader->FEMId && 0xff)) {
                                it = expected.erase(it);
                            } else {
                                it++;
                            }
                        }
                        //std::for_each(reinterpret_cast<uint64_t*>(msg.GetData()),
                        //    reinterpret_cast<uint64_t*>(msg.GetData() + msg.GetSize()),
                        //    ::HexDump{4});
 
                        if (stfBuf.parts.Size() > 2) {
                                auto & hb0 = stfBuf.parts[2];
                                uint64_t hb00 = (reinterpret_cast<uint64_t *>(hb0.GetData()))[0];
                                hb.push_back(hb00);
                        }
                    }


                    std::sort(femid.begin(), femid.end());
                    //std::cout << "#D lost FEMid :" << stfId << ":";
                    //for (auto & i : expected) std::cout << " " << (i & 0xff);
                    std::cout << "#D FEM TFN: " << stfId << ", N: " << femid.size()
                        << ", tfBuf.size(): " << tfBuf.size()
                        << ", id:";
                    for (auto & i : femid) std::cout << " " << (i & 0xff);
                    std::cout << std::endl;

                    #if 0
                    std::cout << "#D HB :" << stfId << ":";
                    for (auto & i : hb) std::cout << " " << std::hex <<i;
                    std::cout << std::dec << std::endl;
                    #endif
                    #endif

                    /*
                    {// for debug-begin

                      for (auto& stfBuf: tfBuf) {
                        for (auto& m: stfBuf.parts) {
                          std::for_each(reinterpret_cast<uint64_t*>(m->GetData()),
                                        reinterpret_cast<uint64_t*>(m->GetData() + m->GetSize()),
                                        ::HexDump{4});
                        }
                      }
                    }// for debug-end
                    */
                    
                    tfBuf.clear();
                    //LOG(warn) << "Number of discarded timeframes: " << fDiscarded.size();
 
                    build_fail++;

                }
            }        

            // remove empty buffer
            if (tfBuf.empty()) {
                itr = fTFBuffer.erase(itr);
            }
            else {
                ++itr;
            }
        }
        
    }

    return true;
}

//______________________________________________________________________________
void TimeFrameBuilder::Init()
{
}

//______________________________________________________________________________
void TimeFrameBuilder::InitTask()
{
    using opt = OptionKey;
    auto sBufferTimeoutInMs = fConfig->GetProperty<std::string>(opt::BufferTimeoutInMs.data());
    fBufferTimeoutInMs = std::stoi(sBufferTimeoutInMs);
    fInputChannelName  = fConfig->GetProperty<std::string>(opt::InputChannelName.data());
    fOutputChannelName = fConfig->GetProperty<std::string>(opt::OutputChannelName.data());
    fDecimatorChannelName = fConfig->GetProperty<std::string>(opt::DecimatorChannelName.data());

    auto numSubChannels = GetNumSubChannels(fInputChannelName);
    LOG(debug) << " Nsubchnnael[" << fInputChannelName << "]: " << numSubChannels;

    //wait for STB connections
    //std::this_thread::sleep_for(std::chrono::milliseconds(800));

    fNumSource = 0;
    for (auto i=0u; i<numSubChannels; ++i) {
        fNumSource += GetNumberOfConnectedPeers(fInputChannelName, i);
    }

    fDQMChannelName    = fConfig->GetProperty<std::string>(opt::DQMChannelName.data());
    
    LOG(debug) << " input channel : name = " << fInputChannelName
               << " num = " << GetNumSubChannels(fInputChannelName)
               << " num peer = " << GetNumberOfConnectedPeers(fInputChannelName,0);

    LOG(debug) << " number of source = " << fNumSource;

    fNumDestination = GetNumSubChannels(fOutputChannelName);
    fPollTimeoutMS  = std::stoi(fConfig->GetProperty<std::string>(opt::PollTimeout.data()));
    fDecimationFactor = std::stoi(fConfig->GetProperty<std::string>(opt::DecimationFactor.data()));
    fDecimationOffset = std::stoi(fConfig->GetProperty<std::string>(opt::DecimationOffset.data()));
    if ((fDecimationFactor > 0)  && (fDecimationOffset >= fDecimationFactor)) {
        LOG(warn) << "invalid decimation-offset = " << fDecimationOffset << " (< decimation-factor).  Set to 0";
        fDecimationOffset = 0;
    }
    LOG(debug) << " decimation-factor = " << fDecimationFactor << " decimation-offset = " << fDecimationOffset;
    fDecimatorNumberOfConnectedPeers = 0; 
    if (fChannels.count(fDecimatorChannelName) > 0) {
        auto nsub = GetNumSubChannels(fDecimatorChannelName);
        for (auto i = 0u; i<nsub; ++i) {
            fDecimatorNumberOfConnectedPeers += GetNumberOfConnectedPeers(fDecimatorChannelName,i);
        }
    }
    LOG(debug) << "fDecimatorNumberOfConnectedPeers : " << fDecimatorNumberOfConnectedPeers << std::endl;


    static bool atFirst = true;
    if (atFirst) {
        atFirst = false;
        std::string service_name = fConfig->GetProperty<std::string>("service-name");
        std::string separator   = fConfig->GetProperty<std::string>("separator");
        fKeyPrefix = "ts" + separator + fId + separator;
        fKeyObjPrefix = "dqm" + separator + fId + separator;

        fRedisUrl = fConfig->GetProperty<std::string>(opt::RedisUrl.data());
        fDb = std::make_unique<RedisDataStore>(fRedisUrl);
        fRedisObjUrl = fConfig->GetProperty<std::string>(opt::RedisObjUrl.data());
        fObjDb = std::make_unique<RedisDataStore>(fRedisObjUrl);

        LOG(info) << "DQM DB: " << fRedisUrl << " " << fRedisObjUrl;
        LOG(info) << "DQM Prefix: " << fKeyPrefix << " " << fKeyObjPrefix;

        //fHId = new UH1Book("FE ID", 256, 0, 256);
        fHId = new UH1Book("FE ID", 105, 0, 105);

    } else {
        fHId->Reset();
    }
}

//______________________________________________________________________________
void TimeFrameBuilder::PostRun()
{
    fTFBuffer.clear();
    //fDiscarded.clear();

    int nrecv=0;
    if (fChannels.count(fInputChannelName) > 0) {
        auto n = fChannels.count(fInputChannelName);

        for (auto i = 0u; i < n; ++i) {
            // std::cout << " #D SubChannel : "<< i << std::endl;
            while(true) {

                FairMQParts part;
                if (Receive(part, fInputChannelName, i, 1000) <= 0) {
                    LOG(debug) << __func__ << " no data received " << nrecv;
                    ++nrecv;
                    if (nrecv > 10) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                } else {
                    LOG(debug) << __func__ << " data comes..";
                }
            }
        }
    }// for clean up

}

//_____________________________________________________________________________
void TimeFrameBuilder::PreRun()
{
    fDirection    = 0;
    fNumSend = 0;
}
