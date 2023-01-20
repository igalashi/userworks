#ifndef E16_UTILITY_FILE_SYSTEM_H_
#define E16_UTILITY_FILE_SYSTEM_H_

#if __has_include(<filesystem>)
#include <filesystem>
namespace stdfs = std::filesystem;
#else
#include <experimental/filesystem>
namespace stdfs = std::experimental::filesystem;
#endif

#include <string>
#include <string_view>

namespace e16 {
inline std::string filename(std::string_view s)
{
   return stdfs::path(s.data()).filename().string();
}

inline std::string stem(std::string_view s)
{
   return stdfs::path(s.data()).stem().string();
}
} // namespace e16

#endif // E16_UTILITY_FILE_SYSTEM_H_
