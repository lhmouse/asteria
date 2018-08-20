// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UTILITIES_HPP_
#define ROCKET_UTILITIES_HPP_

#include <type_traits> // std::common_type<>
#include <iterator> // std::iterator_traits<>
#include <utility> // std::swap(), std::move(), std::forward()
#include <new> // placement new
#include <initializer_list> // std::initializer_list<>
#include <cstddef> // std::size_t, std::ptrdiff_t

namespace rocket
{

namespace noadl = ::rocket;

using ::std::common_type;
using ::std::is_nothrow_constructible;
using ::std::is_nothrow_destructible;
using ::std::iterator_traits;
using ::std::initializer_list;
using ::std::size_t;
using ::std::ptrdiff_t;

template<typename withT, typename typeT>
  inline typeT exchange(typeT &ref, withT &&with)
    {
      auto old = ::std::move(ref);
      ref = ::std::forward<withT>(with);
      return old;
    }

template<typename lhsT, typename rhsT>
  inline void adl_swap(lhsT &lhs, rhsT &rhs)
    {
      using ::std::swap;
      swap(lhs, rhs);
    }

template<typename lhsT, typename rhsT>
  constexpr typename common_type<lhsT &&, rhsT &&>::type min(lhsT &&lhs, rhsT &&rhs)
    {
      return !(rhs < lhs) ? ::std::forward<lhsT>(lhs) : ::std::forward<rhsT>(rhs);
    }
template<typename lhsT, typename rhsT>
  constexpr typename common_type<lhsT &&, rhsT &&>::type max(lhsT &&lhs, rhsT &&rhs)
    {
      return !(lhs < rhs) ? ::std::forward<lhsT>(lhs) : ::std::forward<rhsT>(rhs);
    }

template<typename iteratorT, typename functionT, typename ...paramsT>
  inline void ranged_for(iteratorT first, iteratorT last, functionT &&func, paramsT &&...params)
    {
      for(auto it = ::std::move(first); it != last; ++it) {
        ::std::forward<functionT>(func)(it, ::std::forward<paramsT>(params)...);
      }
    }
template<typename iteratorT, typename functionT, typename ...paramsT>
  inline void ranged_do_while(iteratorT first, iteratorT last, functionT &&func, paramsT &&...params)
    {
      auto it = ::std::move(first);
      do {
        ::std::forward<functionT>(func)(it, ::std::forward<paramsT>(params)...);
      } while(++it != last);
    }

namespace details_utilities
  {
    template<typename iteratorT>
      constexpr size_t estimate_distance(::std::input_iterator_tag, iteratorT /*first*/, iteratorT /*last*/)
        {
          return 0;
        }
    template<typename iteratorT>
      inline size_t estimate_distance(::std::forward_iterator_tag, iteratorT first, iteratorT last)
        {
          size_t total = 0;
          for(auto it = ::std::move(first); it != last; ++it) {
            ++total;
          }
          return total;
        }
    template<typename iteratorT>
      constexpr size_t estimate_distance(::std::random_access_iterator_tag, iteratorT first, iteratorT last)
        {
          return static_cast<size_t>(last - first);
        }
  }

template<typename iteratorT>
  constexpr size_t estimate_distance(iteratorT first, iteratorT last)
    {
      return details_utilities::estimate_distance(typename iterator_traits<iteratorT>::iterator_category(),
                                                  ::std::move(first), ::std::move(last));
    }

template<typename elementT, typename ...paramsT>
  inline elementT * construct_at(elementT *ptr, paramsT &&...params) noexcept(is_nothrow_constructible<elementT, paramsT &&...>::value)
    {
      return ::new(static_cast<void *>(ptr)) elementT(::std::forward<paramsT>(params)...);
    }
template<typename elementT>
  inline void destroy_at(elementT *ptr) noexcept(is_nothrow_destructible<elementT>::value)
    {
      ptr->~elementT();
    }

template<typename elementT>
  inline bool is_any_of(const elementT &elem, initializer_list<elementT> init)
    {
      for(auto ptr = init.begin(); ptr != init.end(); ++ptr) {
        if(elem == *ptr) {
          return true;
        }
      }
      return false;
    }

}

#endif
