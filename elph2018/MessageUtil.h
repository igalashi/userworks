#ifndef MessageUtil_h
#define MessageUtil_h

//#include <iostream>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>
#include <numeric>

#include <FairMQDevice.h>

namespace highp::e50::MessageUtil {

  // false value for static assert to invoke compilation error
  template <typename T> static  constexpr bool false_v = false;

  // traits type to check whether T is std::vector or not
  template <typename T> struct is_vector : std::false_type {};
  template <typename T> struct is_vector<std::vector<T>> : std::true_type {};
  template <typename T> constexpr bool is_vector_v = is_vector<T>::value;

//______________________________________________________________________________
  // RAII class to release pointer when destructor is called
  template <class T> 
  struct ReleaseHelper 
  {
    ReleaseHelper(std::unique_ptr<T>&& arg) : fData(std::move(arg)) {}
    ~ReleaseHelper() { fData.release(); }
    std::unique_ptr<T> fData;
  };

//______________________________________________________________________________
template <class T>
FairMQMessagePtr NewMessage(FairMQDevice& dev, 
                            std::unique_ptr<T> arg)
{
  // This function is intended to simplify call of NewMessage() with user data allocated as std::unique_ptr<T>
  // and to clarify ownership transfer of the user data from user to FairMQMessage by requiring the use of move-semantics (std::move()). 
  // Deallocation function (ffn interms of zeromq) is specified as a lambda expression, 
  // in which std::unique_ptr aquires the ownership of the buffer (data or hint) and delete it when the deallocation function returns.
  // 
  // Call of std::unique_ptr::release() on the argument list of FairMQDevice::NewMessage() may delete the buffer before actual transfer of ownership. 
  // Therefore, std::unique_ptr<T>::release() must be called after the execution of FairMQDevice::NewMessage(). 
  // Helper class ReleaseHelper<T> automatically calls release() in its destructor.


  using Ptr = std::unique_ptr<T>;
  // Array of std::unique_ptr is not supported.
  ReleaseHelper<T> h(std::forward<Ptr>(arg));

  if constexpr (std::is_same_v<T, std::string>) {
      // std::cout << " T = std::string type is called" << std::endl;
      return dev.NewMessage(const_cast<char*>(h.fData->data()), 
                            h.fData->length(), 
                            [](void*, void* hint) { Ptr msgDeleter{reinterpret_cast<T*>(hint)}; }, 
                            h.fData.get());
    } 
  else if constexpr (is_vector_v<T>) {
      // T is std::vector
      using Val = typename T::value_type;
      if constexpr (std::is_standard_layout<Val>{}) {
          // T is std::vector of simple type
          //std::cout << " T = vector type is called " << std::endl;
          return dev.NewMessage(reinterpret_cast<uint8_t*>(h.fData->data()), 
                                h.fData->size() * sizeof(Val),
                                [](void* , void* hint) { Ptr msgDeleter{reinterpret_cast<T*>(hint)}; }, 
                                h.fData.get());
        }
      else {
        // std::vector of unsupported type. (compilation error)
        static_assert(false_v<T>);
      }
    }
  else if constexpr (std::is_standard_layout<T>{}) {
      //std::cout << " T = simple type is called " << std::endl;
      return dev.NewMessage(reinterpret_cast<uint8_t*>(h.fData.get()), 
                            sizeof(T), 
                            [](void* data, void* ) { Ptr msgDeleter{reinterpret_cast<T*>(data)}; }, 
                            nullptr);
    }
  else {
    // unsupported type. (compilation error)
    static_assert(false_v<T>);
  }

}

//______________________________________________________________________________
inline uint64_t 
TotalLength(const FairMQParts& parts)
{
  auto & c = const_cast<FairMQParts&>(parts);
  return std::accumulate(c.begin(), c.end(), static_cast<uint64_t>(0), 
                         [](uint64_t init, auto& m) -> uint64_t {
                           return (!m) ? init : init + m->GetSize();
                         });
}

} // namespace highp::e50::MessageUtil


#endif
