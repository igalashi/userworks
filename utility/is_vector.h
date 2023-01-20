#ifndef E16_UTILITY_IS_VECTOR_H_
#define E16_UTILITY_IS_VECTOR_H_

#include <type_traits>
#include <vector>

namespace e16 {
template <typename T>
static constexpr bool false_v = false;

template <typename T>
struct is_vector : std::false_type {
};

template <typename T>
struct is_vector<std::vector<T>> : std::true_type {
};

template <typename T>
static constexpr bool is_vector_v = is_vector<T>::value;
} // namespace e16

#endif // E16_UTILITY_IS_VECTOR_H_
