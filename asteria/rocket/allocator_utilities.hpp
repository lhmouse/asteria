// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ALLOCATOR_UTILITIES_HPP_
#define ROCKET_ALLOCATOR_UTILITIES_HPP_

#include "utilities.hpp"
#include <type_traits>  // std::conditional<>, std::false_type, std::true_type

namespace rocket {

#include "details/allocator_utilities.tcc"

template<typename allocT>
    struct allocator_wrapper_base_for : conditional<is_final<allocT>::value,
                                                    details_allocator_utilities::final_wrapper<allocT>,
                                                    allocT>
  {
  };

template<typename allocT>
    void propagate_allocator_on_copy(allocT& lhs, const allocT& rhs) noexcept
  {
    details_allocator_utilities::propagate<allocT>(
      typename conditional<allocator_traits<allocT>::propagate_on_container_copy_assignment::value,
                           details_allocator_utilities::propagate_copy_tag,
                           details_allocator_utilities::propagate_none_tag>::type(),
      lhs, rhs);
  }
template<typename allocT>
    void propagate_allocator_on_move(allocT& lhs, allocT& rhs) noexcept
  {
    details_allocator_utilities::propagate<allocT>(
      typename conditional<allocator_traits<allocT>::propagate_on_container_move_assignment::value,
                           details_allocator_utilities::propagate_move_tag,
                           details_allocator_utilities::propagate_none_tag>::type(),
      lhs, rhs);
  }
template<typename allocT>
    void propagate_allocator_on_swap(allocT& lhs, allocT& rhs) noexcept
  {
    details_allocator_utilities::propagate<allocT>(
      typename conditional<allocator_traits<allocT>::propagate_on_container_swap::value,
                           details_allocator_utilities::propagate_swap_tag,
                           details_allocator_utilities::propagate_none_tag>::type(),
      lhs, rhs);
  }

template<typename allocT>
    struct is_std_allocator : false_type
  {
  };
template<typename valueT>
    struct is_std_allocator<::std::allocator<valueT>> : true_type
  {
  };

}  // namespace rocket

#endif
