// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FWD_
#  error Please include <rocket/fwd.hpp> instead.
#endif
namespace details_fwd {

// `is_input_iterator`
template<typename iteratorT, typename = void>
struct is_input_iterator_aux
  :
    false_type
  {
  };

template<typename iteratorT>
struct is_input_iterator_aux<iteratorT,
             ROCKET_VOID_DECLTYPE(static_cast<::std::input_iterator_tag>(
                    typename iterator_traits<iteratorT>::iterator_category()))>
  :
    true_type
  {
  };

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
    for(auto qit = move(first);  qit != last;  ++qit)
      ++total;
    return total;
  }

// `lowest_signed` and `lowest_unsigned`
template<typename integerT, integerT valueT, typename... candidatesT>
struct integer_selector;

template<typename integerT, integerT valueT, typename firstT, typename... remainingT>
struct integer_selector<integerT, valueT, firstT, remainingT...>
  :
    conditional<firstT(valueT) != valueT,
                integer_selector<integerT, valueT, remainingT...>,
                enable_if<true, firstT>>::type
  {
  };

// `static_or_dynamic_cast()`
template<typename targetT, typename sourceT, typename = void>
struct can_static_cast
  :
    false_type
  {
  };

template<typename targetT, typename sourceT>
struct can_static_cast<targetT, sourceT,
           ROCKET_VOID_DECLTYPE(static_cast<targetT>(declval<sourceT>()))>
  :
    true_type
  {
  };

template<typename targetT, typename sourceT, typename = void>
struct can_dynamic_cast
  :
    false_type
  {
  };

template<typename targetT, typename sourceT>
struct can_dynamic_cast<targetT, sourceT,
           ROCKET_VOID_DECLTYPE(dynamic_cast<targetT>(declval<sourceT>()))>
  :
    true_type
  {
  };

template<typename targetT, typename sourceT>
struct use_static_cast_aux
  :
    integral_constant<bool,
          can_static_cast<targetT, sourceT>::value
          || !can_dynamic_cast<targetT, sourceT>::value>
  {
  };

template<typename targetT, typename sourceT>
constexpr
targetT
static_or_dynamic_cast_aux(true_type, sourceT&& src)
  {
    return static_cast<targetT>(forward<sourceT>(src));
  }

template<typename targetT, typename sourceT>
constexpr
targetT
static_or_dynamic_cast_aux(false_type, sourceT&& src)
  {
    return dynamic_cast<targetT>(forward<sourceT>(src));
  }

template<typename valueT>
struct binder_eq
  {
    valueT m_val;

    constexpr binder_eq(const valueT& xval)
          noexcept(is_nothrow_copy_constructible<valueT>::value)
      : m_val(xval)  { }

    constexpr binder_eq(valueT&& xval)
         noexcept(is_nothrow_move_constructible<valueT>::value)
      : m_val(move(xval))  { }

    template<typename xvalueT>
    constexpr
    bool
    operator()(const xvalueT& other)
      const
      { return this->m_val == other;  }
  };

template<typename valueT>
struct binder_ne
  {
    valueT m_val;

    constexpr binder_ne(const valueT& xval)
          noexcept(is_nothrow_copy_constructible<valueT>::value)
      : m_val(xval)  { }

    constexpr binder_ne(valueT&& xval)
          noexcept(is_nothrow_move_constructible<valueT>::value)
      : m_val(move(xval))  { }

    template<typename xvalueT>
    constexpr
    bool
    operator()(const xvalueT& other)
      const
      { return this->m_val != other;  }
  };

}  // namespace details_fwd
