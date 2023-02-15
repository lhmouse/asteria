// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FWD_
#  error Please include <rocket/fwd.hpp> instead.
#endif
namespace details_fwd {

// `is_input_iterator`
template<typename iteratorT, typename = void>
struct is_input_iterator_aux
  : false_type
  { };

template<typename iteratorT>
struct is_input_iterator_aux<iteratorT,
             ROCKET_VOID_DECLTYPE(static_cast<::std::input_iterator_tag>(
                    typename iterator_traits<iteratorT>::iterator_category()))>
  : true_type
  { };

// `estimate_distance()`
template<typename iteratorT>
constexpr
size_t
estimate_distance_aux(::std::input_iterator_tag, iteratorT /*first*/, iteratorT /*last*/)
  {
    return 0;
  }

template<typename iteratorT>
constexpr  // c++14
size_t
estimate_distance_aux(::std::forward_iterator_tag, iteratorT first, iteratorT last)
  {
    size_t total = 0;
    for(auto qit = ::std::move(first);  qit != last;  ++qit)
      ++total;
    return total;
  }

// `for_each()`
template<typename containerT, typename callbackT>
constexpr  // c++14
void
for_each_nonconstexpr(containerT&& cont, callbackT&& call)
  {
    for(auto&& elem : cont)
      ::std::forward<callbackT>(call)(elem);
  }

// `any_of()`
template<typename containerT, typename callbackT>
constexpr  // c++14
bool
any_of_nonconstexpr(containerT&& cont, callbackT&& call)
  {
    for(auto&& elem : cont)
      if(::std::forward<callbackT>(call)(elem))
        return true;
    return false;
  }

// `none_of()`
template<typename containerT, typename callbackT>
constexpr  // c++14
bool
none_of_nonconstexpr(containerT&& cont, callbackT&& call)
  {
    for(auto&& elem : cont)
      if(::std::forward<callbackT>(call)(elem))
        return false;
    return true;
  }

// `all_of()`
template<typename containerT, typename callbackT>
constexpr  // c++14
bool
all_of_nonconstexpr(containerT&& cont, callbackT&& call)
  {
    for(auto&& elem : cont)
      if(!bool(::std::forward<callbackT>(call)(elem)))
        return false;
    return true;
  }

// `is_any_of()`
template<typename targetT, typename containerT, typename predictorT>
constexpr  // c++14
bool
is_any_of_nonconstexpr(containerT&& cont, targetT&& targ, predictorT&& pred)
  {
    for(auto&& elem : cont)
      if(::std::forward<predictorT>(pred)(elem, targ))
        return true;
    return false;
  }

// `is_none_of()`
template<typename targetT, typename containerT, typename predictorT>
constexpr  // c++14
bool
is_none_of_nonconstexpr(containerT&& cont, targetT&& targ, predictorT&& pred)
  {
    for(auto&& elem : cont)
      if(::std::forward<predictorT>(pred)(elem, targ))
        return false;
    return true;
  }

// `find()`
template<typename containerT, typename targetT, typename predictorT>
constexpr  // c++14
typename remove_reference<decltype(*(begin(::std::declval<containerT>())))>::type*
find_nonconstexpr(containerT&& cont, targetT&& targ, predictorT&& pred)
  {
    for(auto&& elem : cont)
      if(::std::forward<predictorT>(pred)(elem, targ))
        return ::std::addressof(elem);
    return nullptr;
  }

template<typename containerT, typename predictorT>
constexpr  // c++14
typename remove_reference<decltype(*(begin(::std::declval<containerT>())))>::type*
find_if_nonconstexpr(containerT&& cont, predictorT&& pred)
  {
    for(auto&& elem : cont)
      if(::std::forward<predictorT>(pred)(elem))
        return ::std::addressof(elem);
    return nullptr;
  }

// `lowest_signed` and `lowest_unsigned`
template<typename integerT, integerT valueT, typename... candidatesT>
struct integer_selector;

template<typename integerT, integerT valueT, typename firstT, typename... remainingT>
struct integer_selector<integerT, valueT, firstT, remainingT...>
  : conditional<firstT(valueT) != valueT,
                integer_selector<integerT, valueT, remainingT...>,
                identity<firstT>>::type
  { };

// `static_or_dynamic_cast()`
template<typename targetT, typename sourceT, typename = void>
struct can_static_cast
  : false_type
  { };

template<typename targetT, typename sourceT>
struct can_static_cast<targetT, sourceT,
           ROCKET_VOID_DECLTYPE(static_cast<targetT>(::std::declval<sourceT>()))>
  : true_type
  { };

template<typename targetT, typename sourceT, typename = void>
struct can_dynamic_cast
  : false_type
  { };

template<typename targetT, typename sourceT>
struct can_dynamic_cast<targetT, sourceT,
           ROCKET_VOID_DECLTYPE(dynamic_cast<targetT>(::std::declval<sourceT>()))>
  : true_type
  { };

template<typename targetT, typename sourceT>
struct use_static_cast_aux
  : integral_constant<bool,
          can_static_cast<targetT, sourceT>::value || !can_dynamic_cast<targetT, sourceT>::value>
  { };

template<typename targetT, typename sourceT>
constexpr
targetT
static_or_dynamic_cast_aux(true_type, sourceT&& src)
  {
    return static_cast<targetT>(::std::forward<sourceT>(src));
  }

template<typename targetT, typename sourceT>
constexpr
targetT
static_or_dynamic_cast_aux(false_type, sourceT&& src)
  {
    return dynamic_cast<targetT>(::std::forward<sourceT>(src));
  }

template<typename valueT>
struct binder_eq
  {
    valueT m_val;

    constexpr
    binder_eq(const valueT& xval) noexcept(is_nothrow_copy_constructible<valueT>::value)
      : m_val(xval)  { }

    constexpr
    binder_eq(valueT&& xval) noexcept(is_nothrow_move_constructible<valueT>::value)
      : m_val(::std::move(xval))  { }

    template<typename xvalueT>
    constexpr
    bool
    operator()(const xvalueT& other) const
      { return this->m_val == other;  }
  };

template<typename valueT>
struct binder_ne
  {
    valueT m_val;

    constexpr
    binder_ne(const valueT& xval) noexcept(is_nothrow_copy_constructible<valueT>::value)
      : m_val(xval)  { }

    constexpr
    binder_ne(valueT&& xval) noexcept(is_nothrow_move_constructible<valueT>::value)
      : m_val(::std::move(xval))  { }

    template<typename xvalueT>
    constexpr
    bool
    operator()(const xvalueT& other) const
      { return this->m_val != other;  }
  };

}  // namespace details_fwd
