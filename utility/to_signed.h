#ifndef E16_UTILITY_TO_SIGNED_H_
#define E16_UTILITY_TO_SIGNED_H_

namespace e16 {

//______________________________________________________________________________
template <typename T, int N, typename U>
constexpr T to_signed(U a)
{
   static_assert(N > 0);
   return ((-(a >> (N - 1))) << (N - 1)) | a;
}

} // namespace e16

#endif
