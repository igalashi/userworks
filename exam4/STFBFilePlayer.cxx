#include <algorithm>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <functional>
#include <thread>
#include <cassert>
#include <numeric>
#include <unordered_map>
#include <sstream>
#include <sys/time.h>

#include <fairmq/runDevice.h>

#include "HexDump.h"
#include "MessageUtil.h"
#include "SubTimeFrameHeader.h"
#include "AmQStrTdcData.h"
#include "FileSinkHeaderBlock.h"

#include "STFBFilePlayer.h"


//______________________________________________________________________________
STFBFilePlayer::STFBFilePlayer()
    : fair::mq::Device()
{
    mdebug = false;
}

//______________________________________________________________________________
void STFBFilePlayer::BuildFrame(FairMQMessagePtr& msg, int index)
{
    namespace Data = AmQStrTdc::Data;
    using Word     = Data::Word;
    std::size_t offset = 0;

    (void)index;

    if(mdebug) {
        LOG(debug)
                << " buildframe STF = " << fSTFSequenceNumber << " HBF = " << fHBFCounter << "\n"
                << " input payload entries = " << fInputPayloads.size()
                << " offset " << offset << std::endl;
    }

    auto msgBegin  = reinterpret_cast<Word*>(msg->GetData());
    auto msgSize   = msg->GetSize();
    auto nWord     = msgSize / sizeof(Word);

    if(mdebug) {
        LOG(debug) << " msg size " << msgSize << " bytes " << nWord << " words" << std::endl;
        {
            std::for_each(reinterpret_cast<Word*>(msgBegin),
                          msgBegin+nWord,
                          HexDump{4});
        }
    }

    for (long unsigned int i = 0 ; i < nWord ; ++i) {
        auto word = reinterpret_cast<Data::Bits*>(msgBegin+i);

        uint8_t h = word->head;

        if(mdebug)
            LOG(debug) << " head = " << std::hex << static_cast<uint16_t>(h) << std::dec << std::endl;

        bool isHeadValid = false;
        for (auto validHead : {
                    Data::Data, Data::Heartbeat, Data::SpillOn, Data::SpillEnd
                }) {
            if (h == validHead) isHeadValid = true;
        }

        if (!isHeadValid) {

            LOG(warning)
                    << " " << i << " " << offset
                    << " invalid head = " << std::hex << static_cast<uint16_t>(h)
                    << " " << word->raw << std::dec << std::endl;

            if (i - offset > 0) {
                // std::cout << " fill valid part "  << std::setw(10) << offset << " -> " << std::setw(10) << i << std::endl;
                auto first = msgBegin + offset;
                auto last  = msgBegin + i;
                std::for_each(first, last, HexDump{4});
                fInputPayloads.insert(fInputPayloads.end(), std::make_move_iterator(first), std::make_move_iterator(last));
            }
            offset = i+1;
            continue;
        }

        if ((h == Data::Heartbeat) || h == Data::SpillEnd) {
            if (fLastHeader == 0) {
                fLastHeader = h;
                continue;
            } else if (fLastHeader == h) {
                fLastHeader = 0;
            } else {
                // unexpected @TODO
            }

            int32_t delimiterFrameId = ((word->hbspilln & 0xFF)<<16) | (word->hbframe & 0xFFFF);
            //LOG(debug) << " heartbeat/spill-end delimiter comes " << std::hex << hbframe << ", raw = " << word->raw;
            if (fTimeFrameIdType == TimeFrameIdType::FirstHeartbeatDelimiter) { // first heartbeat delimiter or first spill-off delimiter
                if (fSTFId<0) {
                    fSTFId = delimiterFrameId;
                }
            } else { // last heartbeat delimiter or last spill-off delimiter, or sequence number
                fSTFId = delimiterFrameId;
            }
#if 0
            ++hbf_flag;
            if( hbf_flag == 1 ) {
                continue;

            } else if(hbf_flag == 2) {
                hbf_flag = 0;
            }
#endif
            if(mdebug) {
                LOG(debug) << " Fill " << std::setw(10) << offset << " -> " << std::setw(10) << i << " : " << std::hex << word->raw << std::dec;
            }

            auto first = msgBegin + offset;
            auto last  = msgBegin + i;
            offset     = i+1;

            FillData(first, last, (h==Data::SpillEnd));

            if ( h == Data::SpillEnd ) {
                FinalizeSTF();
                continue;
            }

            if ( h == Data::Heartbeat ) {
                ++fHBFCounter;

                if (fSplitMethod==0) {

                    if ((fHBFCounter % fMaxHBF == 0) && (fHBFCounter>0)) {
                        //	    std::cout << "FinalizeSTF " << std::endl;
                        FinalizeSTF();
                    }
                }
            }
        }
#if 0
        if (  ) {
            if (h == Data::SpillEnd) {
                FillData(msgBegin+i, msgBegin+i+2, true);
            }
            FinalizeSTF();
            i += 1;
        }
#endif
    }

    if(mdebug)
        std::cout << " data remains: " << (nWord - offset) << " offset =  " << offset << std::endl;

    if (offset < nWord) { // && !isSpillEnd)) {
        fInputPayloads.insert(fInputPayloads.end(),
                              std::make_move_iterator(msgBegin + offset),
                              std::make_move_iterator(msgBegin + nWord));
    }

}

