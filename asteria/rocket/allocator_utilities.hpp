// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ALLOCATOR_UTILITIES_HPP_
#define ROCKET_ALLOCATOR_UTILITIES_HPP_

#include "utilities.hpp"
#include <type_traits>  // std::conditional<>, std::false_type, std::true_type

namespace rocket {

    namespace details_allocator_utilities {

    template<typename allocT> class final_wrapper
      {
      private:
        allocT m_alloc;

      public:
        constexpr final_wrapper() noexcept(is_nothrow_constructible<allocT>::value)
          :
            m_alloc()
          {
          }
        explicit constexpr final_wrapper(const allocT& alloc) noexcept
          :
            m_alloc(alloc)
          {
          }
        explicit constexpr final_wrapper(allocT&& alloc) noexcept
          :
            m_alloc(noadl::move(alloc))
          {
          }

      public:
        constexpr operator const allocT& () const noexcept
          {
            return this->m_alloc;
          }
        operator allocT& () noexcept
          {
            return this->m_alloc;
          }
      };

    }  // namespace details_allocator_utilities

template<typename allocT>
    struct allocator_wrapper_base_for : conditional<is_final<allocT>::value,
                                                    details_allocator_utilities::final_wrapper<allocT>,
                                                    allocT>
  {
  };

    namespace details_allocator_utilities {

    // don't propagate
    struct propagate_none_tag
      {
      }
    constexpr propagate_none;

    template<typename allocT>
        void propagate(propagate_none_tag, allocT& /*lhs*/, const allocT& /*rhs*/) noexcept
      {
      }

    // propagate_on_container_copy_assignment
    struct propagate_copy_tag
      {
      }
    constexpr propagate_copy;

    template<typename allocT>
        void propagate(propagate_copy_tag, allocT& lhs, const allocT& rhs) noexcept
      {
        lhs = rhs;
      }

    // propagate_on_container_move_assignment
    struct propagate_move_tag
      {
      }
    constexpr propagate_move;

    template<typename allocT>
        void propagate(propagate_move_tag, allocT& lhs, allocT& rhs) noexcept
      {
        lhs = noadl::move(rhs);
      }

    // propagate_on_container_swap
    struct propagate_swap_tag
      {
      }
    constexpr propagate_swap;

    template<typename allocT>
        void propagate(propagate_swap_tag, allocT& lhs, allocT& rhs) noexcept
      {
        xswap(lhs, rhs);
      }

    }

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
