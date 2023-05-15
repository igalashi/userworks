#ifndef STFFilePlayer_h
#define STFFilePlayer_h

#include <string>
#include <string_view>
#include <cstdint>
#include <deque>
#include <memory>
#include <queue>

#include <fairmq/Device.h>

#include "AmQStrTdcData.h"

struct FEMInfo {
    uint64_t magic    {0};
    uint32_t FEMType  {0};
    uint32_t FEMId    {0};
    uint64_t reserved {0};
};

enum class TimeFrameIdType : int {
    FirstHeartbeatDelimiter = 0,
    LastHeartbeatDelimiter,
    SequenceNumberOfTimeFrames
};

class STFBFilePlayer : public fair::mq::Device
{
public:
    using WorkBuffer = std::unique_ptr<std::vector<FairMQMessagePtr>>;
    using RecvBuffer = std::vector<AmQStrTdc::Data::Word>; //64 bits
    using SendBuffer = std::queue<WorkBuffer>;

    struct OptionKey {
        static constexpr std::string_view InputFileName     {"in-file"};
        static constexpr std::string_view FEMId             {"fem-id"};
        static constexpr std::string_view InputChannelName  {"in-chan-name"};
        static constexpr std::string_view OutputChannelName {"out-chan-name"};
        static constexpr std::string_view DQMChannelName    {"dqm-chan-name"};
        static constexpr std::string_view StripHBF          {"strip-hbf"};
        static constexpr std::string_view MaxHBF            {"max-hbf"};
        static constexpr std::string_view SplitMethod       {"split"};
        static constexpr std::string_view TimeFrameIdType   {"time-frame-id-type"};
        static constexpr std::string_view MaxIterations     {"max-iterations"};
        static constexpr std::string_view PollTimeout       {"poll-timeout"};
    };

    STFBFilePlayer();
    STFBFilePlayer(const STFBFilePlayer&)            = delete;
    STFBFilePlayer& operator=(const STFBFilePlayer&) = delete;
    ~STFBFilePlayer() = default;

private:
    void BuildFrame(FairMQMessagePtr& msg, int index);
    void FillData(AmQStrTdc::Data::Word* first,
                  AmQStrTdc::Data::Word* last,
                  bool isSpillEnd);
    void FinalizeSTF();
    void NewData();

    bool ConditionalRun() override;
    bool HandleData(FairMQMessagePtr&, int index);
    void InitTask() override;
    void PostRun() override;
    void PreRun() override;

    uint64_t fFEMId   {0};
    uint64_t fFEMType {0};
    std::string fInputChannelName;
    std::string fOutputChannelName;
    std::string fDQMChannelName;

    //std::string fMaxIterations;
    //std::string fPollTimeoutMS;

    std::string fInputFileName;
    std::ifstream fInputFile;

    int fMaxHBF {1};
    int fHBFCounter {0};
    uint32_t fSTFSequenceNumber {0};
    int fSplitMethod {0};
    uint8_t fLastHeader {0};

    // int fH_flag {0};
    TimeFrameIdType fTimeFrameIdType;
    int32_t fSTFId{-1}; // 8-bit spill counter and 16-bit HB frame from heartbeat delimiter

    int64_t fNumIteration   {0};
    int64_t fMaxIterations  {0};
    int     fDirection      {0};
    int     fNumDestination {0};
    int     fPollTimeoutMS  {0};

    bool mdebug;
    RecvBuffer fInputPayloads;
    WorkBuffer fWorkingPayloads;
    SendBuffer fOutputPayloads;
};

#endif