//______________________________________________________________________________
void STFBFilePlayer::NewData()
{
    if (!fWorkingPayloads) {
        fWorkingPayloads = std::make_unique<std::vector<FairMQMessagePtr>>();
        fWorkingPayloads->reserve(fMaxHBF*2+1);
        // add an empty message, which will be replaced with sub-time-frame header later.
        fWorkingPayloads->push_back(nullptr);
    }
}

//______________________________________________________________________________
void STFBFilePlayer::FillData(AmQStrTdc::Data::Word* first,
                         AmQStrTdc::Data::Word* last,
                         [[maybe_unused]] bool isSpillEnd)
{
    namespace Data = AmQStrTdc::Data;

    // construct send buffer with remained data on heap
    auto buf = std::make_unique<decltype(fInputPayloads)>(std::move(fInputPayloads));

    if(mdebug) {
        LOG(debug) << " FillData " ;
        std::for_each(first, last, HexDump{4});
    }

    if (last != first) {
        if(mdebug)
            LOG(debug) << " first: "<< first << "  last: " << last;
        // insert new data to send buffer
        buf->insert(buf->end(), std::make_move_iterator(first), std::make_move_iterator(last));
    }

    NewData();
    if (!buf->empty()) {
        fWorkingPayloads->emplace_back(MessageUtil::NewMessage(*this, std::move(buf)));
    }

    if(mdebug) {
        LOG(debug)
                << " single word frame : " << std::hex
                << reinterpret_cast<Data::Bits*>(last)->raw
                << std::dec << std::endl;
    }

    if (fSplitMethod!=0) {
        if ((fHBFCounter % fMaxHBF == 0) && (fHBFCounter>0)) {
            LOG(debug) << " calling FinalizeSTF() from FillData()";
            FinalizeSTF();
            NewData();
        }
    }

    //if (!isSpillEnd) {
    fWorkingPayloads->emplace_back(NewSimpleMessage(*last));
    //}

}

//______________________________________________________________________________
void STFBFilePlayer::FinalizeSTF()
{
    namespace STF  = SubTimeFrame;
    //LOG(debug) << " FinalizeSTF()";
    auto stfHeader          = std::make_unique<STF::Header>();
    if (fTimeFrameIdType==TimeFrameIdType::SequenceNumberOfTimeFrames) {
        stfHeader->timeFrameId = fSTFSequenceNumber;
    } else {
        stfHeader->timeFrameId = fSTFId;
    }
    fSTFId = -1;
    stfHeader->FEMType  = fFEMType;
    stfHeader->FEMId    = fFEMId;
    stfHeader->length   = std::accumulate(
                              fWorkingPayloads->begin(),
                              fWorkingPayloads->end(), sizeof(STF::Header),
    [](auto init, auto& m) {
        return (!m) ? init : init + m->GetSize();
    });
    stfHeader->numMessages  = fWorkingPayloads->size();

    struct timeval curtime;
    gettimeofday(&curtime, NULL);
    stfHeader->time_sec = curtime.tv_sec;
    stfHeader->time_usec = curtime.tv_usec;

    //  std::cout << "femtype:  "<< stfHeader->FEMType << std::endl;
    //  std::cout << "sec:  "<< stfHeader->time_sec << std::endl;
    //  std::cout << "usec: "<< stfHeader->time_usec << std::endl;

    // replace first element with STF header
    fWorkingPayloads->at(0) = MessageUtil::NewMessage(*this, std::move(stfHeader));

    fOutputPayloads.emplace(std::move(fWorkingPayloads));

    ++fSTFSequenceNumber;
    fHBFCounter = 0;
}


