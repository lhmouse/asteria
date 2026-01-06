// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FWD_
#define ROCKET_FWD_

#if !defined ROCKET_DEBUG && (defined _LIBCPP_DEBUG || defined _GLIBCXX_DEBUG)
#  define ROCKET_DEBUG   1
#endif

#if !defined NDEBUG && !defined ROCKET_DEBUG
#  define NDEBUG  1
#endif

#include <type_traits>  // so many...
#include <iterator>  // std::iterator_traits<>, std::begin(), std::end()
#include <utility>  // std::swap()
#include <memory>  // std::allocator<>, std::addressof(), std::unique_ptr, std::shared_ptr
#include <new>  // placement new
#include <atomic>  // std::atomic<>
#include <initializer_list>  // std::initializer_list<>
#include <limits>  // std::numeric_limits<>
#include <tuple>  // std::tuple<>
#include <stdexcept>  // standard exceptions...
#include <chrono>  // std::chrono
#include <cstring>  // std::memset()
#include <cwchar>  // std::wcslen()
#include <cstddef>  // std::size_t, std::ptrdiff_t
#include <cstdint>  // std::u?int(8|16|32|64|ptr)_t
#include <limits.h>  // INT_MAX
#include <string.h>  // stpcpy()
#include <wchar.h>  // wcpcpy()
namespace rocket {
namespace noadl = rocket;

using ::std::nullptr_t;
using ::std::max_align_t;
using ::std::ptrdiff_t;
using ::std::size_t;
using ::std::intptr_t;
using ::std::uintptr_t;
using ::std::intmax_t;
using ::std::uintmax_t;
using ::std::int8_t;
using ::std::uint8_t;
using ::std::int16_t;
using ::std::uint16_t;
using ::std::int32_t;
using ::std::uint32_t;
using ::std::int64_t;
using ::std::uint64_t;
using ::std::initializer_list;
using ::std::numeric_limits;
using ::std::type_info;
using ::std::nothrow_t;
using ::std::integer_sequence;
using ::std::index_sequence;

using ::std::true_type;
using ::std::false_type;
using ::std::integral_constant;
using ::std::enable_if;
using ::std::conditional;
using ::std::common_type;
using ::std::decay;
using ::std::is_constructible;
using ::std::is_assignable;
using ::std::is_nothrow_constructible;
using ::std::is_nothrow_assignable;
using ::std::is_nothrow_default_constructible;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_copy_assignable;
using ::std::is_nothrow_move_constructible;
using ::std::is_nothrow_move_assignable;
using ::std::is_nothrow_destructible;
using ::std::is_convertible;
using ::std::is_copy_constructible;
using ::std::is_copy_assignable;
using ::std::is_move_constructible;
using ::std::is_move_assignable;
using ::std::is_const;
using ::std::add_const;
using ::std::remove_const;
using ::std::is_volatile;
using ::std::add_volatile;
using ::std::remove_volatile;
using ::std::add_cv;
using ::std::remove_cv;
using ::std::is_pointer;
using ::std::add_pointer;
using ::std::remove_pointer;
using ::std::is_reference;
using ::std::is_lvalue_reference;
using ::std::is_rvalue_reference;
using ::std::add_lvalue_reference;
using ::std::add_rvalue_reference;
using ::std::remove_reference;
using ::std::is_same;
using ::std::is_object;
using ::std::is_trivial;
using ::std::is_integral;
using ::std::is_floating_point;
using ::std::is_void;
using ::std::is_trivially_default_constructible;
using ::std::is_trivially_copy_constructible;
using ::std::is_trivially_move_constructible;
using ::std::is_trivially_copy_assignable;
using ::std::is_trivially_move_assignable;
using ::std::is_trivially_destructible;
using ::std::is_trivially_copyable;
using ::std::underlying_type;
using ::std::is_array;
using ::std::is_base_of;
using ::std::is_signed;
using ::std::make_signed;
using ::std::is_unsigned;
using ::std::make_unsigned;
using ::std::is_empty;
using ::std::is_final;
using ::std::is_enum;
using ::std::is_class;
using ::std::is_union;
using ::std::is_scalar;
using ::std::is_arithmetic;
using ::std::is_function;
using ::std::is_invocable;
using ::std::is_invocable_r;
using ::std::is_nothrow_invocable;
using ::std::is_nothrow_invocable_r;

using ::std::allocator;
using ::std::allocator_traits;
using ::std::default_delete;
using ::std::pair;
using ::std::tuple;
using ::std::iterator_traits;

using ::std::exception_ptr;
using ::std::exception;
using ::std::logic_error;
using ::std::domain_error;
using ::std::invalid_argument;
using ::std::length_error;
using ::std::out_of_range;
using ::std::runtime_error;
using ::std::range_error;
using ::std::overflow_error;
using ::std::underflow_error;

using ::std::begin;
using ::std::end;
using ::std::swap;
using ::std::declval;
using ::std::move;
using ::std::forward;
using ::std::forward_as_tuple;

using ::std::memory_order;
using ::std::memory_order_relaxed;
using ::std::memory_order_consume;
using ::std::memory_order_acquire;
using ::std::memory_order_release;
using ::std::memory_order_acq_rel;
using ::std::memory_order_seq_cst;

using namespace ::std::chrono;
using namespace ::std::literals;

// Utility macros
#define ROCKET_CAR(x, ...)   x
#define ROCKET_CDR(x, ...)   __VA_ARGS__
#define ROCKET_CAT2(x,y)     x##y
#define ROCKET_CAT3(x,y,z)   x##y##z

#define ROCKET_STRINGIFY_NX(...)   #__VA_ARGS__
#define ROCKET_STRINGIFY(...)      ROCKET_STRINGIFY_NX(__VA_ARGS__)
#define ROCKET_SOURCE_LOCATION     __FILE__ ":" ROCKET_STRINGIFY(__LINE__)

#if defined __clang__
#  define ROCKET_FORMAT_PRINTF(...)   __attribute__((__format__(__printf__, __VA_ARGS__)))
#else
#  define ROCKET_FORMAT_PRINTF(...)   __attribute__((__format__(__gnu_printf__, __VA_ARGS__)))
#endif

#define ROCKET_SELECTANY         __attribute__((__weak__))
#define ROCKET_SECTION(...)      __attribute__((__section__(__VA_ARGS__)))
#define ROCKET_NEVER_INLINE      __attribute__((__noinline__))
#define ROCKET_PURE              __attribute__((__pure__))
#define ROCKET_CONST             __attribute__((__const__))
#define ROCKET_ARTIFICIAL        __attribute__((__artificial__, __always_inline__))
#define ROCKET_FLATTEN           __attribute__((__flatten__))
#define ROCKET_ALWAYS_INLINE     __attribute__((__always_inline__)) __inline__
#define ROCKET_COLD              __attribute__((__cold__))
#define ROCKET_HOT               __attribute__((__hot__))

#define ROCKET_UNREACHABLE()       __builtin_unreachable()
#define ROCKET_EXPECT(...)         __builtin_expect(!!(__VA_ARGS__), 1)
#define ROCKET_UNEXPECT(...)       __builtin_expect(!!(__VA_ARGS__), 0)
#define ROCKET_CONSTANT_P(...)     __builtin_constant_p(__VA_ARGS__)
#define ROCKET_FUNCSIG             __PRETTY_FUNCTION__

#define ROCKET_UNDEDUCED(...)    typename ::std::enable_if<true, __VA_ARGS__>::type
#define ROCKET_VOID_T(...)       typename ::std::conditional<true, void, __VA_ARGS__>::type
#define ROCKET_ENABLE_IF(...)    typename ::std::enable_if<+bool(__VA_ARGS__)>::type* = nullptr
#define ROCKET_DISABLE_IF(...)   typename ::std::enable_if<!bool(__VA_ARGS__)>::type* = nullptr

#define ROCKET_VOID_DECLTYPE(...)         ROCKET_VOID_T(decltype(__VA_ARGS__))
#define ROCKET_ENABLE_IF_HAS_TYPE(...)    ROCKET_VOID_T(__VA_ARGS__)* = nullptr
#define ROCKET_ENABLE_IF_HAS_VALUE(...)   ROCKET_ENABLE_IF(sizeof(__VA_ARGS__) != 0)
#define ROCKET_DISABLE_SELF(T, ...)       ROCKET_DISABLE_IF(::std::is_base_of<T,  \
                                              typename ::std::remove_reference<__VA_ARGS__>::type  \
                                               >::value)

