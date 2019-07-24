// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ALLOCATOR_UTILITIES_HPP_
#define ROCKET_ALLOCATOR_UTILITIES_HPP_

#include <type_traits>  // std::conditional<>, std::false_type, std::true_type
#include "utilities.hpp"

namespace rocket {

    namespace details_allocator_utilities {

    template<typename allocatorT> class final_wrapper
      {
      private:
        allocatorT m_alloc;

      public:
        constexpr final_wrapper() noexcept(is_nothrow_constructible<allocatorT>::value)
          : m_alloc()
          {
          }
        explicit constexpr final_wrapper(const allocatorT& alloc) noexcept
          : m_alloc(alloc)
          {
          }
        explicit constexpr final_wrapper(allocatorT&& alloc) noexcept
          : m_alloc(noadl::move(alloc))
          {
          }

      public:
        constexpr operator const allocatorT& () const noexcept
          {
            return this->m_alloc;
          }
        operator allocatorT& () noexcept
          {
            return this->m_alloc;
          }
      };

    }  // namespace details_allocator_utilities

template<typename allocatorT> struct allocator_wrapper_base_for : conditional<std::is_final<allocatorT>::value,
                                                                              details_allocator_utilities::final_wrapper<allocatorT>,
                                                                              allocatorT>
  {
  };

    namespace details_allocator_utilities {

    // don't propagate
    struct propagate_none_tag
      {
      }
    constexpr propagate_none;

    template<typename allocatorT> void propagate(propagate_none_tag,
                                                 allocatorT& /*lhs*/, const allocatorT& /*rhs*/) noexcept
      {
      }

    // propagate_on_container_copy_assignment
    struct propagate_copy_tag
      {
      }
    constexpr propagate_copy;

    template<typename allocatorT> void propagate(propagate_copy_tag,
                                                 allocatorT& lhs, const allocatorT& rhs) noexcept
      {
        lhs = rhs;
      }

    // propagate_on_container_move_assignment
    struct propagate_move_tag
      {
      }
    constexpr propagate_move;

    template<typename allocatorT> void propagate(propagate_move_tag,
                                                 allocatorT& lhs, allocatorT& rhs) noexcept
      {
        lhs = noadl::move(rhs);
      }

    // propagate_on_container_swap
    struct propagate_swap_tag
      {
      }
    constexpr propagate_swap;

    template<typename allocatorT> void propagate(propagate_swap_tag,
                                                 allocatorT& lhs, allocatorT& rhs) noexcept
      {
        noadl::adl_swap(lhs, rhs);
      }

    }

template<typename allocatorT> void propagate_allocator_on_copy(allocatorT& lhs, const allocatorT& rhs) noexcept
  {
    details_allocator_utilities::propagate<allocatorT>(typename conditional<allocator_traits<allocatorT>::propagate_on_container_copy_assignment::value,
                                                                            details_allocator_utilities::propagate_copy_tag,
                                                                            details_allocator_utilities::propagate_none_tag>::type(),
                                                       lhs, rhs);
  }

template<typename allocatorT> void propagate_allocator_on_move(allocatorT& lhs, allocatorT&& rhs) noexcept
  {
    details_allocator_utilities::propagate<allocatorT>(typename conditional<allocator_traits<allocatorT>::propagate_on_container_move_assignment::value,
                                                                            details_allocator_utilities::propagate_move_tag,
                                                                            details_allocator_utilities::propagate_none_tag>::type(),
                                                       lhs, rhs);
  }

template<typename allocatorT> void propagate_allocator_on_swap(allocatorT& lhs, allocatorT& rhs) noexcept
  {
    details_allocator_utilities::propagate<allocatorT>(typename conditional<allocator_traits<allocatorT>::propagate_on_container_swap::value,
                                                                            details_allocator_utilities::propagate_swap_tag,
                                                                            details_allocator_utilities::propagate_none_tag>::type(),
                                                       lhs, rhs);
  }

template<typename allocatorT> struct is_std_allocator : false_type
  {
  };
template<typename valueT> struct is_std_allocator<::std::allocator<valueT>> : true_type
  {
  };

}  // namespace rocket

#endif