//______________________________________________________________________________
bool STFBFilePlayer::HandleData(FairMQMessagePtr& msg, int index)
{
    namespace STF  = SubTimeFrame;
    namespace Data = AmQStrTdc::Data;
    //    using Bits     = Data::Bits;

    if(mdebug)
        std::cout << "HandleData() HBF " << fHBFCounter << " input message " << msg->GetSize() << std::endl;

    //Reporter::AddInputMessageSize(msg->GetSize());


    //  std::cout << "============ data in ============= "<< std::endl;
    //  auto indata_size = msg->GetSize();
    //  std::for_each(reinterpret_cast<uint64_t*>(msg->GetData()),
    //		reinterpret_cast<uint64_t*>(msg->GetData() + msg->GetSize()),
    //		HexDump{4});

    BuildFrame(msg, index);

    while (!fOutputPayloads.empty()) {
        // create a multipart message and move ownership of messages to the multipart message
        FairMQParts parts;
        FairMQParts dqmParts;

        bool dqmSocketExists = fChannels.count(fDQMChannelName);

        if(mdebug)
            std::cout << " send data " << fOutputPayloads.size() << std::endl;

        auto& payload = fOutputPayloads.front();

        for (auto& tmsg : *payload) {
            //      std::for_each(reinterpret_cast<uint64_t*>(tmsg->GetData()),
            //                    reinterpret_cast<uint64_t*>(tmsg->GetData() + tmsg->GetSize()),
            //                    HexDump{4});

            if (dqmSocketExists) {
                if (tmsg->GetSize()==sizeof(STF::Header)) {
                    FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
                    msgCopy->Copy(*tmsg);
                    dqmParts.AddPart(std::move(msgCopy));
                } else {
                    /*
                              auto b = reinterpret_cast<Bits*>(tmsg->GetData());
                              if (b->head == Data::Heartbeat     ||
                                      //	      b->head == Data::ErrorRecovery ||
                    b->head == Data::Data     ||
                    b->head == Data::SpillEnd) {
                                  FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
                                  msgCopy->Copy(*tmsg);
                                  dqmParts.AddPart(std::move(msgCopy));
                      }
                    */

                    FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
                    msgCopy->Copy(*tmsg);
                    dqmParts.AddPart(std::move(msgCopy));
                }
            }

            parts.AddPart(std::move(tmsg));
        }

        fOutputPayloads.pop();

        auto h = reinterpret_cast<STF::Header*>(parts.At(0)->GetData());

        /*
        { // for debug-begin

          std::cout << " parts size = " << parts.Size() << std::endl;
          for (int i=0; i<parts.Size(); ++i){
        const auto& msg = parts.At(i);

        if (i==0) {
          auto stfh = reinterpret_cast<STF::Header*>(msg->GetData());
          LOG(debug) << "STF " << stfh->timeFrameId << " length " << stfh->length << " header " << msg->GetSize() << std::endl;
          auto msize = msg->GetSize();
          std::for_each(reinterpret_cast<uint64_t*>(msg->GetData()),
        		reinterpret_cast<uint64_t*>(msg->GetData() + msize),
        		HexDump{4});
        } else {
          LOG(debug) << " body " << i << " " << msg->GetSize() << " "
        	     << std::showbase << std::hex <<  msg->GetSize() << std::noshowbase<< std::dec << std::endl;
          auto n = msg->GetSize()/sizeof(Data::Word);

          std::for_each(reinterpret_cast<Data::Word*>(msg->GetData()),
        		reinterpret_cast<Data::Word*>(msg->GetData()) + n,
        		HexDump{4});
        }
          }
        } // for debug-end
        */

        // Push multipart message into send queue
        // LOG(debug) << "send multipart message ";

        //Reporter::AddOutputMessageSize(parts);

        if (dqmSocketExists) {
            if (Send(dqmParts, fDQMChannelName) < 0) {
                // timeout
                if (NewStatePending()) {
                    LOG(info) << "Device is not RUNNING";
                    return false;
                }
                LOG(error) << "Failed to enqueue sub time frame (DQM) : FEM = "
                           << std::hex << h->FEMId << std::dec << "  STF = " << h->timeFrameId << std::endl;
            }
        }

        auto direction = (fTimeFrameIdType==TimeFrameIdType::SequenceNumberOfTimeFrames)
                         ? (h->timeFrameId % fNumDestination)
                         : ((h->timeFrameId/fMaxHBF) % fNumDestination);

        if(mdebug)
            std::cout << "direction: " << direction << std::endl;

        unsigned int err_count = 0;
        while (Send(parts, fOutputChannelName, direction, 0) < 0) {
            // timeout
            if (NewStatePending()) {
                LOG(info) << "Device is not RUNNING";
                return false;
            }
            if( err_count < 10 )
                LOG(error) << "Failed to enqueue sub time frame (data) : FEM = "
                    << std::hex << h->FEMId
                    << std::dec << "  STF = " << h->timeFrameId << std::endl;

            err_count++;
        }
    }

    return true;
}