#define ROCKET_CONSTEXPR_INLINE   constexpr ROCKET_ALWAYS_INLINE  // https://gcc.gnu.org/PR109464
#define ROCKET_SET_IF(x, ...)   ((x) && ((void) (*(x) = (__VA_ARGS__)), true))

#define ROCKET_DEFINE_ENUM_OPERATORS(enumT)  \
  constexpr  \
  enumT  \
  operator&(enumT lhs, enumT rhs)  \
    noexcept  \
    {  \
      return static_cast<enumT>(  \
          static_cast<typename ::std::underlying_type<enumT>::type>(lhs)  \
          & static_cast<typename ::std::underlying_type<enumT>::type>(rhs));  \
    }  \
  \
  constexpr  \
  enumT&  \
  operator&=(enumT& lhs, enumT rhs)  \
    noexcept  \
    {  \
      return lhs = static_cast<enumT>(  \
          static_cast<typename ::std::underlying_type<enumT>::type>(lhs)  \
          & static_cast<typename ::std::underlying_type<enumT>::type>(rhs));  \
    }  \
  \
  constexpr  \
  enumT  \
  operator|(enumT lhs, enumT rhs)  \
    noexcept  \
    {  \
      return static_cast<enumT>(  \
          static_cast<typename ::std::underlying_type<enumT>::type>(lhs)  \
          | static_cast<typename ::std::underlying_type<enumT>::type>(rhs));  \
    }  \
  \
  constexpr  \
  enumT&  \
  operator|=(enumT& lhs, enumT rhs)  \
    noexcept  \
    {  \
      return lhs = static_cast<enumT>(  \
          static_cast<typename ::std::underlying_type<enumT>::type>(lhs)  \
          | static_cast<typename ::std::underlying_type<enumT>::type>(rhs));  \
    }  \
  \
  constexpr  \
  enumT  \
  operator^(enumT lhs, enumT rhs)  \
    noexcept  \
    {  \
      return static_cast<enumT>(  \
          static_cast<typename ::std::underlying_type<enumT>::type>(lhs)  \
          ^ static_cast<typename ::std::underlying_type<enumT>::type>(rhs));  \
    }  \
  \
  constexpr  \
  enumT&  \
  operator^=(enumT& lhs, enumT rhs)  \
    noexcept  \
    {  \
      return lhs = static_cast<enumT>(  \
          static_cast<typename ::std::underlying_type<enumT>::type>(lhs)  \
          ^ static_cast<typename ::std::underlying_type<enumT>::type>(rhs));  \
    }  \
  \
  constexpr  \
  enumT  \
  operator~(enumT rhs)  \
    noexcept  \
    {  \
      return static_cast<enumT>(  \
          ~ static_cast<typename ::std::underlying_type<enumT>::type>(rhs));  \
    }

