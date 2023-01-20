#include <algorithm>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <functional>
#include <thread>
#include <cassert>
#include <numeric>

#include "utility/HexDump.h"
#include "RAWHeader.h"
#include "HeartbeatFrameHeader.h"
#include "MessageUtil.h"
#include "SubTimeFrameHeader.h"
#include "Reporter.h"
#include "SubTimeFrameBuilder.h"

namespace STF = highp::e50::SubTimeFrame;

//______________________________________________________________________________
void CopyMessage(FairMQMessagePtr& src, 
                 uint64_t srcOffset, 
                 uint64_t srcLength, 
                 std::vector<uint8_t>& dest)
{
  auto srcBegin = reinterpret_cast<uint8_t*>(src->GetData()) + srcOffset;
  auto srcEnd   = srcBegin + srcLength;

  std::copy(srcBegin, srcEnd, std::back_inserter(dest));
}


//______________________________________________________________________________
highp::e50::
SubTimeFrameBuilder::SubTimeFrameBuilder()
  : FairMQDevice()
{
}

//______________________________________________________________________________
void 
highp::e50::
SubTimeFrameBuilder::BuildFrame(FairMQParts& msgParts, int index)
{
  namespace HBF = HeartbeatFrame;

  assert(msgParts.Size() != 0);
  assert(msgParts.Size() == 2);

  auto  rawHeader  = reinterpret_cast<RAW::Header*>(msgParts.At(0)->GetData());

  // std::cout << " dump raw header " << sizeof(RAW::Header) << " bytes" << std::endl;
  // std::for_each(reinterpret_cast<uint64_t*>(rawHeader), 
  //               reinterpret_cast<uint64_t*>(rawHeader) + sizeof(RAW::Header)/sizeof(uint64_t), 
  //               HexDump{});

  // add the newest payload into the receive buffer
  fInputPayloads.emplace_back(std::move(msgParts.At(1)));

  // search heartbeat frame in last payload
  //std::cout << " input payload entries = " << fInputPayloads.size() << std::endl;

  auto rBegin       = fInputPayloads.rbegin();
  auto lastMsgBegin = reinterpret_cast<uint8_t*>((*rBegin)->GetData());
  auto lastMsgSize  = (*rBegin)->GetSize();
  auto n            = lastMsgSize - sizeof(HBF::Header); 

  // std::for_each(reinterpret_cast<uint64_t*>(lastMsgBegin), 
  //               reinterpret_cast<uint64_t*>(lastMsgBegin)+lastMsgSize/sizeof(uint64_t), 
  //               HexDump{4});

  for (auto i=0; i<n; ) {
    auto hbfHeader = reinterpret_cast<HBF::Header*>(lastMsgBegin + i);
    //std::cout << " idx " << i << std::endl;
    if (hbfHeader->magic == HBF::Magic) {
      // std::cout << " HBF found " << i << std::endl;
      // heart beat frame found
      if (!fWorkingPayloads) {
        // std::cout << " create message buffer " << std::endl;
        fWorkingPayloads = std::make_unique<std::vector<FairMQMessagePtr>>();
        fWorkingPayloads->reserve(fMaxHBF*2+1);

        // add an empty message, which will be replaced with sub-time-frame header later.
        fWorkingPayloads->push_back(nullptr);
      }

      // calculate data size of sub time frame
      auto totalSize = 0;
      auto iBegin    = fInputPayloads.begin();
      auto itr       = iBegin;

      totalSize -= fFirst;
      
      for (; (&*itr) != (&*rBegin); ++itr) {
        totalSize += (*itr)->GetSize();
      }
      totalSize += i;

      // allocate message buffer
      std::unique_ptr<std::vector<uint8_t>> msgBody{std::make_unique<std::vector<uint8_t>>()}; 
      msgBody->reserve(totalSize);

      // copy payload of sub time frame to the allocated buffer
      itr = iBegin;

      // iteration ends at the object before the last one
      for (; (&*itr) != (&*rBegin); ++itr) { 
        auto srcOffset = (itr==iBegin) ? fFirst : 0;
        auto srcLength = (itr==iBegin) ? ((*itr)->GetSize() - fFirst) : (*itr)->GetSize();
        CopyMessage(*itr, srcOffset, srcLength, *msgBody);
        fInputPayloads.pop_front();
      }

      CopyMessage(*itr, 0, i, *msgBody); 
      if (!msgBody->empty()) {
        fWorkingPayloads->emplace_back(MessageUtil::NewMessage(*this, std::move(msgBody))); 
      }
      
      // add heartbeat frame
      // create message by copying
      fWorkingPayloads->emplace_back(NewSimpleMessage(*hbfHeader));

      // update start pointer 
      fFirst = i + sizeof(HBF::Header);
      if (fFirst >= lastMsgSize) {
        fFirst = 0;
        fInputPayloads.pop_front();
      }

      ++fHBFCounter;
      if (fHBFCounter % fMaxHBF == 0) {
        // finalize sub time frame and move the ownership to send buffer
        // std::cout << " finalize " << std::endl;

        auto stfHeader          = std::make_unique<STF::Header>(); 
        stfHeader->timeFrameId  = fSTFId;
        stfHeader->FEMId        = rawHeader->word1;

        stfHeader->length       = std::accumulate(fWorkingPayloads->begin(), fWorkingPayloads->end(), sizeof(STF::Header), 
                                                  [](auto init, auto& m) { return (!m) ? init : init + m->GetSize(); });
        stfHeader->numMessages  = fWorkingPayloads->size();

        // replace first element with STF header
        fWorkingPayloads->at(0) = MessageUtil::NewMessage(*this, std::move(stfHeader));


        fOutputPayloads.emplace(std::move(fWorkingPayloads));

        ++fSTFId;
      }

      i += sizeof(HBF::Header);
    }
    else {
      ++i;
    }
  }
  
  
}

