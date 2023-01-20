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

#include "utility/HexDump.h"
#include "MessageUtil.h"
#include "SubTimeFrameHeader.h"
#include "Reporter.h"
#include "HulStrTdcSTFBuilder.h"

namespace STF  = highp::e50::SubTimeFrame;

//______________________________________________________________________________
highp::e50::
HulStrTdcSTFBuilder::HulStrTdcSTFBuilder()
  : FairMQDevice()
{
}

//______________________________________________________________________________
void 
highp::e50::
HulStrTdcSTFBuilder::BuildFrame(FairMQMessagePtr& msg, int index)
{
  namespace Data = HulStrTdc::Data;
  using Word     = Data::Word;
  std::size_t offset = 0;

  // LOG(debug)
  //   << " buildframe STF = " << fSTFId << " HBF = " << fHBFCounter << "\n"
  //   << " input payload entries = " << fInputPayloads.size() 
  //   << " offset " << offset << std::endl;

  auto msgBegin  = reinterpret_cast<Word*>(msg->GetData());
  auto msgSize   = msg->GetSize();
  auto nWord     = msgSize / sizeof(Word);

  //LOG(debug) << " msg size " << msgSize << " bytes " << nWord << " words" << std::endl;
  // {
  //   std::for_each(reinterpret_cast<Word*>(msgBegin), 
  //                 msgBegin+nWord,
  //                 //msgBegin+100,
  //                 HexDump{4});
  // }
  
  for (auto i=0; i<nWord; ++i) {
    auto word = reinterpret_cast<Data::Bits*>(msgBegin+i);
    // LOG(debug) << " idx = " << std::setw(10) << i << " (" << std::hex << std::setw(10) << i << ") "
    //            << " i-offset = " << std::setw(10) << i-offset << " (" << std::hex << std::setw(10) << i-offset << ") "
    //            << HexDump::cast<uint64_t>(*word)
    //            << std::dec << std::endl;

    uint8_t h = word->head;
    // LOG(debug) << " head = " << std::hex << static_cast<uint16_t>(h) << std::dec << std::endl;
    bool isHeadValid = false;
    for (auto validHead : {Data::Data, Data::Heartbeat, Data::ErrorRecovery, Data::SpillEnd}) {
      if (h == validHead) isHeadValid = true;
    }

    if (!isHeadValid) { 
      // if (h!=0) {
      //   LOG(warning)
      //     << " " << i << " " << offset << " invalid head = " << std::hex << static_cast<uint16_t>(h) 
      //     << " " << word->raw << std::dec << std::endl;
      // }
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

    if ((h == Data::Heartbeat) || (h == Data::ErrorRecovery) || (h == Data::SpillEnd)) {
      // LOG(debug) << " Fill " << std::setw(10) << offset << " -> " << std::setw(10) << i << " : " << std::hex << word->raw << std::dec;
      auto first = msgBegin + offset;
      auto last  = msgBegin + i;
      offset     = i+1;
      FillData(first, last, (h==Data::SpillEnd));
      if ((h==Data::Heartbeat) || (h==Data::ErrorRecovery)) {
        ++fHBFCounter;
        if (fSplitMethod==0) {
          if ((fHBFCounter % fMaxHBF == 0) && (fHBFCounter>0)) {
            FinalizeSTF();
          }
        }
      }
    }

    if (h == Data::SpillEnd) {
      FillData(msgBegin+i, msgBegin+offset, true);
      FinalizeSTF();
    }
  }
  
  // std::cout << " data remains: " << (nWord - offset) << " offset =  " << offset << std::endl;
  if (offset < nWord) { // && !isSpillEnd)) {
    fInputPayloads.insert(fInputPayloads.end(), 
                          std::make_move_iterator(msgBegin + offset), 
                          std::make_move_iterator(msgBegin + nWord));
  }

}

//______________________________________________________________________________
void 
highp::e50::
HulStrTdcSTFBuilder::FillData(HulStrTdc::Data::Word* first, 
                              HulStrTdc::Data::Word* last, 
                              bool isSpillEnd)
{
  namespace Data = HulStrTdc::Data;
  // construct send buffer with remained data on heap
  auto buf = std::make_unique<decltype(fInputPayloads)>(std::move(fInputPayloads));

  if (last != first) {
    // LOG(debug) << " first != last";
    // insert new data to send buffer
    buf->insert(buf->end(), std::make_move_iterator(first), std::make_move_iterator(last));
  }

  NewData();
  if (!buf->empty()) {
    fWorkingPayloads->emplace_back(MessageUtil::NewMessage(*this, std::move(buf))); 
  }

  // LOG(debug)
  //   << " single word frame : " << std::hex
  //   << reinterpret_cast<Data::Bits*>(last)->raw
  //   << std::dec << std::endl;

  if (fSplitMethod!=0) {
    if ((fHBFCounter % fMaxHBF == 0) && (fHBFCounter>0)) {
      // LOG(debug) << " calling FinalizeSTF() from FillData()"; 
      FinalizeSTF();
      NewData();
    }
  }

  if (!isSpillEnd) {
    // LOG(debug) << " not spill-end";
    fWorkingPayloads->emplace_back(NewSimpleMessage(*last));
  }

}

//______________________________________________________________________________
void 
highp::e50::
HulStrTdcSTFBuilder::FinalizeSTF()
{
  //LOG(debug) << " FinalizeSTF()";
  auto stfHeader          = std::make_unique<STF::Header>(); 
  stfHeader->timeFrameId  = fSTFId;
  stfHeader->FEMId        = fFEMId;
  stfHeader->length       = std::accumulate(fWorkingPayloads->begin(), fWorkingPayloads->end(), sizeof(STF::Header), 
                                            [](auto init, auto& m) { return (!m) ? init : init + m->GetSize(); });
  stfHeader->numMessages  = fWorkingPayloads->size();

  // replace first element with STF header
  fWorkingPayloads->at(0) = MessageUtil::NewMessage(*this, std::move(stfHeader));

  fOutputPayloads.emplace(std::move(fWorkingPayloads));

  ++fSTFId;
  fHBFCounter = 0;
}


//______________________________________________________________________________
bool
highp::e50::
HulStrTdcSTFBuilder::HandleData(FairMQMessagePtr& msg, int index)
{
  namespace Data = HulStrTdc::Data;
  using Word     = Data::Word;
  using Bits     = Data::Bits;
  //LOG(debug)
  // std::cout << "HandleData() HBF " << fHBFCounter << " input message " << msg->GetSize() << std::endl;
  Reporter::AddInputMessageSize(msg->GetSize());
  BuildFrame(msg, index);
  
  while (!fOutputPayloads.empty()) {
    // create a multipart message and move ownership of messages to the multipart message
    FairMQParts parts;
    FairMQParts dqmParts;

    bool dqmSocketExists = fChannels.count(fDQMChannelName);

    // std::cout << " send data " << fOutputPayloads.size() << std::endl;
    auto& payload = fOutputPayloads.front();
    for (auto& msg : *payload) {
      //std::for_each(reinterpret_cast<uint64_t*>(msg->GetData()), 
      //              reinterpret_cast<uint64_t*>(msg->GetData() + msg->GetSize()), 
      //              HexDump{4});

      if (dqmSocketExists) {
        if (msg->GetSize()==sizeof(STF::Header)) {
          FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
          msgCopy->Copy(*msg);
          dqmParts.AddPart(std::move(msgCopy));
        }
        else {
          auto b = reinterpret_cast<Bits*>(msg->GetData());
          if (b->head == Data::Heartbeat     ||
              b->head == Data::ErrorRecovery || 
              b->head == Data::SpillEnd) {
            FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
            msgCopy->Copy(*msg);
            dqmParts.AddPart(std::move(msgCopy));
          }
        }
      }

      parts.AddPart(std::move(msg));
    }
    fOutputPayloads.pop();

    auto h = reinterpret_cast<STF::Header*>(parts.At(0)->GetData());

    // { // for debug-begin
    //   std::cout << " parts size = " << parts.Size() << std::endl;
    //   for (int i=0; i<parts.Size(); ++i){
    //     const auto& msg = parts.At(i);
    //     if (i==0) {
    //       auto stfh = reinterpret_cast<STF::Header*>(msg->GetData());
    //       LOG(debug) << "STF " << stfh->timeFrameId << " length " << stfh->length << " header " << msg->GetSize() << std::endl;
    //       std::for_each(reinterpret_cast<uint64_t*>(msg->GetData()),
    //                     reinterpret_cast<uint64_t*>(msg->GetData() + msg->GetSize()),
    //                     HexDump{4});
    //     }
    //     else {
    //       LOG(debug) << " body " << i << " " << msg->GetSize() << " " 
    //                  << std::showbase << std::hex <<  msg->GetSize() << std::noshowbase<< std::dec << std::endl;
    //       auto n = msg->GetSize()/sizeof(Word);
    //       // n = (n>10) ? 10 : n;
    //       std::for_each(reinterpret_cast<Word*>(msg->GetData()),
    //                     reinterpret_cast<Word*>(msg->GetData()) + n,
    //                     HexDump{4});
    //       if (n == 1) {
    //         auto v = reinterpret_cast<Word*>(msg->GetData());
    //         std::cout << "single word = " << std::hex << HexDump::cast<uint64_t>(*v) << std::dec << std::endl;
    //       }
    //     }
    //   }
    // } // for debug-end

    // Push multipart message into send queue
    // LOG(debug) << "send multipart message ";

    Reporter::AddOutputMessageSize(parts);

    if (dqmSocketExists) {
      if (Send(dqmParts, fDQMChannelName) < 0) {
        // timeout
        //if (!CheckCurrentState(RUNNING)) {
        if (GetCurrentState() != fair::mq::State::Running) {
          LOG(INFO) << "Device is not RUNNING"; 
          return false;
        }
        LOG(ERROR) << "Failed to enqueue sub time frame (DQM) : FEM = " << std::hex << h->FEMId << std::dec << "  STF = " << h->timeFrameId << std::endl;
      }
    }

    auto direction = h->timeFrameId % fNumDestination;
    while (Send(parts, fOutputChannelName, direction, 0) < 0) {
      // timeout
      //if (!CheckCurrentState(RUNNING)) {
      if (GetCurrentState() != fair::mq::State::Running) {
        LOG(INFO) << "Device is not RUNNING"; 
        return false;
      }
      LOG(ERROR) << "Failed to enqueue sub time frame (data) : FEM = " << std::hex << h->FEMId << std::dec << "  STF = " << h->timeFrameId << std::endl;
    }
  }

  return true;
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcSTFBuilder::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcSTFBuilder::InitTask()
{
  using opt = OptionKey;

  {
    // convert IP address  to 32 bit. e.g. 192.168.10.16 -> FEM id = 0xC0A80A10
    fFEMId = 0;
    auto femid = fConfig->GetValue<std::string>(opt::FEMId.data());
    std::istringstream iss(femid);
    std::string token;
    int i = 3;
    while (std::getline(iss, token, '.')) {
      if (i<0)  break;
      uint32_t v = (std::stoul(token) & 0xff) << (8*i);
      // std::cout << " i = " << i << " token = " << token << " v = " << v << std::endl;
      fFEMId |= v;
      --i;
    }
    LOG(debug) << "FEM ID " << std::hex << fFEMId << std::dec << std::endl;
  }

  fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
  fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());
  fDQMChannelName    = fConfig->GetValue<std::string>(opt::DQMChannelName.data());

  // std::cout << "input = " << fInputChannelName 
  //           << " output = " << fOutputChannelName
  //           << std::endl;

  fMaxHBF = fConfig->GetValue<int>(opt::MaxHBF.data());
  LOG(debug) << "fMaxHBF = " <<fMaxHBF; 

  fSplitMethod = fConfig->GetValue<int>(opt::SplitMethod.data());

  fSTFId      = 0;
  fHBFCounter = 0;

  LOG(debug) << " output channels: name = " << fOutputChannelName
	     << " num = " << fChannels.at(fOutputChannelName).size();
  fNumDestination = fChannels.at(fOutputChannelName).size();
  LOG(debug) << " number of desntination = " << fNumDestination;

  if (fChannels.count(fDQMChannelName)) {
    LOG(debug) << " data quality monitoring channels: name = " << fDQMChannelName
               << " num = " << fChannels.at(fDQMChannelName).size();
  }


  OnData(fInputChannelName, &HulStrTdcSTFBuilder::HandleData);

  Reporter::Reset();
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcSTFBuilder::NewData()
{
  if (!fWorkingPayloads) {
    fWorkingPayloads = std::make_unique<std::vector<FairMQMessagePtr>>();
    fWorkingPayloads->reserve(fMaxHBF*2+1);
    // add an empty message, which will be replaced with sub-time-frame header later.
    fWorkingPayloads->push_back(nullptr);
  }
}

//______________________________________________________________________________
void
highp::e50::
HulStrTdcSTFBuilder::PostRun()
{
  fInputPayloads.clear();
  fWorkingPayloads.reset();
  SendBuffer ().swap(fOutputPayloads);
}
