#ifndef E16_UTILITY_TO_STRING_H_
#define E16_UTILITY_TO_STRING_H_

#include <type_traits>
#include <sstream>
#include <string>

namespace e16 {
template <typename T>
std::string ToString(const T &t, std::ios_base &(*f)(std::ios_base &) = std::dec)
{
   std::ostringstream oss;
   if constexpr (std::is_same_v<T, uint8_t>) {
      oss << f << static_cast<uint16_t>(t);
   } else if constexpr (std::is_same_v<T, int8_t>) {
      oss << f << static_cast<int16_t>(t);
   } else {
      oss << f << t;
   }
   return oss.str();
}
} // namespace e16

#endif // E16_UTILITY_TO_STRING_H_
