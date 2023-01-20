#ifndef E16_UTILITY_OPTIONS_H_
#define E16_UTILITY_OPTIONS_H_

#include <iostream>
#include <algorithm>
#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <type_traits>
#include <vector>

#include "utility/is_vector.h"
#include "utility/DebugFlags.h"

namespace e16 {

class Options {
public:
   using List_t = std::unordered_map<std::string, std::any>; // key = record name
   using Apps_t = std::unordered_map<std::string, List_t>;   // key = app name
   using Map_t = std::unordered_map<std::string, Apps_t>;    // key = id (ip address)

   struct AppRecord {
      std::string fName;
      List_t fList;

      int Count(std::string_view key) const { return fList.count(key.data()); }
      template <typename T>
      const T Get(std::string_view key) const;
      void Print() const;
   };

   Options(std::size_t debug = 0);
   Options(const e16::DebugFlags &debug);
   Options(const Options &) = delete;
   Options &operator=(const Options &) = delete;
   ~Options() = default;

   int Count(std::string_view id, std::string_view app) const
   {
      return (fTable.count(id.data()) <= 0) ? 0 : fTable.at(id.data()).count(app.data());
   }
   int Count(std::string_view id, std::string_view app, std::string_view key) const
   {
      return (fTable.count(id.data()) <= 0)                  ? 0
             : (fTable.at(id.data()).count(app.data()) <= 0) ? 0
                                                             : fTable.at(id.data()).at(app.data()).count(key.data());
   }
   Map_t &Get() { return fTable; }
   const Map_t &Get() const { return fTable; }
   template <typename T>
   const T Get(std::string_view id, std::string_view app, std::string_view key) const;
   const AppRecord GetAppRecord(std::string_view id, std::string_view app) const;

   void Parse(std::string_view file);
   void Parse(const std::vector<std::string> &files);
   static auto ParsePairs(std::string_view arg) -> std::vector<std::pair<std::string, std::string>>;
   void Print() const;

private:
   e16::DebugFlags fDebug;
   Map_t fTable{};
};

} // namespace e16