//______________________________________________________________________________
void STFBFilePlayer::InitTask()
{

    using opt = OptionKey;
    fInputChannelName  = fConfig->GetProperty<std::string>(opt::InputChannelName.data());
    fOutputChannelName = fConfig->GetProperty<std::string>(opt::OutputChannelName.data());
    fDQMChannelName    = fConfig->GetProperty<std::string>(opt::DQMChannelName.data());

    fMaxIterations     = std::stoll(fConfig->GetProperty<std::string>(opt::MaxIterations.data()));
    fPollTimeoutMS     = std::stoi(fConfig->GetProperty<std::string>(opt::PollTimeout.data()));

    fTimeFrameIdType   = static_cast<TimeFrameIdType>(
                         std::stoi(fConfig->GetProperty<std::string>(opt::TimeFrameIdType.data())));
    fSTFId = -1;


#if 0
    //////
    FairMQMessagePtr msginfo(NewMessage());
    int nrecv=0;
    while(true) {
        if (Receive(msginfo, fInputChannelName) <= 0) {
            LOG(debug) << __func__ << " Trying to get FEMIfo " << nrecv;
            nrecv++;
        } else {
            break;
        }
    }

    //  fromFEMInfo feminfo;
    auto femInfo = reinterpret_cast<FEMInfo*>(msginfo->GetData());
    auto femID   = femInfo->FEMId;
    auto femType = femInfo->FEMType;

    LOG(info) << "magic: "<< std::hex << femInfo->magic << std::dec;
    LOG(info) << "FEMId: "<< std::hex << femID << std::dec;
    LOG(info) << "FEMType: "<< std::hex << femType << std::dec;
    ////

    fFEMId = femID;
    fFEMType = femType;

    auto s_maxHBF = fConfig->GetProperty<std::string>(opt::MaxHBF.data());
    fMaxHBF = std::stoi(s_maxHBF);
    if (fMaxHBF<1) {
        LOG(warn) << "fMaxHBF: non-positive value was specified = " << fMaxHBF;
        fMaxHBF = 1;
    }
    LOG(debug) << "fMaxHBF = " <<fMaxHBF;
#endif

    auto s_splitMethod = fConfig->GetProperty<std::string>(opt::SplitMethod.data());
    fSplitMethod = std::stoi(s_splitMethod);

    fSTFSequenceNumber = 0;
    fHBFCounter = 0;

    LOG(debug) << " output channels: name = " << fOutputChannelName
               << " num = " << GetNumSubChannels(fOutputChannelName);
    fNumDestination = GetNumSubChannels(fOutputChannelName);
    LOG(debug) << " number of desntination = " << fNumDestination;
    if (fNumDestination < 1) {
        LOG(warn) << " number of destination is non-positive";
    }


    LOG(debug) << " data quality monitoring channels: name = " << fDQMChannelName;
    //	       << " num = " << fChannels.count(fDQMChannelName);

    //    if (fChannels.count(fDQMChannelName)) {
    //        LOG(debug) << " data quality monitoring channels: name = " << fDQMChannelName
    //                   << " num = " << fChannels.at(fDQMChannelName).size();
    //    }

#if 0
    OnData(fInputChannelName, &STFBFilePlayer::HandleData);
#endif

}

