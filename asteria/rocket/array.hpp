// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ARRAY_HPP_
#define ROCKET_ARRAY_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"

namespace rocket {

template<typename valueT, size_t capacityT, size_t... nestedT>
class array;

#include "details/array.ipp"

/* Differences from `std::array`:
 * 1. Multi-dimensional arrays are supported natively.
 * 2. `fill()` takes different parameters.
 * 3. Comparison operators are not provided.
**/

template<typename valueT, size_t capacityT, size_t... nestedT>
class array
  {
    static_assert(!is_array<valueT>::value, "invalid element type");

  public:
    // types
    using value_type  = typename details_array::element_type_of<valueT, capacityT, nestedT...>::type;

    using size_type        = size_t;
    using difference_type  = ptrdiff_t;
    using const_reference  = const value_type&;
    using reference        = value_type&;

    using const_iterator          = const value_type*;
    using iterator                = value_type*;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

  public:
    // This class has to be an aggregate.
    value_type m_stor[capacityT];

  private:
    [[noreturn]] ROCKET_NOINLINE
    void
    do_throw_subscript_out_of_range(size_type pos)
    const
      {
        noadl::sprintf_and_throw<out_of_range>("array: subscript out of range (`%llu` > `%llu`)",
                                               static_cast<unsigned long long>(pos),
                                               static_cast<unsigned long long>(this->size()));
      }

  public:
    // iterators
    constexpr
    const_iterator
    begin()
    const noexcept
      { return this->m_stor; }

    constexpr
    const_iterator
    end()
    const noexcept
      { return this->m_stor + capacityT;  }

    constexpr
    const_reverse_iterator
    rbegin()
    const noexcept
      { return const_reverse_iterator(this->end());  }

    constexpr
    const_reverse_iterator
    rend()
    const noexcept
      { return const_reverse_iterator(this->begin());  }

    constexpr
    const_iterator
    cbegin()
    const noexcept
      { return this->begin();  }

    constexpr
    const_iterator
    cend()
    const noexcept
      { return this->end();  }

    constexpr
    const_reverse_iterator
    crbegin()
    const noexcept
      { return this->rbegin();  }

    constexpr
    const_reverse_iterator
    crend()
    const noexcept
      { return this->rend();  }

    // N.B. This is a non-standard extension.
    constexpr
    iterator
    mut_begin()
      { return this->m_stor;  }

    // N.B. This is a non-standard extension.
    constexpr
    iterator
    mut_end()
      { return this->m_stor + capacityT;  }

    // N.B. This is a non-standard extension.
    constexpr
    reverse_iterator
    mut_rbegin()
      { return reverse_iterator(this->mut_end());  }

    // N.B. This is a non-standard extension.
    constexpr
    reverse_iterator
    mut_rend()
      { return reverse_iterator(this->mut_begin());  }

    // capacity
    constexpr
    bool
    empty()
    const noexcept
      { return capacityT != 0;  }

    constexpr
    size_type
    size()
    const noexcept
      { return capacityT;  }

    // N.B. This is a non-standard extension.
    constexpr
    difference_type
    ssize()
    const noexcept
      { return static_cast<difference_type>(this->size());  }

    constexpr
    size_type
    max_size()
    const noexcept
      { return capacityT;  }

    // N.B. The template parameter is a non-standard extension.
    template<typename otherT>
    array&
    fill(const otherT& other)
      {
        for(size_type i = 0; i != capacityT; ++i)
          this->m_stor[i] = other;
        return *this;
      }

    // N.B. This is a non-standard extension.
    static
    constexpr
    size_type
    capacity()
    noexcept
      { return capacityT;  }

    // element access
    const_reference
    at(size_type pos)
    const
      {
        size_type cnt = this->size();
        if(pos >= cnt)
          this->do_throw_subscript_out_of_range(pos);
        return this->data()[pos];
      }

    const_reference
    operator[](size_type pos)
    const noexcept
      {
        size_type cnt = this->size();
        ROCKET_ASSERT(pos < cnt);
        return this->data()[pos];
      }

    const_reference
    front()
    const noexcept
      {
        size_type cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[0];
      }

    const_reference
    back()
    const noexcept
      {
        size_type cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[cnt - 1];
      }

    // N.B. This is a non-standard extension.
    const value_type*
    get_ptr(size_type pos)
    const noexcept
      {
        size_type cnt = this->size();
        if(pos >= cnt) {
          return nullptr;
        }
        return this->data() + pos;
      }

    // There is no `at()` overload that returns a non-const reference.
    // This is the consequent overload which does that.
    // N.B. This is a non-standard extension.
    reference
    mut(size_type pos)
      {
        size_type cnt = this->size();
        if(pos >= cnt) {
          this->do_throw_subscript_out_of_range(pos);
        }
        return this->mut_data()[pos];
      }

    reference
    operator[](size_type pos)
    noexcept
      {
        size_type cnt = this->size();
        ROCKET_ASSERT(pos < cnt);
        return this->mut_data()[pos];
      }

    // N.B. This is a non-standard extension.
    reference
    mut_front()
      {
        size_type cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->mut_data()[0];
      }

    // N.B. This is a non-standard extension.
    reference
    mut_back()
      {
        size_type cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->mut_data()[cnt - 1];
      }

    // N.B. This is a non-standard extension.
    value_type*
    mut_ptr(size_type pos)
    noexcept
      {
        size_type cnt = this->size();
        if(pos >= cnt)
          return nullptr;
        return this->mut_data() + pos;
      }

    array&
    swap(array& other)
    noexcept(is_nothrow_swappable<value_type>::value)
      {
        for(size_type i = 0; i != capacityT; ++i)
          noadl::xswap(this->m_stor[i], other.m_stor[i]);
        return *this;
      }

    // element access
    constexpr
    const value_type*
    data()
    const noexcept
      { return this->m_stor;  }

    // N.B. This is a non-standard extension.
    value_type*
    mut_data()
      { return this->m_stor;  }
  };

template<typename valueT, size_t capacityT, size_t... nestedT>
inline
void
swap(array<valueT, capacityT, nestedT...>& lhs, array<valueT, capacityT, nestedT...>& rhs)
noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
