// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TRANSPARENT_COMPARATORS_HPP_
#define ROCKET_TRANSPARENT_COMPARATORS_HPP_

#include "compatibility.h"
#include <type_traits>
#include <utility>

namespace rocket {

using ::std::true_type;

struct transparent_equal_to
  {
    template<typename lhsT, typename rhsT>
      constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const -> decltype(::std::declval<lhsT>() == ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) == ::std::forward<rhsT>(rhs);
      }

    using is_transparent = true_type;
  };

struct transparent_not_equal_to
  {
    template<typename lhsT, typename rhsT>
      constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const -> decltype(::std::declval<lhsT>() != ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) != ::std::forward<rhsT>(rhs);
      }

    using is_transparent = true_type;
  };

struct transparent_less
  {
    template<typename lhsT, typename rhsT>
    constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const -> decltype(::std::declval<lhsT>() < ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) < ::std::forward<rhsT>(rhs);
      }

    using is_transparent = true_type;
  };

struct transparent_greater
  {
    template<typename lhsT, typename rhsT>
      constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const -> decltype(::std::declval<lhsT>() > ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) > ::std::forward<rhsT>(rhs);
      }

    using is_transparent = true_type;
  };

struct transparent_less_equal
  {
    template<typename lhsT, typename rhsT>
      constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const -> decltype(::std::declval<lhsT>() <= ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) <= ::std::forward<rhsT>(rhs);
      }

    using is_transparent = true_type;
  };

struct transparent_greater_equal
  {
    template<typename lhsT, typename rhsT>
      constexpr auto operator()(lhsT &&lhs, rhsT &&rhs) const -> decltype(::std::declval<lhsT>() >= ::std::declval<rhsT>())
      {
        return ::std::forward<lhsT>(lhs) >= ::std::forward<rhsT>(rhs);
      }

    using is_transparent = true_type;
  };

}

#endif