template<typename typeT>
struct remove_cvref
  : remove_cv<typename remove_reference<typeT>::type>  { };

template<typename typeT>
struct is_nothrow_swappable
  : integral_constant<bool, noexcept(swap(declval<typeT&>(), declval<typeT&>()))>  { };

template<typename... typesT>
struct conjunction
  : true_type  { };

template<typename firstT, typename... restT>
struct conjunction<firstT, restT...>
  : conditional<bool(firstT::value), conjunction<restT...>, firstT>::type  { };

template<typename... typesT>
struct disjunction
  : false_type  { };

template<typename firstT, typename... restT>
struct disjunction<firstT, restT...>
  : conditional<bool(firstT::value), firstT, disjunction<restT...>>::type  { };

template<typename targetT, typename sourceT>
struct copy_const
  : conditional<is_const<sourceT>::value, add_const<targetT>, remove_const<targetT>>::type  { };

template<typename targetT, typename sourceT>
struct copy_volatile
  : conditional<is_volatile<sourceT>::value, add_volatile<targetT>, remove_volatile<targetT>>::type  { };

template<typename targetT, typename sourceT>
struct copy_cv
  : copy_const<typename copy_volatile<targetT, sourceT>::type, sourceT>  { };

template<typename containerT>
constexpr
decltype(declval<const containerT&>().size())
size(const containerT& cont)
  noexcept(noexcept(cont.size()))
  { return cont.size();  }

template<typename elementT, size_t countT>
constexpr
size_t
size(const elementT (&)[countT])
  noexcept
  { return countT;  }

template<typename containerT>
constexpr
decltype(static_cast<ptrdiff_t>(declval<const containerT&>().size()))
ssize(const containerT& cont)
  noexcept(noexcept(static_cast<ptrdiff_t>(cont.size())))
  { return static_cast<ptrdiff_t>(cont.size());  }

template<typename elementT, size_t countT>
constexpr
ptrdiff_t
ssize(const elementT (&)[countT])
  noexcept
  { return static_cast<ptrdiff_t>(countT);  }

template<typename valueT, typename withT>
constexpr
valueT
exchange(valueT& ref, withT&& with)
  {
    valueT old = ::std::move(ref);
    ref = ::std::forward<withT>(with);
    return old;
  }

template<typename valueT, typename... withT>
constexpr
valueT
exchange(valueT& ref, withT&&... with)
  {
    valueT old = ::std::move(ref);
    ref = { ::std::forward<withT>(with)... };
    return old;
  }

#include "details/fwd.ipp"

template<typename typeT>
inline
void
xswap(typeT& lhs, typeT& rhs)  \
  noexcept(noexcept(swap(lhs, rhs)))
  { swap(lhs, rhs);  }

template<typename firstT, typename secondT, typename... restT>
struct select_type
  : select_type<typename select_type<firstT, secondT>::type, restT...>  { };

template<typename firstT, typename secondT>
struct select_type<firstT, secondT>
  : common_type<firstT, secondT>  { };

template<typename iteratorT>
struct is_input_iterator
  : details_fwd::is_input_iterator_aux<iteratorT, void>  { };

