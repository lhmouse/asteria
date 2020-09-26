// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ALLOCATOR_UTIL_HPP_
#define ROCKET_ALLOCATOR_UTIL_HPP_

#include "fwd.hpp"
#include <type_traits>  // std::conditional<>, std::false_type, std::true_type

namespace rocket {

#include "details/allocator_util.ipp"

template<typename allocT>
struct allocator_wrapper_base_for
  : conditional<is_final<allocT>::value,
                details_allocator_util::final_wrapper<allocT>,
                allocT>
  { };

template<typename allocT>
void
propagate_allocator_on_copy(allocT& lhs, const allocT& rhs)
noexcept
  {
    details_allocator_util::propagate<allocT>(
        typename conditional<allocator_traits<allocT>::propagate_on_container_copy_assignment::value,
                             details_allocator_util::propagate_copy_tag,
                             details_allocator_util::propagate_none_tag>::type(),
        lhs, rhs);
  }

template<typename allocT>
void
propagate_allocator_on_move(allocT& lhs, allocT& rhs)
noexcept
  {
    details_allocator_util::propagate<allocT>(
        typename conditional<allocator_traits<allocT>::propagate_on_container_move_assignment::value,
                             details_allocator_util::propagate_move_tag,
                             details_allocator_util::propagate_none_tag>::type(),
        lhs, rhs);
  }

template<typename allocT>
void
propagate_allocator_on_swap(allocT& lhs, allocT& rhs)
noexcept
  {
    details_allocator_util::propagate<allocT>(
        typename conditional<allocator_traits<allocT>::propagate_on_container_swap::value,
                             details_allocator_util::propagate_swap_tag,
                             details_allocator_util::propagate_none_tag>::type(),
        lhs, rhs);
  }

template<typename allocT>
struct is_std_allocator
  : false_type
  { };

template<typename valueT>
struct is_std_allocator<::std::allocator<valueT>>
  : true_type
  { };

template<typename allocT>
struct is_always_equal_allocator
#if __cpp_lib_allocator_traits_is_always_equal + 0 < 201411  // < c++17
  : is_std_allocator<allocT>
#else  // >= c++17
  : allocator_traits<allocT>::is_always_equal
#endif
  { };

}  // namespace rocket

#endif
