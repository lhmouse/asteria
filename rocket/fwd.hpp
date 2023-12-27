// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FWD_
#define ROCKET_FWD_

#include "xcompiler.h"
#include <type_traits>  // so many...
#include <iterator>  // std::iterator_traits<>, std::begin(), std::end()
#include <utility>  // std::swap()
#include <memory>  // std::allocator<>, std::addressof(), std::default_delete<>, std::unique_ptr, std::shared_ptr
#include <new>  // placement new
#include <atomic>  // std::atomic<>
#include <initializer_list>  // std::initializer_list<>
#include <limits>  // std::numeric_limits<>
#include <functional>  // std::hash<>, std::equal_to<>
#include <tuple>  // std::tuple<>
#include <stdexcept>  // standard exceptions...
#include <cstring>  // std::memset()
#include <cstddef>  // std::size_t, std::ptrdiff_t
#include <cstdint>  // std::u?int(8|16|32|64|ptr)_t
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
using ::std::add_volatile;
using ::std::remove_volatile;
using ::std::add_cv;
using ::std::remove_cv;
using ::std::is_pointer;
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
using ::std::aligned_storage;
using ::std::aligned_union;
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

using ::std::allocator;
using ::std::allocator_traits;
using ::std::default_delete;
using ::std::hash;
using ::std::equal_to;
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

// Utility macros
#define ROCKET_CAR(x, ...)   x
#define ROCKET_CDR(x, ...)   __VA_ARGS__
#define ROCKET_CAT2(x,y)     x##y
#define ROCKET_CAT3(x,y,z)   x##y##z

#define ROCKET_STRINGIFY_NX(...)   #__VA_ARGS__
#define ROCKET_STRINGIFY(...)      ROCKET_STRINGIFY_NX(__VA_ARGS__)
#define ROCKET_SOURCE_LOCATION     __FILE__ ":" ROCKET_STRINGIFY(__LINE__)

#define ROCKET_MUL_ADD_OVERFLOW(x,y,z,r)   (ROCKET_MUL_OVERFLOW(x,y,r) | ROCKET_ADD_OVERFLOW(*(r),z,r))
#define ROCKET_ADD_MUL_OVERFLOW(x,y,z,r)   (ROCKET_ADD_OVERFLOW(x,y,r) | ROCKET_MUL_OVERFLOW(*(r),z,r))
#define ROCKET_MUL_SUB_OVERFLOW(x,y,z,r)   (ROCKET_MUL_OVERFLOW(x,y,r) | ROCKET_SUB_OVERFLOW(*(r),z,r))
#define ROCKET_SUB_MUL_OVERFLOW(x,y,z,r)   (ROCKET_SUB_OVERFLOW(x,y,r) | ROCKET_MUL_OVERFLOW(*(r),z,r))

#define ROCKET_VOID_T(...)       typename ::std::conditional<1, void, __VA_ARGS__>::type
#define ROCKET_ENABLE_IF(...)    typename ::std::enable_if<+bool(__VA_ARGS__)>::type* = nullptr
#define ROCKET_DISABLE_IF(...)   typename ::std::enable_if<!bool(__VA_ARGS__)>::type* = nullptr

#define ROCKET_VOID_DECLTYPE(...)         ROCKET_VOID_T(decltype(__VA_ARGS__))
#define ROCKET_ENABLE_IF_HAS_TYPE(...)    ROCKET_VOID_T(__VA_ARGS__)* = nullptr
#define ROCKET_ENABLE_IF_HAS_VALUE(...)   ROCKET_ENABLE_IF(sizeof(__VA_ARGS__) | 1)

#define ROCKET_DISABLE_SELF(x,...)  \
    ROCKET_DISABLE_IF(::std::is_same<x,  \
        typename ::std::remove_cv<typename ::std::remove_reference<__VA_ARGS__>::type>::type  \
          >::value)

template<typename typeT>
struct remove_cvref
  : remove_cv<typename remove_reference<typeT>::type>  { };

template<typename typeT>
struct is_nothrow_swappable
  : integral_constant<bool, noexcept(swap(declval<typeT&>(), declval<typeT&>()))>  { };

template<typename typeT>
struct identity
  : enable_if<true, typeT> { };

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
struct copy_cv
  : identity<targetT>  { };

template<typename targetT, typename sourceT>
struct copy_cv<targetT, const sourceT>
  : add_const<targetT>  { };

template<typename targetT, typename sourceT>
struct copy_cv<targetT, volatile sourceT>
  : add_volatile<targetT>  { };

template<typename targetT, typename sourceT>
struct copy_cv<targetT, const volatile sourceT>
  : add_cv<targetT>  { };

template<typename containerT>
constexpr
decltype(declval<const containerT&>().size())
size(const containerT& cont) noexcept(noexcept(cont.size()))
  { return cont.size();  }

template<typename elementT, size_t countT>
constexpr
size_t
size(const elementT (&)[countT]) noexcept
  { return countT;  }

template<typename containerT>
constexpr
decltype(static_cast<ptrdiff_t>(declval<const containerT&>().size()))
ssize(const containerT& cont) noexcept(noexcept(static_cast<ptrdiff_t>(cont.size())))
  { return static_cast<ptrdiff_t>(cont.size());  }

