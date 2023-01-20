#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>

#include "utility/RegisterMapParser.h"
#include "utility/RegisterMap.h"

using namespace std::string_literals;

namespace e16 {
using impl = RegisterMapParser;

//______________________________________________________________________________
uint32_t RegisterMap::GetAddress(std::string_view app, std::string_view reg, int index) const
{
   try {
      auto addrFirst = std::stoul(fRegisters.at(impl::keyRegAddrFirst(app, reg)), nullptr, 0);
      auto unit_distance = impl::accessWidth(fRegisters.at(impl::keyRegAccessWidth(app, reg))) / fWordSize + 1;
      return GetBaseAddress(app) + addrFirst + unit_distance * index;
   } catch (const std::exception &e) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " " << e.what()
                << ": app = " << app << ", "
                << " reg = " << reg << ", index = " << index << std::endl;
   } catch (...) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " unknown"
                << ": app = " << app << ", "
                << " reg = " << reg << ", index = " << index << std::endl;
   }
   return 0;
}

//______________________________________________________________________________
uint32_t RegisterMap::GetBaseAddress(std::string_view app) const
{
   const auto &key = impl::keyBaseAddress(app);
   if (fRegisters.count(key) > 0) {
      try {
         return std::stoul(fRegisters.at(key), nullptr, 0);
      } catch (const std::exception &e) {
         std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " " << e.what()
                   << ": app = " << app << ", key = " << key << std::endl;
      } catch (...) {
         std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " unknown"
                   << ": app = " << app << ", key = " << key << std::endl;
      }
   }
   return 0;
}

//______________________________________________________________________________
const std::string RegisterMap::GetConst(std::string_view app, std::string_view ns, std::string_view name) const
{
   try {
      return fConstants.at(impl::keyConst(app, ns, name));
   } catch (const std::exception &e) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " " << e.what()
                << ": app = " << app << ", ns = " << ns << ", name = " << name << std::endl;
   } catch (...) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " unknown"
                << ": app = " << app << ", ns = " << ns << ", name = " << name << std::endl;
   }

   return {""};
}

//______________________________________________________________________________
const std::string RegisterMap::GetPort(std::string_view app) const
{
   try {
      return fRegisters.at(impl::keyPort(app));
   } catch (const std::exception &e) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " " << e.what()
                << ": app = " << app << std::endl;
   } catch (...) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " unknown"
                << ": app = " << app << std::endl;
   }
   return 0;
}

//______________________________________________________________________________
uint32_t RegisterMap::GetSize(std::string_view app, std::string_view reg) const
{
   try {
      auto first = std::stoul(fRegisters.at(impl::keyRegAddrFirst(app, reg)), nullptr, 0);
      auto last = std::stoul(fRegisters.at(impl::keyRegAddrLast(app, reg)), nullptr, 0);
      return last - first + 1;
   } catch (const std::exception &e) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " " << e.what()
                << ": app = " << app << ", reg = " << reg << std::endl;
   } catch (...) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " unknown"
                << ": app = " << app << ", reg = " << reg << std::endl;
   }
   return 0xffffffff;
}

//______________________________________________________________________________
uint32_t RegisterMap::GetWidth(std::string_view app, std::string_view reg) const
{
   try {
      return impl::accessWidth(fRegisters.at(impl::keyRegAccessWidth(app, reg))) + 1;
   } catch (const std::exception &e) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " " << e.what()
                << ": app = " << app << ", reg = " << reg << std::endl;
   } catch (...) {
      std::cout << "Exception: at " << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << " unknown"
                << ": app = " << app << ", reg = " << reg << std::endl;
   }
   return 0xffffffff;
}

//______________________________________________________________________________
void RegisterMap::Parse(std::string_view filename)
{
   e16::RegisterMapParser p(fRegisters, fConstants);
   const auto &top = YAML::LoadFile(filename.data());
   p.Parse(top);
   fName = p.fMapName;
   fWordSize = p.fWordSize;
}

//______________________________________________________________________________
void RegisterMap::Print(std::string_view sortOpt) const
{
   std::cout << "RegisterMap::Print()" << std::endl;
   {
      std::cout << "Registers\n";
      std::map<std::string, std::string> m(fRegisters.cbegin(), fRegisters.cend());
      for (const auto &[k, v] : m) {
         std::cout << std::setw(60) << k << " " << std::setw(10) << v << "\n";
      }
   }

   {
      std::cout << "Constants\n";
      std::map<std::string, std::string> m(fConstants.cbegin(), fConstants.cend());
      for (const auto &[k, v] : m) {
         std::cout << std::setw(60) << k << " " << std::setw(10) << v << "\n";
      }
   }

   std::cout << std::noshowbase;
}

} // namespace e16