//______________________________________________________________________________
template <typename T>
const T e16::Options::AppRecord::Get(std::string_view key) const
{
   try {
      const auto &var = fList.at(key.data());
      if constexpr (is_vector_v<T>) {
         // const auto &src = std::get<std::vector<std::string>>(var);
         const auto &src = std::any_cast<std::vector<std::string>>(var);
         using Val = typename T::value_type;
         std::vector<Val> v;
         v.reserve(src.size());
         if constexpr (std::is_same_v<std::string, Val>) {
            // std::cout << " is_string" << std::endl;
            return src;
         } else if constexpr (std::is_same_v<uint32_t, Val>) {
            // std::cout << " is_uint32_t" << std::endl;
            std::transform(src.cbegin(), src.cend(), std::back_inserter(v),
                           [](const std::string &s) { return std::stoul(s, nullptr, 0); });

         } else if constexpr (std::is_same_v<uint64_t, Val>) {
            // std::cout << " is_uint64_t" << std::endl;
            std::transform(src.cbegin(), src.cend(), std::back_inserter(v),
                           [](const std::string &s) { return std::stoull(s, nullptr, 0); });

         } else if constexpr (std::is_same_v<int64_t, Val>) {
            // std::cout << " is_int64_t" << std::endl;
            std::transform(src.cbegin(), src.cend(), std::back_inserter(v),
                           [](const std::string &s) { return std::stoll(s, nullptr, 0); });

         } else if constexpr (std::is_integral_v<Val>) {
            // std::cout << " hoge is_integral" << std::endl;
            std::transform(src.cbegin(), src.cend(), std::back_inserter(v),
                           [](const std::string &s) { return static_cast<Val>(std::stoi(s, nullptr, 0)); });

         } else if constexpr (std::is_same_v<float, Val>) {
            // std::cout << " is_float" << std::endl;
            std::transform(src.cbegin(), src.cend(), std::back_inserter(v),
                           [](const std::string &s) { return std::stof(s); });

         } else if constexpr (std::is_same_v<double, Val>) {
            // std::cout << " is_double" << std::endl;
            std::transform(src.cbegin(), src.cend(), std::back_inserter(v),
                           [](const std::string &s) { return std::stod(s); });

         } else if constexpr (std::is_same_v<long double, Val>) {
            // std::cout << " is_long_double" << std::endl;
            std::transform(src.cbegin(), src.cend(), std::back_inserter(v),
                           [](const std::string &s) { return std::stold(s); });

         } else if constexpr (std::is_floating_point_v<Val>) {
            // std::cout << " is_floating_point" << std::endl;
            std::transform(src.cbegin(), src.cend(), std::back_inserter(v),
                           [](const std::string &s) { return static_cast<Val>(std::stold(s)); });

         } else {
            static_assert(false_v<T>);
         }
         return v;
      } else {
         // const auto &src = std::get<std::string>(var);
         const auto &src = std::any_cast<std::string>(var);
         if constexpr (std::is_same_v<std::string, T>) {
            // std::cout << " is_string" << std::endl;
            return src;

         } else if constexpr (std::is_same_v<uint32_t, T>) {
            // std::cout << " is_uint32_t" << std::endl;
            return std::stoul(src, nullptr, 0);

         } else if constexpr (std::is_same_v<uint64_t, T>) {
            // std::cout << " is_uint64_t" << std::endl;
            return std::stoull(src, nullptr, 0);

         } else if constexpr (std::is_same_v<int64_t, T>) {
            // std::cout << " is_int64_t" << std::endl;
            return std::stoll(src, nullptr, 0);

         } else if constexpr (std::is_integral_v<T>) {
            // std::cout << " is_integral" << std::endl;
            return static_cast<T>(std::stoi(src, nullptr, 0));

         } else if constexpr (std::is_same_v<float, T>) {
            // std::cout << " is_float" << std::endl;
            return std::stof(src);

         } else if constexpr (std::is_same_v<double, T>) {
            // std::cout << " is_double" << std::endl;
            return std::stod(src);

         } else if constexpr (std::is_same_v<long double, T>) {
            // std::cout << " is_long_double" << std::endl;
            return std::stold(src);

         } else if constexpr (std::is_floating_point_v<T>) {
            // std::cout << " is_floating_point" << std::endl;
            return static_cast<T>(std::stold(src));

         } else {
            static_assert(false_v<T>);
         }
      }
   } catch (const std::bad_any_cast &e) {
      std::cerr << "#Exception:  e16::Options::Record::Get() failed: (" << e.what() << ")"
                << " key = " << key << std::endl;
      return T{};
   } catch (const std::exception &e) {
      std::cerr << "#Exception:  e16::Options::Record::Get() failed: " << e.what() << " key = " << key << std::endl;
      return T{};
   } catch (...) {
      std::cerr << "#Exception:  e16::Options::Record::Get() failed: unknown exception"
                << " key = " << key << std::endl;
      return T{};
   }
}

//______________________________________________________________________________
template <typename T>
const T e16::Options::Get(std::string_view id, std::string_view app, std::string_view key) const
{
   try {
      const auto &mVar = fTable.at(id.data()).at(app.data());
      AppRecord rec{app.data(), mVar};
      return rec.Get<T>(key);
   } catch (const std::bad_any_cast &e) {
      std::cerr << "#Exception:  e16::Options::Get() failed: (" << e.what() << ") id = " << id << " app = " << app
                << " key = " << key << std::endl;
      return T{};
   } catch (const std::exception &e) {
      std::cerr << "#Exception:  e16::Options::Get() failed: " << e.what() << " id = " << id << " app = " << app
                << " key = " << key << std::endl;
      return T{};
   } catch (...) {
      std::cerr << "#Exception:  e16::Options::Get() failed: unknown exception"
                << " id = " << id << " app = " << app << " key = " << key << std::endl;
      return T{};
   }
}

#endif // E16_UTILITY_OPTIONS_H_
