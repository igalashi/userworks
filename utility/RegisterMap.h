#ifndef E16_UTILITY_REGISTER_MAP_H_
#define E16_UTILITY_REGISTER_MAP_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace e16 {
class RegisterMap {
public:
   using Map = std::unordered_map<std::string, std::string>;

   RegisterMap() = default;
   RegisterMap(const RegisterMap &) = delete;
   RegisterMap &operator=(const RegisterMap &) = delete;
   virtual ~RegisterMap() = default;

   uint32_t GetAddress(std::string_view app, std::string_view reg, int index = 0) const;
   uint32_t GetBaseAddress(std::string_view app) const;
   const std::string GetConst(std::string_view app, std::string_view ns, std::string_view name) const;
   const Map &GetConstants() const { return fConstants; }
   const std::string GetName() const { return fName; }
   const std::string GetPort(std::string_view app) const;
   Map &GetRegisters() { return fRegisters; }
   uint32_t GetSize(std::string_view app, std::string_view reg) const;
   uint32_t GetWidth(std::string_view app, std::string_view reg) const;
   void Parse(std::string_view filename);
   void Print(std::string_view sortOpt = "") const;
   void SetName(std::string_view name) { fName = name; }
   void SetWordSize(std::size_t n) { fWordSize = n; }

protected:
   std::string fName{};
   Map fRegisters{};
   // example:
   // {"app=app_name:base_address",           "value"}
   // {"app=app_name:reg=reg_name:address:first", "value"}
   // {"app=app_name:reg=reg_name:address:last",  "value"}
   // {"app=app_name:reg=reg_name:access:mode",   "value"}
   // {"app=app_name:reg=reg_name:access:width",  "value"}

   Map fConstants{};
   // example:
   // {"const_group:name", "value"}

   std::size_t fWordSize{1}; // number of bytes per unit word
};
} // namespace e16

#endif