template<typename targetT, typename... candidatesT>
struct is_any_type_of
  : false_type  { };

template<typename targetT, typename... restT>
struct is_any_type_of<targetT, targetT, restT...>
  : true_type  { };

template<typename targetT, typename firstT, typename... restT>
struct is_any_type_of<targetT, firstT, restT...>
  : is_any_type_of<targetT, restT...>  { };

template<typename lhsT, typename rhsT>
constexpr
typename select_type<lhsT&&, rhsT&&>::type
min(lhsT&& lhs, rhsT&& rhs)
  {
    return (rhs < lhs) ? forward<rhsT>(rhs) : forward<lhsT>(lhs);
  }

template<typename lhsT, typename rhsT, typename... restT>
constexpr
typename select_type<lhsT&&, rhsT&&, restT&&...>::type
min(lhsT&& lhs, rhsT&& rhs, restT&&... rest)
  {
    return noadl::min(noadl::min(forward<lhsT>(lhs), forward<rhsT>(rhs)), forward<restT>(rest)...);
  }

template<typename lhsT, typename rhsT>
constexpr
typename select_type<lhsT&&, rhsT&&>::type
max(lhsT&& lhs, rhsT&& rhs)
  {
    return (lhs < rhs) ? forward<rhsT>(rhs) : forward<lhsT>(lhs);
  }

template<typename lhsT, typename rhsT, typename... restT>
constexpr
typename select_type<lhsT&&, rhsT&&, restT&&...>::type
max(lhsT&& lhs, rhsT&& rhs, restT&&... rest)
  {
    return noadl::max(noadl::max(forward<lhsT>(lhs), forward<rhsT>(rhs)), forward<restT>(rest)...);
  }

template<typename xvT, typename loT, typename upT>
constexpr
typename select_type<xvT&&, loT&&, upT&&>::type
clamp(xvT&& xv, loT&& lo, upT&& up)
  {
    return (xv < lo) ? forward<loT>(lo)
           : (up < xv) ? forward<upT>(up)
           : forward<xvT>(xv);
  }

template<typename resultT, typename xvT, typename loT, typename upT>
constexpr
resultT
clamp_cast(xvT&& xv, loT&& lo, upT&& up)
  {
    return static_cast<resultT>((xv < lo) ? forward<loT>(lo)
                                : (up < xv) ? forward<upT>(up)
                                : forward<xvT>(xv));
  }

template<typename xvT, typename yvT,
ROCKET_ENABLE_IF(is_integral<xvT>::value && is_integral<yvT>::value)>
constexpr
typename common_type<xvT, yvT>::type
addm(xvT x, yvT y, bool* ovr = nullptr)
  noexcept
  {
    typename common_type<xvT, yvT>::type r = 0;
    bool of = __builtin_add_overflow(x, y, ::std::addressof(r));
    if(ovr)
      *ovr |= of;
    return r;
  }

template<typename xvT, typename yvT,
ROCKET_ENABLE_IF(is_integral<xvT>::value && is_integral<yvT>::value)>
constexpr
typename common_type<xvT, yvT>::type
subm(xvT x, yvT y, bool* ovr = nullptr)
  noexcept
  {
    typename common_type<xvT, yvT>::type r = 0;
    bool of = __builtin_sub_overflow(x, y, ::std::addressof(r));
    if(ovr)
      *ovr |= of;
    return r;
  }