template<typename elementT, size_t countT>
constexpr
ptrdiff_t
ssize(const elementT (&)[countT]) noexcept
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
xswap(typeT& lhs, typeT& rhs) noexcept(noexcept(swap(lhs, rhs)))
  { swap(lhs, rhs);  }

template<typename firstT, typename secondT, typename... restT>
struct select_type
  : select_type<typename select_type<firstT, secondT>::type, restT...>  { };

template<typename firstT, typename secondT>
struct select_type<firstT, secondT>
  : identity<decltype(true ? declval<firstT()>()() : declval<secondT()>()())>  { };

template<typename lhsT, typename rhsT>
constexpr
typename select_type<lhsT&&, rhsT&&>::type
min(lhsT&& lhs, rhsT&& rhs)
  { return (rhs < lhs) ? forward<rhsT>(rhs) : forward<lhsT>(lhs);  }

template<typename lhsT, typename rhsT, typename... restT>
constexpr
typename select_type<lhsT&&, rhsT&&, restT&&...>::type
min(lhsT&& lhs, rhsT&& rhs, restT&&... rest)
  { return noadl::min(noadl::min(forward<lhsT>(lhs), forward<rhsT>(rhs)), forward<restT>(rest)...);  }

template<typename lhsT, typename rhsT>
constexpr
typename select_type<lhsT&&, rhsT&&>::type
max(lhsT&& lhs, rhsT&& rhs)
  { return (lhs < rhs) ? forward<rhsT>(rhs) : forward<lhsT>(lhs);  }

template<typename lhsT, typename rhsT, typename... restT>
constexpr
typename select_type<lhsT&&, rhsT&&, restT&&...>::type
max(lhsT&& lhs, rhsT&& rhs, restT&&... rest)
  { return noadl::max(noadl::max(forward<lhsT>(lhs), forward<rhsT>(rhs)), forward<restT>(rest)...);  }

template<typename xvT, typename loT, typename upT>
constexpr
typename select_type<xvT&&, loT&&, upT&&>::type
clamp(xvT&& xv, loT&& lo, upT&& up)
  { return (xv < lo) ? forward<loT>(lo) : (up < xv) ? forward<upT>(up) : forward<xvT>(xv);  }

template<typename resultT, typename xvT, typename loT, typename upT>
constexpr
resultT
clamp_cast(xvT&& xv, loT&& lo, upT&& up)
  { return static_cast<resultT>((xv < lo) ? forward<loT>(lo) : (up < xv) ? forward<upT>(up) : forward<xvT>(xv));  }

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

template<typename elementT, typename... paramsT>
ROCKET_ALWAYS_INLINE
elementT*
construct(elementT* ptr, paramsT&&... params) noexcept(is_nothrow_constructible<elementT, paramsT&&...>::value)
  {
#ifdef ROCKET_DEBUG
    ::std::memset((void*)ptr, 0xAA, sizeof(elementT));
#endif
    return ::new((void*)ptr) elementT(forward<paramsT>(params)...);
  }

template<typename elementT>
ROCKET_ALWAYS_INLINE
elementT*
default_construct(elementT* ptr) noexcept(is_nothrow_default_constructible<elementT>::value)
  {
#ifdef ROCKET_DEBUG
    ::std::memset((void*)ptr, 0xBE, sizeof(elementT));
#endif
    return ::new((void*)ptr) elementT;
  }

template<typename elementT>
ROCKET_ALWAYS_INLINE
void
destroy(elementT* ptr) noexcept(is_nothrow_destructible<elementT>::value)
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
reconstruct(elementT* ptr, paramsT&&... params) noexcept
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

    for(;;)
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

        // `isr` will have been decreased by `isl`, which will
        // not yield zero. `isl` is unchanged.
        stp = brk;
        isr = end - brk;
      }
      else if(isl > isr) {
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

        // `isl` will have been decreased by `isr`, which will
        // not yield zero. `isr` is unchanged.
        brk = stp;
        isl = stp - bot;
      }
      else {
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

        // `brk` equals `end` so there is no more work to do.
        break;
      }
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
int_from(charT c) noexcept
  {
    return static_cast<int>(c);
  }

ROCKET_PURE constexpr
int
int_from(char c) noexcept
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
mulh128(uint64_t x, uint64_t y, uint64_t* lo = nullptr) noexcept
  {
#ifdef __SIZEOF_INT128__
    __extension__ using my_uint128_t = unsigned __int128;
    my_uint128_t r = (my_uint128_t) x * y;
    lo && (*lo = (uint64_t) r);
    return (uint64_t) (r >> 64);
#else
    // https://github.com/catid/fp61/blob/master/fp61.h
    constexpr uint64_t M32 = 1ULL << 32;
    uint64_t xl = x % M32, xh = x / M32;
    uint64_t yl = y % M32, yh = y / M32;
    uint64_t xlyl = xl * yl, xhyl = xh * yl, xlyh = xl * yh, xhyh = xh * yh;
    lo && (*lo = xlyl + xhyl / M32 + xlyh / M32);
    return (xlyl / M32 + xhyl + xlyh % M32) / M32 + xlyh / M32 + xhyh;
#endif
  }

}  // namespace rocket
#endif
