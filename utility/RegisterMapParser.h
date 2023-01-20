#ifndef E16_UTILITY_REGISTER_MAP_PARSER_H_
#define E16_UTILITY_REGISTER_MAP_PARSER_H_

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <yaml-cpp/yaml.h>

namespace e16 {
struct RegisterMapParser {

   RegisterMapParser(std::unordered_map<std::string, std::string> &m, std::unordered_map<std::string, std::string> &c)
      : fRegisters(m), fConstants(c)
   {
   }
   ~RegisterMapParser() = default;

   static const std::string accessMode(int v);
   static int accessMode(std::string_view v);
   static const std::string accessWidth(int v);
   static int accessWidth(std::string_view v);

   static const std::string keyBaseAddress(std::string_view app);
   static const std::string keyConst(std::string_view app, std::string_view group, std::string_view name);
   static const std::string keyPort(std::string_view app);
   static const std::string keyReg(std::string_view app, std::string_view reg);
   static const std::string keyRegAccessMode(std::string_view app, std::string_view reg);
   static const std::string keyRegAccessWidth(std::string_view app, std::string_view reg);
   static const std::string keyRegAddrFirst(std::string_view app, std::string_view reg);
   static const std::string keyRegAddrLast(std::string_view app, std::string_view reg);

   void Parse(const YAML::Node &node);
   void ParseConstant(const YAML::Node &node);
   void ParseRegister(const YAML::Node &node);
   void ParseApp(const YAML::Node &node);

   std::string fMapName{};
   std::size_t fWordSize{1};
   std::string fName{};
   std::string fKey{};
   std::pair<std::string, std::string> fConstKV{};

   std::string fAppName;
   std::string fRegName;
   std::string fConstGroup;
   std::string fConstName;
   std::unordered_map<std::string, std::string> &fRegisters;
   std::unordered_map<std::string, std::string> &fConstants;
};

} // namespace e16

#endif