//______________________________________________________________________________
bool
highp::e50::
SubTimeFrameBuilder::HandleData(FairMQParts& msgParts, int index)
{
  Reporter::AddInputMessageSize(msgParts);
  BuildFrame(msgParts, index);
  
  while (!fOutputPayloads.empty()) {
    // create a multipart message and move ownership of messages to the multipart message
    FairMQParts parts;
    auto& payload = fOutputPayloads.front();
    for (auto& msg : *payload) {
      // std::for_each(reinterpret_cast<uint64_t*>(msg->GetData()), 
      //               reinterpret_cast<uint64_t*>(msg->GetData()) + msg->GetSize()/sizeof(uint64_t), 
      //               HexDump{4});
      parts.AddPart(std::move(msg));
    }
    fOutputPayloads.pop();

    auto h = reinterpret_cast<STF::Header*>(parts.At(0)->GetData());
    // push multipart message into send queue
    Reporter::AddOutputMessageSize(parts);

    auto direction = h->timeFrameId % fNumDestination;
    while ((Send(parts, fOutputChannelName, direction, 0) < 0)) {
      //if (!CheckCurrentState(RUNNING)) { 
      if (GetCurrentState() != fair::mq::State::Running) { 
        LOG(INFO) << " Device is not RUNNIG";
        return false;
      }
      // timeout
      LOG(ERROR) << "Failed to queue sub time frame : FEM = " << h->FEMId << "  STF = " << h->timeFrameId;
    }
  }

  

  return true;
}

//______________________________________________________________________________
void
highp::e50::
SubTimeFrameBuilder::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
SubTimeFrameBuilder::InitTask()
{
  using opt = OptionKey;
  fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
  fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());
  fMaxHBF            = fConfig->GetValue<int>(opt::MaxHBF.data());
  
  fFirst = 0;
  fSeek  = 0;

  fSTFId      = 0;
  fHBFCounter = 0;

  LOG(debug) << " output channels: name = " << fOutputChannelName
	     << " num = " << fChannels.at(fOutputChannelName).size();
  fNumDestination = fChannels.at(fOutputChannelName).size();
  LOG(debug) << " number of desntination = " << fNumDestination;


  OnData(fInputChannelName, &SubTimeFrameBuilder::HandleData);

  Reporter::Reset();
}

//______________________________________________________________________________
void 
highp::e50::
SubTimeFrameBuilder::PostRun()
{
  fInputPayloads.clear();
  fWorkingPayloads.reset();
  SendBuffer ().swap(fOutputPayloads);
}
