#include <chrono>
#include <functional>
#include <thread>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <sstream>

#include "utility/HexDump.h"

#include "SubTimeFrameHeader.h"
#include "MessageUtil.h"
#include "TimeFrameHeader.h"
#include "Reporter.h"
#include "TimeFrameBuilder.h"

//______________________________________________________________________________
highp::e50::
TimeFrameBuilder::TimeFrameBuilder()
  : FairMQDevice()
{
}

//______________________________________________________________________________
bool
highp::e50::
TimeFrameBuilder::ConditionalRun()
{
  namespace STF = SubTimeFrame;
  namespace TF  = TimeFrame; 

  // receive
  FairMQParts inParts;
  if (Receive(inParts, fInputChannelName, 0, 1000) > 0) {
    assert(inParts.Size() >= 2);
    Reporter::AddInputMessageSize(inParts);

    auto stfHeader = reinterpret_cast<STF::Header*>(inParts.At(0)->GetData());
    auto stfId     = stfHeader->timeFrameId;

    if (fDiscarded.find(stfId) == fDiscarded.end()) {
      // accumulate sub time frame with same STF ID
      if (fTFBuffer.find(stfId) == fTFBuffer.end()) {
        fTFBuffer[stfId].reserve(fNumSource);
      }
      fTFBuffer[stfId].emplace_back(STFBuffer {std::move(inParts), std::chrono::steady_clock::now()});
    }
    else {
      // if received ID has been previously discarded.
      LOG(warn) << "Received part from an already discarded timeframe with id " << stfId;
    }
  }

  // send
  if (!fTFBuffer.empty()) {
    // find time frame in ready
    for (auto itr = fTFBuffer.begin(); itr!=fTFBuffer.end();) {
      auto stfId  = itr->first;
      auto& tfBuf = itr->second;
      if (tfBuf.size() == fNumSource) {
        // move ownership to complete time frame
        FairMQParts outParts;
	auto h = std::make_unique<TF::Header>();
        h->magic       = TF::Magic;
        h->timeFrameId = stfId;
        h->numSource   = fNumSource;
        h->length      = std::accumulate(tfBuf.begin(), tfBuf.end(), sizeof(TF::Header), 
					 [](auto init, auto& stfBuf) {
					   return init + std::accumulate(stfBuf.parts.begin(), stfBuf.parts.end(), 0, 
                                                                         [] (auto jinit, auto& m) { return (!m) ? jinit : jinit + m->GetSize(); });
					 });
        // LOG(debug) << " length = " << h->length;
	outParts.AddPart(MessageUtil::NewMessage(*this, std::move(h)));
        for (auto& stfBuf: tfBuf) {
          for (auto& m: stfBuf.parts) {
            outParts.AddPart(std::move(m));
          }
        }
        tfBuf.clear();


	// { // for debug
	//   LOG(debug) << " send message parts size = " << outParts.Size() << std::endl;
	//   for (int i=0; i<outParts.Size(); ++ i) {
	//     const auto& msg = outParts.At(i);
	//     LOG(debug) << "msg[" << i << "] length = " << msg->GetSize() << " bytes" << std::endl;
        //     if (i==0) {
        //       std::for_each(reinterpret_cast<uint64_t*>(msg->GetData()), 
        //                     reinterpret_cast<uint64_t*>(msg->GetData()+msg->GetSize()),
        //                     HexDump{4});
        //     }
        //   }
        // }

        Reporter::AddOutputMessageSize(outParts);

        auto nSock = fChannels.at(fOutputChannelName).size();
        for (auto isock=nSock-1; isock>0; --isock) {
          FairMQParts partsCopy;
          for (int k = 0, n = outParts.Size(); k < n; ++k) {
            FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
            msgCopy->Copy(outParts.AtRef(k));
            partsCopy.AddPart(std::move(msgCopy));
          }
          Send(partsCopy, fOutputChannelName, isock);
        }

        while ( Send(outParts, fOutputChannelName) < 0) {
          // timeout
          //if (!CheckCurrentState(RUNNING)) {
          if (GetCurrentState() != fair::mq::State::Running) {
            LOG(info) << "Device is not RUNNING";
            return true;
          }
          LOG(error) << "Failed to queue time frame : TF = " << h->timeFrameId;
        }
      } else {
        // discard incomplete time frame
        auto dt = std::chrono::steady_clock::now() - tfBuf.front().start;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() > fBufferTimeoutInMs) {
          LOG(warn) << "Timeframe #" << stfId << " incomplete after " << fBufferTimeoutInMs << " milliseconds, discarding";
          fDiscarded.insert(stfId);
          tfBuf.clear();
          LOG(warn) << "Number of discarded timeframes: " << fDiscarded.size();
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
void
highp::e50::
TimeFrameBuilder::Init()
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void
highp::e50::
TimeFrameBuilder::InitTask()
{
  using opt = OptionKey;
  fNumSource         = fConfig->GetValue<int>(opt::NumSource.data());
  fBufferTimeoutInMs = fConfig->GetValue<int>(opt::BufferTimeoutInMs.data());
  fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
  fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());

  LOG(debug) << " input channel : name = " << fInputChannelName 
	     << " num = " << fChannels.at(fInputChannelName).size();
  LOG(debug) << " number of source = " << fNumSource;

  Reporter::Reset();
}

//______________________________________________________________________________
void
highp::e50::
TimeFrameBuilder::PostRun()
{
  fTFBuffer.clear();
  fDiscarded.clear();
}
