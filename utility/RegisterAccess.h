#ifndef E16_UTILITY_REGISTER_ACCESS_H_
#define E16_UTILITY_REGISTER_ACCESS_H_

namespace e16 {
enum class AccessMode : int { ReadOnly, WriteOnly, ReadWrite, Undefined };

static constexpr std::string_view AccessRead{"R"};
static constexpr std::string_view AccessWrite{"W"};
static constexpr std::string_view AccessReadWrite{"RW"};
static constexpr std::string_view AccessWriteRead{"WR"};

enum class AccessWidth : int { D8, D16, D24, D32, D40, D48, D56, D64, Undefined };

static constexpr std::string_view WidthD8{"D8"};
static constexpr std::string_view WidthD16{"D16"};
static constexpr std::string_view WidthD24{"D24"};
static constexpr std::string_view WidthD32{"D32"};
static constexpr std::string_view WidthD40{"D40"};
static constexpr std::string_view WidthD48{"D48"};
static constexpr std::string_view WidthD56{"D56"};
static constexpr std::string_view WidthD64{"D64"};
} // namespace e16

#endif