template<typename xvT, typename yvT,
ROCKET_ENABLE_IF(is_integral<xvT>::value && is_integral<yvT>::value)>
constexpr
typename common_type<xvT, yvT>::type
mulm(xvT x, yvT y, bool* ovr = nullptr)
  noexcept
  {
    typename common_type<xvT, yvT>::type r = 0;
    bool of = __builtin_mul_overflow(x, y, ::std::addressof(r));
    if(ovr)
      *ovr |= of;
    return r;
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 2))>
ROCKET_ALWAYS_INLINE
valueT
load_be(const void* ptr)
  noexcept
  {
    uint16_t temp;
    ::memcpy(&temp, ptr, 2);
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    temp = __builtin_bswap16(temp);
#endif
    valueT value;
    ::memcpy(::std::addressof(value), &temp, 2);
    return value;
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 2))>
ROCKET_ALWAYS_INLINE
valueT
load_le(const void* ptr)
  noexcept
  {
    uint16_t temp;
    ::memcpy(&temp, ptr, 2);
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    temp = __builtin_bswap16(temp);
#endif
    valueT value;
    ::memcpy(::std::addressof(value), &temp, 2);
    return value;
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 4))>
ROCKET_ALWAYS_INLINE
valueT
load_be(const void* ptr)
  noexcept
  {
    uint32_t temp;
    ::memcpy(&temp, ptr, 4);
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    temp = __builtin_bswap32(temp);
#endif
    valueT value;
    ::memcpy(::std::addressof(value), &temp, 4);
    return value;
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 4))>
ROCKET_ALWAYS_INLINE
valueT
load_le(const void* ptr)
  noexcept
  {
    uint32_t temp;
    ::memcpy(&temp, ptr, 4);
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    temp = __builtin_bswap32(temp);
#endif
    valueT value;
    ::memcpy(::std::addressof(value), &temp, 4);
    return value;
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 8))>
ROCKET_ALWAYS_INLINE
valueT
load_be(const void* ptr)
  noexcept
  {
    uint64_t temp;
    ::memcpy(&temp, ptr, 8);
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    temp = __builtin_bswap64(temp);
#endif
    valueT value;
    ::memcpy(::std::addressof(value), &temp, 8);
    return value;
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 8))>
ROCKET_ALWAYS_INLINE
valueT
load_le(const void* ptr)
  noexcept
  {
    uint64_t temp;
    ::memcpy(&temp, ptr, 8);
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    temp = __builtin_bswap64(temp);
#endif
    valueT value;
    ::memcpy(::std::addressof(value), &temp, 8);
    return value;
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 2))>
ROCKET_ALWAYS_INLINE
void
store_be(void* ptr, valueT value)
  noexcept
  {
    uint16_t temp;
    ::memcpy(&temp, ::std::addressof(value), 2);
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    temp = __builtin_bswap16(temp);
#endif
    ::memcpy(ptr, &temp, 2);
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 2))>
ROCKET_ALWAYS_INLINE
void
store_le(void* ptr, valueT value)
  noexcept
  {
    uint16_t temp;
    ::memcpy(&temp, ::std::addressof(value), 2);
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    temp = __builtin_bswap16(temp);
#endif
    ::memcpy(ptr, &temp, 2);
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 4))>
ROCKET_ALWAYS_INLINE
void
store_be(void* ptr, valueT value)
  noexcept
  {
    uint32_t temp;
    ::memcpy(&temp, ::std::addressof(value), 4);
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    temp = __builtin_bswap32(temp);
#endif
    ::memcpy(ptr, &temp, 4);
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 4))>
ROCKET_ALWAYS_INLINE
void
store_le(void* ptr, valueT value)
  noexcept
  {
    uint32_t temp;
    ::memcpy(&temp, ::std::addressof(value), 4);
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    temp = __builtin_bswap32(temp);
#endif
    ::memcpy(ptr, &temp, 4);
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 8))>
ROCKET_ALWAYS_INLINE
void
store_be(void* ptr, valueT value)
  noexcept
  {
    uint64_t temp;
    ::memcpy(&temp, ::std::addressof(value), 8);
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    temp = __builtin_bswap64(temp);
#endif
    ::memcpy(ptr, &temp, 8);
  }

template<typename valueT,
ROCKET_ENABLE_IF(is_trivially_copyable<valueT>::value && (sizeof(valueT) == 8))>
ROCKET_ALWAYS_INLINE
void
store_le(void* ptr, valueT value)
  noexcept
  {
    uint64_t temp;
    ::memcpy(&temp, ::std::addressof(value), 8);
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    temp = __builtin_bswap64(temp);
#endif
    ::memcpy(ptr, &temp, 8);
  }

constexpr
uint32_t
lzcnt32(uint32_t x)
  noexcept
  { return (x != 0) ? static_cast<uint32_t>(__builtin_clz(x) & 31) : 32;  }

constexpr
uint32_t
tzcnt32(uint32_t x)
  noexcept
  { return (x != 0) ? static_cast<uint32_t>(__builtin_ctz(x) & 31) : 32;  }

constexpr
uint32_t
popcnt32(uint32_t x)
  noexcept
  { return static_cast<uint32_t>(__builtin_popcount(x));  }

constexpr
uint64_t
lzcnt64(uint64_t x)
  noexcept
  { return (x != 0) ? static_cast<uint64_t>(__builtin_clzll(x) & 63) : 64;  }

constexpr
uint64_t
tzcnt64(uint64_t x)
  noexcept
  { return (x != 0) ? static_cast<uint64_t>(__builtin_ctzll(x) & 63) : 64;  }

constexpr
uint64_t
popcnt64(uint64_t x)
  noexcept
  { return static_cast<uint64_t>(__builtin_popcountll(x));  }

template<typename firstT, typename lastT, typename funcT, typename... paramsT>
void
for_range(firstT first, lastT last, funcT&& func, const paramsT&... params)
  {
    for(auto qit = move(first); qit != last; ++qit)
      forward<funcT>(func)(qit, params...);
  }

template<typename firstT, typename lastT, typename funcT, typename... paramsT>
void
do_while_range(firstT first, lastT last, funcT&& func, const paramsT&... params)
  {
    auto qit = move(first);
    do
      forward<funcT>(func)(qit, params...);
    while(++qit != last);
  }

