// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UTILITIES_HPP_
#  error Please include <rocket/utilities.hpp> instead.
#endif

namespace details_utilities {

// `estimate_distance()`
template<typename iteratorT>
    constexpr size_t estimate_distance_aux(input_iterator_tag, iteratorT /*first*/, iteratorT /*last*/)
  {
    return 0;
  }
template<typename iteratorT>
    size_t estimate_distance_aux(forward_iterator_tag, iteratorT first, iteratorT last)
  {
    size_t total = 0;
    for(auto qit = noadl::move(first); qit != last; ++qit)
      ++total;
    return total;
  }
template<typename iteratorT>
    constexpr size_t estimate_distance_aux(random_access_iterator_tag, iteratorT first, iteratorT last)
  {
    return static_cast<size_t>(last - first);
  }

// `for_each()`
template<typename containerT, typename callbackT>
    void for_each_nonconstexpr(containerT&& cont, callbackT&& callback)
  {
    for(auto&& qelem : cont)
      noadl::forward<callbackT>(callback)(qelem);
  }

// `any_of()`
template<typename containerT, typename callbackT>
    bool any_of_nonconstexpr(containerT&& cont, callbackT&& callback)
  {
    for(auto&& qelem : cont) {
      if(noadl::forward<callbackT>(callback)(qelem))
        return true;
    }
    return false;
  }

// `none_of()`
template<typename containerT, typename callbackT>
    bool none_of_nonconstexpr(containerT&& cont, callbackT&& callback)
  {
    for(auto&& qelem : cont) {
      if(noadl::forward<callbackT>(callback)(qelem))
        return false;
    }
    return true;
  }

// `is_any_of()`
template<typename targetT, typename containerT>
    bool is_any_of_nonconstexpr(targetT&& targ, containerT&& cont)
  {
    for(auto&& qelem : cont) {
      if(noadl::forward<targetT>(targ) == qelem)
        return true;
    }
    return false;
  }

// `is_none_of()`
template<typename targetT, typename containerT>
    bool is_none_of_nonconstexpr(targetT&& targ, containerT&& cont)
  {
    for(auto&& qelem : cont) {
      if(noadl::forward<targetT>(targ) == qelem)
        return false;
    }
    return true;
  }

// `lowest_signed` and `lowest_unsigned`
template<typename integerT, integerT valueT, typename... candidatesT>
    struct integer_selector  // Be SFINAE-friendly.
  {
  };
template<typename integerT, integerT valueT, typename firstT, typename... remainingT>
    struct integer_selector<integerT, valueT, firstT, remainingT...>
      : conditional<firstT(valueT) != valueT,
                    integer_selector<integerT, valueT, remainingT...>,
                    enable_if<1, firstT>>::type
  {
  };

// `static_or_dynamic_cast()`
template<typename targetT, typename sourceT, typename = void>
   struct can_static_cast_aux : false_type
  {
  };
template<typename targetT, typename sourceT>
   struct can_static_cast_aux<targetT, sourceT,
     decltype(static_cast<void>(static_cast<targetT>(::std::declval<sourceT>())))> : true_type
  {
  };

template<typename targetT, typename sourceT>
    constexpr targetT static_or_dynamic_cast_aux(true_type, sourceT&& src)
  {
    return static_cast<targetT>(noadl::forward<sourceT>(src));
  }
template<typename targetT, typename sourceT>
    constexpr targetT static_or_dynamic_cast_aux(false_type, sourceT&& src)
  {
    return dynamic_cast<targetT>(noadl::forward<sourceT>(src));
  }

}  // namespace details_utilities
