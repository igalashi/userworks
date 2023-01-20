#include <iostream>

#include "utility/RegisterAccess.h"
#include "utility/RegisterMapParser.h"

using namespace std::string_literals;
namespace e16 {
using am = AccessMode;
using aw = AccessWidth;

const std::unordered_map<am, std::string> accessModeItoS{{am::ReadOnly, AccessRead.data()},
                                                         {am::WriteOnly, AccessWrite.data()},
                                                         {am::ReadWrite, AccessReadWrite.data()},
                                                         {am::Undefined, "Undefined"}};

const std::unordered_map<std::string, am> accessModeStoI{{AccessRead.data(), am::ReadOnly},
                                                         {AccessWrite.data(), am::WriteOnly},
                                                         {AccessReadWrite.data(), am::ReadWrite},
                                                         {"Undefined", am::Undefined}};

const std::unordered_map<aw, std::string> accessWidthItoS{
   {aw::D8, WidthD8.data()},   {aw::D16, WidthD16.data()}, {aw::D24, WidthD24.data()},
   {aw::D32, WidthD32.data()}, {aw::D40, WidthD40.data()}, {aw::D48, WidthD48.data()},
   {aw::D56, WidthD56.data()}, {aw::D64, WidthD64.data()}, {aw::Undefined, "Undefined"}};

const std::unordered_map<std::string, aw> accessWidthStoI{
   {WidthD8.data(), aw::D8},   {WidthD16.data(), aw::D16}, {WidthD24.data(), aw::D24},
   {WidthD32.data(), aw::D32}, {WidthD40.data(), aw::D40}, {WidthD48.data(), aw::D48},
   {WidthD56.data(), aw::D56}, {WidthD64.data(), aw::D64}, {"Undefined", aw::Undefined}};

//______________________________________________________________________________
const std::string RegisterMapParser::accessMode(int v)
{
   return accessModeItoS.at(static_cast<am>(v));
}

//______________________________________________________________________________
int RegisterMapParser::accessMode(std::string_view v)
{
   return static_cast<int>(accessModeStoI.at(v.data()));
}

//______________________________________________________________________________
const std::string RegisterMapParser::accessWidth(int v)
{
   return accessWidthItoS.at(static_cast<aw>(v));
}

//______________________________________________________________________________
int RegisterMapParser::accessWidth(std::string_view v)
{
   return static_cast<int>(accessWidthStoI.at(v.data()));
}

//______________________________________________________________________________
const std::string RegisterMapParser::keyBaseAddress(std::string_view app)
{
   return "app="s + app.data() + ":base_address"s;
}

//______________________________________________________________________________
const std::string RegisterMapParser::keyConst(std::string_view app, std::string_view group, std::string_view name)
{
   return "app="s + app.data() + ":const="s + group.data() + ":"s + name.data();
}

//______________________________________________________________________________
const std::string RegisterMapParser::keyPort(std::string_view app)
{
   return "app="s + app.data() + ":port"s;
}

//______________________________________________________________________________
const std::string RegisterMapParser::keyReg(std::string_view app, std::string_view reg)
{
   return "app="s + app.data() + ":reg="s + reg.data();
}

//______________________________________________________________________________
const std::string RegisterMapParser::keyRegAccessMode(std::string_view app, std::string_view reg)
{
   return "app="s + app.data() + ":reg="s + reg.data() + ":access:mode";
}

//______________________________________________________________________________
const std::string RegisterMapParser::keyRegAccessWidth(std::string_view app, std::string_view reg)
{
   return "app="s + app.data() + ":reg="s + reg.data() + ":access:width";
}

//______________________________________________________________________________
const std::string RegisterMapParser::keyRegAddrFirst(std::string_view app, std::string_view reg)
{
   return "app="s + app.data() + ":reg="s + reg.data() + ":address:first";
}

//______________________________________________________________________________
const std::string RegisterMapParser::keyRegAddrLast(std::string_view app, std::string_view reg)
{
   return "app="s + app.data() + ":reg="s + reg.data() + ":address:Last";
}

//______________________________________________________________________________
void RegisterMapParser::Parse(const YAML::Node &node)
{
   if (node.IsMap()) {
      // std::cout << fKey << " " << node.Tag() << " " << node.Scalar() << " map. size = " << node.size() << std::endl;
      for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
         fKey = it->first.as<std::string>();
         Parse(it->second);
      }
   } else if (node.IsSequence()) {
      // std::cout << fKey << " " << "-------------------------------------------"
      //            << node.Tag() << " " << node.Scalar() << " sequence. size = " << node.size() << std::endl;
      if (fKey == "applications") {
         for (const auto &n : node) {
            ParseApp(node);
         }
      }
      for (const auto &n : node) {
         Parse(n);
      }
   } else if (node.IsScalar()) {
      // std::cout << fKey << ": " << node.Tag() << " " << node.Scalar() << " scalar, size = " << node.size() <<
      // std::endl;

      if (fKey == "module-type") {
         fMapName = node.as<std::string>();
      } else if (fKey == "word_size") {
         fWordSize = node.as<int>();
      }

   } else if (node.IsNull()) {
      // std::cout << "register map Parse() node is null " << std::endl;
   } else if (!node.IsDefined()) {
      std::cout << "register map Parse() node is undefined " << std::endl;
   }
}