template<typename iteratorT>
constexpr
size_t
estimate_distance(iteratorT first, iteratorT last)
  {
    return details_fwd::estimate_distance_aux(
             typename iterator_traits<iteratorT>::iterator_category(),
             move(first), move(last));
  }

template<typename... altsT>
struct storage_for
  {
    static constexpr size_t size = noadl::max(1UL, sizeof(altsT)...);
    static constexpr size_t align = noadl::max(1UL, alignof(altsT)...);
    alignas(align) char bytes[size];
  };

template<typename elementT, typename... paramsT>
ROCKET_ALWAYS_INLINE
elementT*
construct(elementT* ptr, paramsT&&... params)
  noexcept(is_nothrow_constructible<elementT, paramsT&&...>::value)
  {
#ifdef ROCKET_DEBUG
    ::std::memset((void*)ptr, 0xAA, sizeof(elementT));
#endif
    return ::new((void*)ptr) elementT(forward<paramsT>(params)...);
  }

template<typename elementT>
ROCKET_ALWAYS_INLINE
elementT*
default_construct(elementT* ptr)
  noexcept(is_nothrow_default_constructible<elementT>::value)
  {
#ifdef ROCKET_DEBUG
    ::std::memset((void*)ptr, 0xBE, sizeof(elementT));
#endif
    return ::new((void*)ptr) elementT;
  }

template<typename elementT>
ROCKET_ALWAYS_INLINE
void
destroy(elementT* ptr)
  noexcept(is_nothrow_destructible<elementT>::value)
  {
    // The C++ standard says the lifetime of a trivial object does not end.
    if(is_trivially_destructible<elementT>::value)
      return;

    ptr->~elementT();
#ifdef ROCKET_DEBUG
    ::std::memset((void*)ptr, 0xD9, sizeof(elementT));
#endif
  }

template<typename elementT, typename... paramsT>
ROCKET_ALWAYS_INLINE
elementT*
reconstruct(elementT* ptr, paramsT&&... params)
  noexcept
  {
    // This has to be noexcept, as there is no way to recover the destroyed
    // object if a new one can't be constructed.
    static_assert(is_nothrow_constructible<elementT, paramsT&&...>::value);
    ptr->~elementT();
#ifdef ROCKET_DEBUG
    ::std::memset((void*)ptr, 0xAA, sizeof(elementT));
#endif
    return ::new((void*)ptr) elementT(forward<paramsT>(params)...);
  }

struct equal
  {
    template<typename lhsT, typename rhsT>
    constexpr
    bool
    operator()(lhsT&& lhs, rhsT&& rhs)
      const
      noexcept(noexcept(::std::declval<lhsT>() == ::std::declval<rhsT>()))
      { return forward<lhsT>(lhs) == forward<rhsT>(rhs);  }
  };

struct unequal
  {
    template<typename lhsT, typename rhsT>
    constexpr
    bool
    operator()(lhsT&& lhs, rhsT&& rhs)
      const
      noexcept(noexcept(::std::declval<lhsT>() != ::std::declval<rhsT>()))
      { return forward<lhsT>(lhs) != forward<rhsT>(rhs);  }
  };

struct less
  {
    template<typename lhsT, typename rhsT>
    constexpr
    bool
    operator()(lhsT&& lhs, rhsT&& rhs)
      const
      noexcept(noexcept(::std::declval<lhsT>() < ::std::declval<rhsT>()))
      { return forward<lhsT>(lhs) < forward<rhsT>(rhs);  }
  };

struct greater
  {
    template<typename lhsT, typename rhsT>
    constexpr
    bool
    operator()(lhsT&& lhs, rhsT&& rhs)
      const
      noexcept(noexcept(::std::declval<lhsT>() > ::std::declval<rhsT>()))
      { return forward<lhsT>(lhs) > forward<rhsT>(rhs);  }
  };

struct less_equal
  {
    template<typename lhsT, typename rhsT>
    constexpr
    bool
    operator()(lhsT&& lhs, rhsT&& rhs)
      const
      noexcept(noexcept(::std::declval<lhsT>() <= ::std::declval<rhsT>()))
      { return forward<lhsT>(lhs) <= forward<rhsT>(rhs);  }
  };

struct greater_equal
  {
    template<typename lhsT, typename rhsT>
    constexpr
    bool
    operator()(lhsT&& lhs, rhsT&& rhs)
      const
      noexcept(noexcept(::std::declval<lhsT>() >= ::std::declval<rhsT>()))
      { return forward<lhsT>(lhs) >= forward<rhsT>(rhs);  }
  };

