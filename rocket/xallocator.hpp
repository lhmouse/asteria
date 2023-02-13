// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XALLOCATOR_
#define ROCKET_XALLOCATOR_

#include "fwd.hpp"
namespace rocket {

#include "details/xallocator.ipp"

template<typename allocT>
struct allocator_wrapper_base_for
  : conditional<is_final<allocT>::value,
                details_xallocator::final_wrapper<allocT>,
                allocT>
  { };

template<typename allocT>
void
propagate_allocator_on_copy(allocT& lhs, const allocT& rhs) noexcept
  {
    details_xallocator::propagate<allocT>(
        typename conditional<allocator_traits<allocT>::propagate_on_container_copy_assignment::value,
                             details_xallocator::propagate_copy_tag,
                             details_xallocator::propagate_none_tag>::type(),
        lhs, rhs);
  }

template<typename allocT>
void
propagate_allocator_on_move(allocT& lhs, allocT& rhs) noexcept
  {
    details_xallocator::propagate<allocT>(
        typename conditional<allocator_traits<allocT>::propagate_on_container_move_assignment::value,
                             details_xallocator::propagate_move_tag,
                             details_xallocator::propagate_none_tag>::type(),
        lhs, rhs);
  }

template<typename allocT>
void
propagate_allocator_on_swap(allocT& lhs, allocT& rhs) noexcept
  {
    details_xallocator::propagate<allocT>(
        typename conditional<allocator_traits<allocT>::propagate_on_container_swap::value,
                             details_xallocator::propagate_swap_tag,
                             details_xallocator::propagate_none_tag>::type(),
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
