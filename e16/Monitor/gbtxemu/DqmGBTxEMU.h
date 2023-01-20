#ifndef E16DAQ_GBTxEMU_DQM_h
#define E16DAQ_GBTxEMU_DQM_h

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "mq/Monitor/Dqm.h"

namespace boost::program_options {
class options_description;
class variables_map;
} // namespace boost::program_options

class TH1;
class TH2;
class THttpServer;

namespace gbtxemu {

class Unpacker;

class Dqm {
public:
   const std::string fClassName;
   struct OptionKey {
      static constexpr std::string_view HttpEngine{"http-engine"};
      static constexpr std::string_view UpdateInterval{"update-interval"};
      static constexpr std::string_view RunNumber{"run_number"};
      static constexpr std::string_view RunNumberFormat{"run-number-format"};
      static constexpr std::string_view Prefix{"prefix"};
      static constexpr std::string_view UpLinkMapFile{"uplink-map"};
      static constexpr std::string_view TrgUpLink{"trg-uplink"};
      static constexpr std::string_view TrgChannel{"trg-channel"};
   };

   Dqm() = default;
   Dqm(const Dqm &) = delete;
   Dqm &operator=(const Dqm &) = delete;
   ~Dqm() = default;

   static void AddOptions(boost::program_options::options_description &options);

   void FillHit(const Hit &hit);
   void Init(const boost::program_options::variables_map &vm, int run_numer = -1);
   void InitHist();
   void MakeCanvas();
   void SavePDF(const TString &filename);
   void SaveROOTFile(const TString &filename);
   void Update(bool forceUpdate = false);

   std::chrono::steady_clock::time_point fPrevUpdateTimePoint;
   int fUpdateIntervalMS;
   bool fInitialized;
   std::unique_ptr<THttpServer> fHttpServer;
   int fRunNumber{0};
   std::string fRunNumberFormat;
   std::string fPrefix;
   // int fFebType;

   std::vector<char> fBuffer;
   std::unique_ptr<Unpacker> fUnpacker;
   std::unordered_map<int, int> fUpLinkMap;
   int fTrgUpLink;
   int fTrgChannel;
   int fMinChipId{0xFFFF};
   int fMaxChipId{-0xFFFF};

   TH1 *fH1Ch;
   TH1 *fH1Adc;
   TH1 *fH1Tdc;
   TH2 *fH2Adc;
};

} // namespace gbtxemu

#endif