//______________________________________________________________________________
void RegisterMapParser::ParseConstant(const YAML::Node &node)
{
   if (node.IsMap()) {
      // std::cout << fKey << " " << node.Tag() << " " << node.Scalar() << " map. size = " << node.size() << std::endl;
      for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
         fKey = it->first.as<std::string>();
         ParseConstant(it->second);
      }
   } else if (node.IsSequence()) {
      // std::cout << fKey << " " << node.Tag() << " " << node.Scalar() << " sequence. size = " << node.size() <<
      // std::endl;
      const auto k = fKey;
      fConstGroup = fKey;
      for (const auto &n : node) {
         ParseConstant(n);
         // std::cout << fConstKV.first << " " << fConstKV.second << std::endl;
         fConstants.emplace(keyConst(fAppName, fConstGroup, fConstKV.first), fConstKV.second);
      }
   } else if (node.IsScalar()) {
      // std::cout << fKey << " " << node.Tag() << " " << node.Scalar() << " scalar, size = " << node.size() <<
      // std::endl;
      if (fKey == "name") {
         fConstKV.first = node.as<std::string>();
      } else if (fKey == "value") {
         fConstKV.second = node.as<std::string>();
      }

   } else if (node.IsNull()) {
      // std::cout << "register map ParseConstant() node is null " << std::endl;
   } else if (!node.IsDefined()) {
      std::cout << "register map ParseConstant()  node is undefined " << std::endl;
   }
}

//______________________________________________________________________________
void RegisterMapParser::ParseRegister(const YAML::Node &node)
{
   if (node.IsMap()) {
      // std::cout << " " << node.Tag() << " " << node.Scalar() << " map. size = " << node.size() << std::endl;
      for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
         fKey = it->first.as<std::string>();
         ParseRegister(it->second);
      }
   } else if (node.IsSequence()) {
      // std::cout << node.Tag() << " " << node.Scalar() << " sequence. size = " << node.size() << std::endl;
      if (fKey == "access") {
         std::vector<std::string> v;
         for (const auto &n : node) {
            v.push_back(n.as<std::string>());
         }
         if (v.empty() || v.size() > 2) {
            throw std::runtime_error("invalid register access parameter");
         }

         fRegisters.emplace(keyRegAccessMode(fAppName, fRegName), v[0]);
         if (v.size() == 2) {
            fRegisters.emplace(keyRegAccessWidth(fAppName, fRegName), v[1]);
         } else {
            fRegisters.emplace(keyRegAccessWidth(fAppName, fRegName), WidthD8.data());
         }
      } else if (fKey == "address") {
         std::vector<std::string> v;
         for (const auto &n : node) {
            v.push_back(n.as<std::string>());
         }
         if (v.empty() || v.size() > 2) {
            throw std::runtime_error("invalid register address parameter");
         }
         if (v.size() == 2) {
            fRegisters.emplace(keyRegAddrFirst(fAppName, fRegName), v[0]);
            fRegisters.emplace(keyRegAddrLast(fAppName, fRegName), v[1]);

         } else if (v.size() == 1) {
            fRegisters.emplace(keyRegAddrFirst(fAppName, fRegName), v[0]);
            fRegisters.emplace(keyRegAddrLast(fAppName, fRegName), v[0]);
         }
      } else {
         for (const auto &n : node) {
            ParseRegister(n);
         }
      }
   } else if (node.IsScalar()) {
      // std::cout << node.Tag() << " " << node.Scalar() << " scalar, size = " << node.size() << std::endl;
      if (fKey == "name") {
         fRegName = node.as<std::string>();
         // std::cout << " register name = " << fRegister->fName << std::endl;
      }

   } else if (node.IsNull()) {
      std::cout << "register map ParseRegister() node is null " << std::endl;
   } else if (!node.IsDefined()) {
      std::cout << "register map ParseRegister() node is undefined " << std::endl;
   }
}

//______________________________________________________________________________
void RegisterMapParser::ParseApp(const YAML::Node &node)
{
   if (node.IsMap()) {
      // std::cout << fKey << " " << node.Tag() << " " << node.Scalar() << " map. size = " << node.size() << std::endl;
      if (fKey == "constants") {
         // std::cout << " parse constants " << std::endl;
         for (auto it = node.begin(); it != node.end(); ++it) {
            fKey = it->first.as<std::string>();
            ParseConstant(it->second);
         }
      } else {
         for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
            fKey = it->first.as<std::string>();
            ParseApp(it->second);
         }
      }
   } else if (node.IsSequence()) {
      // std::cout << fKey << " " << "-------------------------------------------"
      //            << node.Tag() << " " << node.Scalar() << " sequence. size = " << node.size() << std::endl;
      if (fKey == "registers") {
         // std::cout << " parse registers " << std::endl;
         ParseRegister(node);
         // std::cout << " parse registers done" << std::endl;
      } else {
         for (const auto &n : node) {
            ParseApp(n);
         }
      }
   } else if (node.IsScalar()) {
      // std::cout << fKey << ": " << node.Tag() << " " << node.Scalar() << " scalar, size = " << node.size() <<
      // std::endl;
      if (fKey == "name") {
         fAppName = node.as<std::string>();
         // std::cout << " appname = " << fAppName << std::endl;
      } else if (fKey == "base_address") {
         auto addr = node.as<std::string>();
         fRegisters.emplace(keyBaseAddress(fAppName), addr);
      } else if (fKey == "port") {
         auto addr = node.as<std::string>();
         fRegisters.emplace(keyPort(fAppName), addr);
      }

   } else if (node.IsNull()) {
      std::cout << "register map ParseApp() node is null " << std::endl;
   } else if (!node.IsDefined()) {
      std::cout << "register map ParseApp() node is undefined " << std::endl;
   }
}

} // namespace e16
