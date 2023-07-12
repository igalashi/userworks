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

#include <fmt/core.h>

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

    fInputFileName     = fConfig->GetProperty<std::string>(opt::InputFileName.data());


    fSTFId = -1;

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

    LOG(debug) << "DQM channels: name = " << fDQMChannelName;

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
        LOG(error) << "PreRun: failed to open file = " << fInputFileName;

	assert(!fInputFile);

        return;
    }

    LOG(info) << "PreRun: Input file: " << fInputFileName;

    // check FileSinkHeaderBlock
    uint64_t buf;
    fInputFile.read(reinterpret_cast<char*>(&buf), sizeof(buf));
    if (fInputFile.gcount() != sizeof(buf)) {
        LOG(warn) << "Failed to read the first 8 bytes";
        return;
    }
    LOG(info) << "check FS header : " << buf;

    if (buf == SubTimeFrame::Magic) {
    //if ((buf[0] != nestdaq::FileSinkHeaderBlock::kMagic)
    //    || (buf[1] != nestdaq::FileSinkHeaderBlock::kMagi)) {
        fInputFile.seekg(0, std::ios_base::beg);
        fInputFile.clear();
        LOG(debug) << "no FS header";
    } else {
        uint64_t magic{0};
        fInputFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic == nestdaq::FileSinkHeaderBlock::kMagic) {
            LOG(debug) << "skip File header : " << magic << " " << buf ;
            fInputFile.seekg(buf - 2*sizeof(uint64_t), std::ios_base::cur);
        }
    }
}


//_____________________________________________________________________________
bool STFBFilePlayer::ConditionalRun()
{
    namespace STF = SubTimeFrame;

    if (fInputFile.eof()) {
        LOG(warn) << "Reached end of input file. stop RUNNING";
        return false;
    }

    FairMQParts outParts;

    outParts.AddPart(NewMessage(sizeof(STF::Header)));
    auto &msgSTFHeader = outParts[0];
    fInputFile.read(reinterpret_cast<char*>(msgSTFHeader.GetData()), msgSTFHeader.GetSize());
    if (static_cast<size_t>(fInputFile.gcount()) < msgSTFHeader.GetSize()) {
        LOG(warn) << "No data read. request = " << msgSTFHeader.GetSize()
                  << " bytes. gcount = " << fInputFile.gcount();
        return false;
    }
    auto stfHeader = reinterpret_cast<STF::Header*>(msgSTFHeader.GetData());

	std::cout << "#D Header size :" << msgSTFHeader.GetSize() << std::endl;
	std::cout << "#D maigic : " << std::hex << stfHeader->magic
		<< " length : " << std::dec << stfHeader->length << std::endl;

	uint32_t *ddd = reinterpret_cast<uint32_t *>(stfHeader);
	for (int ii = 0 ; ii < 32 ; ii++) {
		if ((ii % 8) == 0) std::cout << std::endl << std::hex << std::setw(4) << ii << " : ";
		std::cout << " " << std::setw(8) << std::hex << ddd[ii];
	}
	std::cout << std::dec << std::endl;


    std::vector<char> buf(stfHeader->length - sizeof(STF::Header));
    fInputFile.read(buf.data(), buf.size());

	std::cout << "#D1 maigic : " << std::hex << stfHeader->magic << std::endl;


    if (static_cast<size_t>(fInputFile.gcount()) < msgSTFHeader.GetSize()) {
        LOG(warn) << "No data read. request = " << msgSTFHeader.GetSize()
                  << " bytes. gcount = " << fInputFile.gcount();
        return false;
    }
    LOG(debug4) << " buf size = " << buf.size();
    auto bufBegin = buf.data();

#if 0
    std::for_each(reinterpret_cast<uint64_t*>(bufBegin),
        reinterpret_cast<uint64_t*>(bufBegin)+buf.size()/sizeof(uint64_t),
       	HexDump(4));
#endif


        auto header = reinterpret_cast<char*>(msgSTFHeader.GetData());
        auto headerNBytes = msgSTFHeader.GetSize();
#if 1
        LOG(debug4) << fmt::format(
            "STF header: magic = {:016x}, tf-id = {:d}, rsv = {:08x}, FEM-type = {:08x}, FEM-id = {:08x}, bytes = {:d}, n-msg = {:d}, sec = {:d}, usec = {:d}",
            stfHeader->magic, stfHeader->timeFrameId, stfHeader->reserved, stfHeader->FEMType,
	    stfHeader->FEMId, stfHeader->length, stfHeader->numMessages, stfHeader->time_sec,
	    stfHeader->time_usec);
#endif

        //auto wordBegin = reinterpret_cast<uint64_t*>(bufBegin + headerNBytes);
        auto wordBegin = reinterpret_cast<uint64_t*>(bufBegin);
        auto bodyNBytes = stfHeader->length - headerNBytes;
        auto nWords = bodyNBytes/sizeof(uint64_t);
        LOG(debug4) << " nWords = " << nWords;

        //std::for_each(wordBegin, wordBegin+nWords, nestdaq::HexDump());

        auto wBegin = wordBegin;
        auto wEnd   = wordBegin + nWords;
        for (auto ptr = wBegin; ptr!=wEnd; ++ptr) {
            auto d = reinterpret_cast<AmQStrTdc::Data::Bits*>(ptr);

           #if 0
           uint16_t type = d->head;
           LOG(debug4) << fmt::format(" data type = {:x}", type);
           #endif

            switch (d->head) {
            //-----------------------------
            case AmQStrTdc::Data::SpillEnd:
            //-----------------------------
            case AmQStrTdc::Data::Heartbeat: {
                outParts.AddPart(NewMessage(sizeof(uint64_t) * (ptr - wBegin + 1)));
                auto & msg = outParts[outParts.Size()-1];
                LOG(debug4) << " found Heartbeat data. " << msg.GetSize() << " bytes";
                std::memcpy(msg.GetData(), reinterpret_cast<char*>(wBegin), msg.GetSize());

                #if 1
                // LOG(debug4) << " dump";
                std::for_each(
                    reinterpret_cast<uint64_t*>(msg.GetData()),
		    reinterpret_cast<uint64_t*>(msg.GetData())+msg.GetSize()/sizeof(uint64_t),
		    HexDump(4));
                #endif

                wBegin = ptr+1;
                break;
            }
            //-----------------------------
            default:
                break;
            }
        }

    LOG(debug4) << " n-iteration = " << fNumIteration
        << ": out parts.size() = " << outParts.Size();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));


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
                LOG(warn) << "Failed to enqueue time frame : STF = " << stfHeader->timeFrameId;
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
     "Timeout of send-socket polling (in msec)")

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
