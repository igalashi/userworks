#ifndef Reporter_h
#define Reporter_h

#include <string>
#include <cstdint>

//class FairMQMessage;
//class FairMQParts;
//class FairMQProgOptions;
//class fair::mq::Message;
//class fair::mq::Parts;
//class fair::mq::ProgOptions;

#include "FairMQMessage.h"
#include "Parts.h"
#include "ProgOptions.h"

namespace highp::e50 {

class Reporter {
public:
  Reporter() = default;
  Reporter(const Reporter&)            = delete;
  Reporter& operator=(const Reporter&) = delete;
  ~Reporter() = default;

  static uint64_t AddInputMessageSize(uint64_t n);
  static uint64_t AddInputMessageSize(const FairMQParts& parts);
  static uint64_t AddOutputMessageSize(uint64_t n);
  static uint64_t AddOutputMessageSize(const FairMQParts& parts);
  static Reporter& Instance(FairMQProgOptions* config=nullptr);
  static void Reset();
  static void Set(const std::string& content);

private:
  FairMQProgOptions* fConfig {nullptr};
  uint64_t fNumIn {0};
  uint64_t fNumOut{0};
  uint64_t fMsgIn {0};
  uint64_t fMsgOut{0};
};

} // namespace highp::e50

#endif 
