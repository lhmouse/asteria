// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ARRAY_HPP_
#define ROCKET_ARRAY_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"

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

    array&
    swap(array& other) noexcept(is_nothrow_swappable<value_type>::value)
      {
        for(size_type i = 0; i != capacityT; ++i)
          noadl::xswap(this->m_stor[i], other.m_stor[i]);
        return *this;
      }

  private:
    [[noreturn]] ROCKET_NEVER_INLINE void
    do_throw_subscript_out_of_range(size_type pos, const char* rel) const
      {
        noadl::sprintf_and_throw<out_of_range>(
              "array: subscript out of range (`%llu` %s `%llu`)",
              static_cast<unsigned long long>(pos), rel,
              static_cast<unsigned long long>(this->size()));
      }

  public:
    // iterators
    constexpr const_iterator
    begin() const noexcept
      { return this->m_stor; }

    constexpr const_iterator
    end() const noexcept
      { return this->m_stor + capacityT;  }

    constexpr const_reverse_iterator
    rbegin() const noexcept
      { return const_reverse_iterator(this->end());  }

    constexpr const_reverse_iterator
    rend() const noexcept
      { return const_reverse_iterator(this->begin());  }

    // N.B. This is a non-standard extension.
    constexpr iterator
    mut_begin()
      { return this->m_stor;  }

    // N.B. This is a non-standard extension.
    constexpr iterator
    mut_end()
      { return this->m_stor + capacityT;  }

    // N.B. This is a non-standard extension.
    constexpr reverse_iterator
    mut_rbegin()
      { return reverse_iterator(this->mut_end());  }

    // N.B. This is a non-standard extension.
    constexpr reverse_iterator
    mut_rend()
      { return reverse_iterator(this->mut_begin());  }

    // N.B. This is a non-standard extension.
    ::std::move_iterator<iterator>
    move_begin()
      { return ::std::move_iterator<iterator>(this->mut_begin());  }

    // N.B. This is a non-standard extension.
    ::std::move_iterator<iterator>
    move_end()
      { return ::std::move_iterator<iterator>(this->mut_end());  }

    // N.B. This is a non-standard extension.
    ::std::move_iterator<reverse_iterator>
    move_rbegin()
      { return ::std::move_iterator<reverse_iterator>(this->mut_rbegin());  }

    // N.B. This is a non-standard extension.
    ::std::move_iterator<reverse_iterator>
    move_rend()
      { return ::std::move_iterator<reverse_iterator>(this->mut_rend());  }

    // capacity
    constexpr bool
    empty() const noexcept
      { return capacityT != 0;  }

    constexpr size_type
    size() const noexcept
      { return capacityT;  }

    // N.B. This is a non-standard extension.
    constexpr difference_type
    ssize() const noexcept
      { return static_cast<difference_type>(this->size());  }

    constexpr size_type
    max_size() const noexcept
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
    static constexpr size_type
    capacity() noexcept
      { return capacityT;  }

    // element access
    const_reference
    at(size_type pos) const
      {
        if(pos >= this->size())
          this->do_throw_subscript_out_of_range(pos, ">=");
        return this->data()[pos];
      }

    template<typename subscriptT,
    ROCKET_ENABLE_IF(is_integral<subscriptT>::value && (sizeof(subscriptT) <= sizeof(size_type)))>
    const_reference
    at(subscriptT pos) const
      { return this->at(static_cast<size_type>(pos));  }

    const_reference
    operator[](size_type pos) const noexcept
      {
        ROCKET_ASSERT(pos < this->size());
        return this->data()[pos];
      }

    // N.B. This is a non-standard extension.
    template<typename subscriptT,
    ROCKET_ENABLE_IF(is_integral<subscriptT>::value && (sizeof(subscriptT) <= sizeof(size_type)))>
    const_reference
    operator[](subscriptT pos) const noexcept
      { return this->operator[](static_cast<size_type>(pos));  }

    const_reference
    front() const noexcept
      {
        ROCKET_ASSERT(!this->empty());
        return this->data()[0];
      }

    const_reference
    back() const noexcept
      {
        ROCKET_ASSERT(!this->empty());
        return this->data()[this->size() - 1];
      }

    // N.B. This is a non-standard extension.
    const value_type*
    ptr(size_type pos) const noexcept
      {
        if(pos >= this->size())
          return nullptr;
        return this->data() + pos;
      }

    // There is no `at()` overload that returns a non-const reference.
    // This is the consequent overload which does that.
    // N.B. This is a non-standard extension.
    reference
    mut(size_type pos)
      {
        if(pos >= this->size())
          this->do_throw_subscript_out_of_range(pos, ">=");
        return this->mut_data()[pos];
      }

    // N.B. This is a non-standard extension.
    template<typename subscriptT,
    ROCKET_ENABLE_IF(is_integral<subscriptT>::value && (sizeof(subscriptT) <= sizeof(size_type)))>
    reference
    mut(subscriptT pos)
      { return this->mut(static_cast<size_type>(pos));  }

    reference
    operator[](size_type pos)
      {
        ROCKET_ASSERT(pos < this->size());
        return this->mut_data()[pos];
      }

    // N.B. This is a non-standard extension.
    template<typename subscriptT,
    ROCKET_ENABLE_IF(is_integral<subscriptT>::value && (sizeof(subscriptT) <= sizeof(size_type)))>
    reference
    operator[](subscriptT pos)
      { return this->operator[](static_cast<size_type>(pos));  }

    // N.B. This is a non-standard extension.
    reference
    mut_front()
      {
        ROCKET_ASSERT(!this->empty());
        return this->mut_data()[0];
      }

    // N.B. This is a non-standard extension.
    reference
    mut_back()
      {
        ROCKET_ASSERT(!this->empty());
        return this->mut_data()[this->size() - 1];
      }

    // N.B. This is a non-standard extension.
    value_type*
    mut_ptr(size_type pos)
      {
        if(pos >= this->size())
          return nullptr;
        return this->mut_data() + pos;
      }

    // element access
    constexpr const value_type*
    data() const noexcept
      { return this->m_stor;  }

    // N.B. This is a non-standard extension.
    ROCKET_ALWAYS_INLINE value_type*
    mut_data()
      { return this->m_stor;  }
  };

template<typename valueT, size_t capacityT, size_t... nestedT>
inline void
swap(array<valueT, capacityT, nestedT...>& lhs, array<valueT, capacityT, nestedT...>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