template<typename elementT>
void
rotate(elementT* ptr, size_t begin, size_t seek, size_t end)
  {
    size_t bot = begin;
    size_t brk = seek;
    size_t stp = seek;

    //     bot     brk         end
    //   > 0 1 2 3 4 5 6 7 8 9 -
    //     ^- isl -^--- isr ---^
    size_t isl = brk - bot;
    if(isl == 0)
      return;

    size_t isr = end - brk;
    if(isr == 0)
      return;

    while(isl != isr)
      if(isl < isr) {
        // Before:
        //           stp           end
        //     bot   brk
        //   > 0 1 2 3 4 5 6 7 8 9 -
        //
        // After:
        //           stp           end
        //           bot   brk
        //   > 3 4 5 0 1 2 6 7 8 9 -
        do
          swap(ptr[bot++], ptr[brk++]);
        while(bot != stp);

        // `isr` has been decreased by `isl`, which will
        // not yield zero. `isl` is unchanged.
        stp = brk;
        isr = end - brk;
      }
      else {
        // Before:
        //                   stp   end
        //     bot           brk
        //   > 0 1 2 3 4 5 6 7 8 9 -
        //
        // After:
        //                   stp   end
        //           bot           brk
        //   > 7 8 9 3 4 5 6 0 1 2 -
        do
          swap(ptr[bot++], ptr[brk++]);
        while(brk != end);

        // `isl` has been decreased by `isr`, which will
        // not yield zero. `isr` is unchanged.
        brk = stp;
        isl = stp - bot;
      }

    // Before:
    //               stp       end
    //     bot       brk
    //   > 0 1 2 3 4 5 6 7 8 9 -
    //
    // After:
    //               stp       end
    //               bot       brk
    //   > 5 6 7 8 9 0 1 2 3 4 -
    do
      swap(ptr[bot++], ptr[brk++]);
    while(bot != stp);
  }

template<typename containerT, typename callbackT>
constexpr
void
for_each(containerT&& cont, callbackT&& call)
  {
    for(auto&& elem : cont)
      call(static_cast<decltype(elem)&&>(elem));
  }

template<typename elementT, typename callbackT>
constexpr
void
for_each(initializer_list<elementT> init, callbackT&& call)
  {
    for(const auto& elem : init)
      call(elem);
  }

template<typename containerT, typename predictorT>
constexpr
bool
any_of(containerT&& cont, predictorT&& pred)
  {
    for(const auto& elem : cont)
      if(pred(elem))
        return true;
    return false;
  }

template<typename elementT, typename predictorT>
constexpr
bool
any_of(initializer_list<elementT> init, predictorT&& pred)
  {
    for(const auto& elem : init)
      if(pred(elem))
        return true;
    return false;
  }

template<typename containerT, typename predictorT>
constexpr
bool
none_of(containerT&& cont, predictorT&& pred)
  {
    for(const auto& elem : cont)
      if(pred(elem))
        return false;
    return true;
  }

template<typename elementT, typename predictorT>
constexpr
bool
none_of(initializer_list<elementT> init, predictorT&& pred)
  {
    for(const auto& elem : init)
      if(pred(elem))
        return false;
    return true;
  }

template<typename containerT, typename predictorT>
constexpr
bool
all_of(containerT&& cont, predictorT&& pred)
  {
    for(const auto& elem : cont)
      if(!(bool) pred(elem))
        return false;
    return true;
  }

template<typename elementT, typename predictorT>
constexpr
bool
all_of(initializer_list<elementT> init, predictorT&& pred)
  {
    for(const auto& elem : init)
      if(!(bool) pred(elem))
        return false;
    return true;
  }

template<typename targetT, typename containerT>
constexpr
bool
is_any_of(const targetT& targ, containerT&& cont)
  {
    for(const auto& elem : cont)
      if(targ == elem)
        return true;
    return false;
  }

template<typename targetT, typename elementT>
constexpr
bool
is_any_of(const targetT& targ, initializer_list<elementT> init)
  {
    for(const auto& elem : init)
      if(targ == elem)
        return true;
    return false;
  }

template<typename targetT, typename containerT>
constexpr
bool
is_none_of(const targetT& targ, containerT&& cont)
  {
    for(const auto& elem : cont)
      if(targ == elem)
        return false;
    return true;
  }

template<typename targetT, typename elementT>
constexpr
bool
is_none_of(const targetT& targ, initializer_list<elementT> init)
  {
    for(const auto& elem : init)
      if(targ == elem)
        return false;
    return true;
  }

template<typename containerT, typename targetT>
constexpr
typename remove_reference<decltype(*(begin(declval<containerT>())))>::type*
find(containerT&& cont, targetT&& targ)
  {
    for(auto& elem : cont)
      if(elem == targ)
        return ::std::addressof(elem);
    return nullptr;
  }