// ----------------------------------------------------------------------------
void STFBFilePlayer::PreRun()
{
    fNumIteration = 0;
    fDirection = 0;
    fInputFile.open(fInputFileName.data(), std::ios::binary);
    if (!fInputFile) {
        LOG(error) << " failed to open file = " << fInputFileName;
        return;
    }

    // check FileSinkHeaderBlock
    uint64_t buf{0};
    fInputFile.read(reinterpret_cast<char*>(&buf), sizeof(buf));
    if (fInputFile.gcount() != sizeof(buf)) {
        LOG(warn) << "Failed to read the first 8 bytes";
        return;
    }
    LOG(info) << "check FS header";
    //if (buf == SubTimeFrame::Magic) {
    if (buf != nestdaq::FileSinkHeaderBlock::kMagic) {
        fInputFile.seekg(0, std::ios_base::beg);
        fInputFile.clear();
        LOG(debug) << "no FS header";
    } else {
        uint64_t magic{0};
        fInputFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic == nestdaq::FileSinkHeaderBlock::kMagic) {
            LOG(debug) << " header";
            fInputFile.seekg(buf - 2*sizeof(uint64_t), std::ios_base::cur);
        }
    }
}


//_____________________________________________________________________________
bool STFBFilePlayer::ConditionalRun()
{
    //namespace TF  = TimeFrame;
    namespace STF = SubTimeFrame;

    if (fInputFile.eof()) {
        LOG(warn) << "Reached end of input file. stop RUNNING";
        return false;
    }

    FairMQParts outParts;


#if 0
    // TF header
    outParts.AddPart(NewMessage(sizeof(TF::Header)));
    auto &msgTFHeader = outParts[0];
    fInputFile.read(reinterpret_cast<char*>(msgTFHeader.GetData()), msgTFHeader.GetSize());
    if (static_cast<size_t>(fInputFile.gcount()) < msgTFHeader.GetSize()) {
        LOG(warn) << "No data read. request = " << msgTFHeader.GetSize()
                  << " bytes. gcount = " << fInputFile.gcount();
        return false;
    }
    auto tfHeader = reinterpret_cast<TF::Header*>(msgTFHeader.GetData());

//    LOG(debug4) << fmt::format("TF header: magic = {:016x}, tf-id = {:d}, n-src = {:d}, bytes = {:d}",
//                               tfHeader->magic, tfHeader->timeFrameId, tfHeader->numSource, tfHeader->length);

    std::vector<char> buf(tfHeader->length - sizeof(TF::Header));
    fInputFile.read(buf.data(), buf.size());

    if (static_cast<size_t>(fInputFile.gcount()) < msgTFHeader.GetSize()) {
        LOG(warn) << "No data read. request = " << msgTFHeader.GetSize()
                  << " bytes. gcount = " << fInputFile.gcount();
        return false;
    }
    LOG(debug4) << " buf size = " << buf.size();
    auto bufBegin = buf.data();
    //std::for_each(reinterpret_cast<uint64_t*>(bufBegin), reinterpret_cast<uint64_t*>(bufBegin)+buf.size()/sizeof(uint64_t), nestdaq::HexDump());
#endif


    //for (auto i=0u; i<tfHeader->numSource; ++i) {

        outParts.AddPart(NewMessage(sizeof(STF::Header)));
        //LOG(debug4) << " i-sub = " << i << " size = " << outParts.Size();
        LOG(debug4) << " size = " << outParts.Size();

        auto &msgSTFHeader = outParts[outParts.Size()-1];
        LOG(debug4) << " STF header size =  " << msgSTFHeader.GetSize();

        auto header = reinterpret_cast<char*>(msgSTFHeader.GetData());
        auto headerNBytes = msgSTFHeader.GetSize();
        std::memcpy(header, bufBegin, headerNBytes);
        auto stfHeader = reinterpret_cast<STF::Header*>(header);

        LOG(debug4) << fmt::format("STF header: magic = {:016x}, tf-id = {:d}, rsv = {:08x}, FEM-type = {:08x}, FEM-id = {:08x}, bytes = {:d}, n-msg = {:d}, sec = {:d}, usec = {:d}",
            stfHeader->magic, stfHeader->timeFrameId, stfHeader->reserve, stfHeader->FEMType, stfHeader->FEMId, stfHeader->length, stfHeader->numMessages, stfHeader->time_sec, stfHeader->time_usec);


        auto wordBegin = reinterpret_cast<uint64_t*>(bufBegin + headerNBytes);
        auto bodyNBytes = stfHeader->length - headerNBytes;
        auto nWords = bodyNBytes/sizeof(uint64_t);
        LOG(debug4) << " nWords = " << nWords;

        //std::for_each(wordBegin, wordBegin+nWords, nestdaq::HexDump());

        auto wBegin = wordBegin;
        auto wEnd   = wordBegin + nWords;
        for (auto ptr = wBegin; ptr!=wEnd; ++ptr) {
            auto d = reinterpret_cast<AmQStrTdc::Data::Bits*>(ptr);

//            uint16_t type = d->head;
//            LOG(debug4) << fmt::format(" data type = {:x}", type);

            switch (d->head) {
            //-----------------------------
            case AmQStrTdc::Data::SpillEnd:
            //-----------------------------
            case AmQStrTdc::Data::Heartbeat: {
                outParts.AddPart(NewMessage(sizeof(uint64_t) * (ptr - wBegin + 1)));
                auto & msg = outParts[outParts.Size()-1];
                LOG(debug4) << " found Heartbeat data. " << msg.GetSize() << " bytes";
                std::memcpy(msg.GetData(), reinterpret_cast<char*>(wBegin), msg.GetSize());
                LOG(debug4) << " dump";
                //std::for_each(reinterpret_cast<uint64_t*>(msg.GetData()), reinterpret_cast<uint64_t*>(msg.GetData())+msg.GetSize()/sizeof(uint64_t), nestdaq::HexDump());
                wBegin = ptr+1;
                break;
            }
            //-----------------------------
            default:
                break;
            }
        }
        bufBegin += stfHeader->length;
    //}
    LOG(debug4) << " n-iteration = " << fNumIteration << ": out parts.size() = " << outParts.Size();





    auto poller = NewPoller(fOutputChannelName);
    while (!NewStatePending()) {
        poller->Poll(fPollTimeoutMS);
        auto direction = fDirection % fNumDestination;
        ++fDirection;
        if (poller->CheckOutput(fOutputChannelName, direction)) {
            if (Send(outParts, fOutputChannelName, direction) > 0) {
                // successfully sent
                break;
            } else {
                LOG(warn) << "Failed to enqueue time frame : TF = " << tfHeader->timeFrameId;
            }
        }
    }

    ++fNumIteration;
    if (fMaxIterations>0 && fMaxIterations <= fNumIteration) {
        LOG(info) << "number of iterations of ConditionalRun() reached maximum.";
        return false;
    }
    return true;
}


