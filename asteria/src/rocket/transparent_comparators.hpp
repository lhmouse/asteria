// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TRANSPARENT_COMPARATORS_HPP_
#define ROCKET_TRANSPARENT_COMPARATORS_HPP_

#include "compatibility.h"
#include <utility>  // std::declval(), std::forward()

namespace rocket {

struct transparent_equal_to
  {
    using is_transparent = void;

    template<typename lhsT, typename rhsT>
     constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const noexcept(noexcept(::std::declval<lhsT>() == ::std::declval<rhsT>()))
        -> decltype(::std::declval<lhsT>() == ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) == ::std::forward<rhsT>(rhs);
      }
  };

struct transparent_not_equal_to
  {
    using is_transparent = void;

    template<typename lhsT, typename rhsT>
     constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const noexcept(noexcept(::std::declval<lhsT>() != ::std::declval<rhsT>()))
        -> decltype(::std::declval<lhsT>() != ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) != ::std::forward<rhsT>(rhs);
      }
  };

struct transparent_less
  {
    using is_transparent = void;

    template<typename lhsT, typename rhsT>
     constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const noexcept(noexcept(::std::declval<lhsT>() < ::std::declval<rhsT>()))
        -> decltype(::std::declval<lhsT>() < ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) < ::std::forward<rhsT>(rhs);
      }
  };

struct transparent_greater
  {
    using is_transparent = void;

    template<typename lhsT, typename rhsT>
     constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const noexcept(noexcept(::std::declval<lhsT>() > ::std::declval<rhsT>()))
        -> decltype(::std::declval<lhsT>() > ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) > ::std::forward<rhsT>(rhs);
      }
  };

struct transparent_less_equal
  {
    using is_transparent = void;

    template<typename lhsT, typename rhsT>
     constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const noexcept(noexcept(::std::declval<lhsT>() <= ::std::declval<rhsT>()))
        -> decltype(::std::declval<lhsT>() <= ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) <= ::std::forward<rhsT>(rhs);
      }
  };

struct transparent_greater_equal
  {
    using is_transparent = void;

    template<typename lhsT, typename rhsT>
     constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const noexcept(noexcept(::std::declval<lhsT>() >= ::std::declval<rhsT>()))
        -> decltype(::std::declval<lhsT>() >= ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) >= ::std::forward<rhsT>(rhs);
      }
  };

}

#endif