template<typename elementT, typename targetT>
constexpr
const elementT*
find(initializer_list<elementT> init, targetT&& targ)
  {
    for(const auto& elem : init)
      if(elem == targ)
        return ::std::addressof(elem);
    return nullptr;
  }

template<typename containerT, typename predictorT>
constexpr
typename remove_reference<decltype(*(begin(declval<containerT>())))>::type*
find_if(containerT&& cont, predictorT&& pred)
  {
    for(auto& elem : cont)
      if(pred(elem))
        return ::std::addressof(elem);
    return nullptr;
  }

template<typename elementT, typename predictorT>
constexpr
const elementT*
find_if(initializer_list<elementT> init, predictorT&& pred)
  {
    for(const auto& elem : init)
      if(pred(elem))
        return ::std::addressof(elem);
    return nullptr;
  }

template<typename containerT, typename targetT>
constexpr
size_t
count(containerT&& cont, targetT&& targ)
  {
    size_t n = 0;
    for(auto& elem : cont)
      if(elem == targ)
        ++n;
    return n;
  }

template<typename elementT, typename targetT>
constexpr
size_t
count(initializer_list<elementT> init, targetT&& targ)
  {
    size_t n = 0;
    for(const auto& elem : init)
      if(elem == targ)
        ++n;
    return n;
  }

template<typename containerT, typename predictorT>
constexpr
size_t
count_if(containerT&& cont, predictorT&& pred)
  {
    size_t n = 0;
    for(auto& elem : cont)
      if(pred(elem))
        ++n;
    return n;
  }

template<typename elementT, typename predictorT>
constexpr
size_t
count_if(initializer_list<elementT> init, predictorT&& pred)
  {
    size_t n = 0;
    for(const auto& elem : init)
      if(pred(elem))
        ++n;
    return n;
  }

template<typename xvalueT>
constexpr
details_fwd::binder_eq<typename decay<xvalueT>::type>
is(xvalueT&& xval)
  {
    return forward<xvalueT>(xval);
  }

template<typename xvalueT>
constexpr
details_fwd::binder_ne<typename decay<xvalueT>::type>
isnt(xvalueT&& xval)
  {
    return forward<xvalueT>(xval);
  }

template<typename xvalueT>
constexpr
details_fwd::binder_eq<typename decay<xvalueT>::type>
are(xvalueT&& xval)
  {
    return forward<xvalueT>(xval);
  }

template<typename xvalueT>
constexpr
details_fwd::binder_ne<typename decay<xvalueT>::type>
arent(xvalueT&& xval)
  {
    return forward<xvalueT>(xval);
  }

template<typename charT>
ROCKET_PURE constexpr
int
int_from(charT c)
  noexcept
  {
    return static_cast<int>(c);
  }

ROCKET_PURE constexpr
int
int_from(char c)
  noexcept
  {
    return static_cast<unsigned char>(c);
  }

template<intmax_t valueT>
struct lowest_signed
  :
    details_fwd::integer_selector<intmax_t, valueT,
         signed char,
         signed short,
         signed int,
         signed long,
         signed long long>
  {
  };

template<uintmax_t valueT>
struct lowest_unsigned
  :
    details_fwd::integer_selector<uintmax_t, valueT,
         unsigned char,
         unsigned short,
         unsigned int,
         unsigned long,
         unsigned long long>
  {
  };

// Fancy pointer conversion
template<typename pointerT>
constexpr
typename remove_reference<decltype(*(declval<pointerT&>()))>::type*
unfancy(pointerT&& ptr)
  {
    return ptr ? ::std::addressof(*ptr) : nullptr;
  }

template<typename targetT, typename sourceT>
constexpr
targetT
static_or_dynamic_cast(sourceT&& src)
  {
    return details_fwd::static_or_dynamic_cast_aux<targetT, sourceT>(
                     details_fwd::use_static_cast_aux<targetT, sourceT>(),
                     forward<sourceT>(src));
  }

ROCKET_ALWAYS_INLINE
uint64_t
mulh128(uint64_t x, uint64_t y, uint64_t* lo = nullptr)
  noexcept
  {
#ifdef __SIZEOF_INT128__
    __extension__ unsigned __int128 r = (unsigned __int128) x * y;
    if(lo) *lo = (uint64_t) r;
    return (uint64_t) (r >> 64);
#else
    // https://github.com/catid/fp61/blob/master/fp61.h
    constexpr uint64_t M = 0x100000000ULL;
    uint64_t xl = x % M, xh = x / M;
    uint64_t yl = y % M, yh = y / M;
    uint64_t xlyl = xl * yl, xhyl = xh * yl, xlyh = xl * yh, xhyh = xh * yh;
    if(lo) *lo = xlyl + xhyl / M + xlyh / M;
    return (xlyl / M + xhyl + xlyh % M) / M + xlyh / M + xhyh;
#endif
  }

}  // namespace rocket
#endif