//______________________________________________________________________________
void STFBFilePlayer::PostRun()
{
    if (fInputFile.is_open()) {
        LOG(info) << " close input file";
        fInputFile.close();
        fInputFile.clear();
    }


#if 0
    fInputPayloads.clear();
    fWorkingPayloads.reset();
    SendBuffer ().swap(fOutputPayloads);

    int nrecv = 0;

    while(true) {
        FairMQMessagePtr msg(NewMessage());

        if (Receive(msg, fInputChannelName) <= 0) {
            LOG(debug) << __func__ << " no data received " << nrecv;
            ++nrecv;
            if (nrecv>10) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } else {
            LOG(debug) << __func__ << " print data";
            //      HandleData(msg, 0);
        }
    }
#endif

    LOG(debug) << __func__ << " done";

}



namespace bpo = boost::program_options;

//______________________________________________________________________________
void addCustomOptions(bpo::options_description& options)
{
    using opt = STFBFilePlayer::OptionKey;
    options.add_options()
    (opt::InputFileName.data(), bpo::value<std::string>(),
     "path to input data file")
    (opt::FEMId.data(),             bpo::value<std::string>(),
     "FEM ID")
    (opt::InputChannelName.data(),  bpo::value<std::string>()->default_value("in"),
     "Name of the input channel")
    (opt::OutputChannelName.data(), bpo::value<std::string>()->default_value("out"),
     "Name of the output channel")
    (opt::DQMChannelName.data(),    bpo::value<std::string>()->default_value("dqm"),
     "Name of the data quality monitoring")

    (opt::MaxIterations.data(), bpo::value<std::string>()->default_value("0"),
     "maximum number of iterations")
    (opt::PollTimeout.data(),   bpo::value<std::string>()->default_value("0"),
     "Timeout of send-socket polling (in msec)");

    (opt::MaxHBF.data(),            bpo::value<std::string>()->default_value("1"),
     "maximum number of heartbeat frame in one sub time frame")
    (opt::SplitMethod.data(),       bpo::value<std::string>()->default_value("0"),
     "STF split method")
    (opt::TimeFrameIdType.data(),   bpo::value<std::string>()->default_value("0"),
     "Time frame ID type:"
     " 0 = first HB delimiter,"
     " 1 = last HB delimiter,"
     " 2 = sequence number of time frames")
    ;
}

//______________________________________________________________________________
std::unique_ptr<fair::mq::Device> getDevice(FairMQProgOptions&)
{
    return std::make_unique<STFBFilePlayer>();
}
