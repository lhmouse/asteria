// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ARRAY_
#define ROCKET_ARRAY_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"
namespace rocket {

// Differences from `std::array`:
// 1. Multi-dimensional arrays are supported natively.
// 2. `fill()` takes different parameters.
// 3. Comparison operators are not provided.
template<typename valueT, size_t capacityT, size_t... nestedT>
class array;

#include "details/array.ipp"

template<typename valueT, size_t capacityT, size_t... nestedT>
class array
  {
    static_assert(!is_array<valueT>::value, "invalid element type");
    static_assert(!is_reference<valueT>::value, "invalid element type");

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
        ::std::swap(this->m_stor, other.m_stor);
        return *this;
      }

  private:
    [[noreturn]] ROCKET_NEVER_INLINE
    void
    do_throw_subscript_out_of_range(size_type pos, unsigned char rel) const
      {
        static constexpr char opstr[6][3] = { "==", "<", "<=", ">", ">=", "!=" };
        unsigned int inv = 5U - rel;

        noadl::sprintf_and_throw<out_of_range>(
            "array: subscript out of range (`%lld` %s `%lld`)",
            static_cast<long long>(pos), opstr[inv], static_cast<long long>(this->size()));
      }

#define ROCKET_ARRAY_VALIDATE_SUBSCRIPT_(pos, op)  \
        if(!(pos op this->size()))  \
          this->do_throw_subscript_out_of_range(pos,  \
                  ((2 op 1) * 4 + (1 op 2) * 2 + (1 op 1) - 1))

  public:
    // iterators
    constexpr
    const_iterator
    begin() const noexcept
      { return this->m_stor; }

    constexpr
    const_iterator
    end() const noexcept
      { return this->m_stor + capacityT;  }

    constexpr
    const_reverse_iterator
    rbegin() const noexcept
      { return const_reverse_iterator(this->end());  }

    constexpr
    const_reverse_iterator
    rend() const noexcept
      { return const_reverse_iterator(this->begin());  }

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
    constexpr
    bool
    empty() const noexcept
      { return capacityT != 0;  }

    constexpr
    size_type
    size() const noexcept
      { return capacityT;  }

    // N.B. This is a non-standard extension.
    constexpr
    difference_type
    ssize() const noexcept
      { return static_cast<difference_type>(this->size());  }

    constexpr
    size_type
    max_size() const noexcept
      { return capacityT;  }

    // N.B. The template parameter is a non-standard extension.
    template<typename otherT>
    void
    fill(const otherT& other)
      {
        for(size_type i = 0; i != capacityT; ++i)
          this->m_stor[i] = other;
      }

    // N.B. This is a non-standard extension.
    static constexpr
    size_type
    capacity() noexcept
      { return capacityT;  }

    // element access
    const_reference
    at(size_type pos) const
      {
        ROCKET_ARRAY_VALIDATE_SUBSCRIPT_(pos, <);
        return this->data()[pos];
      }

    // N.B. This is a non-standard extension.
    const value_type*
    ptr(size_type pos) const noexcept
      {
        return (pos < this->size())
                 ? (this->data() + pos) : nullptr;
      }

    const_reference
    operator[](size_type pos) const noexcept
      {
        ROCKET_ASSERT(pos < this->size());
        return this->data()[pos];
      }

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

    // There is no `at()` overload that returns a non-const reference.
    // This is the consequent overload which does that.
    // N.B. This is a non-standard extension.
    reference
    mut(size_type pos)
      {
        ROCKET_ARRAY_VALIDATE_SUBSCRIPT_(pos, <);
        return this->mut_data()[pos];
      }

    // N.B. This is a non-standard extension.
    value_type*
    mut_ptr(size_type pos)
      {
        return (pos < this->size())
                 ? (this->mut_data() + pos) : nullptr;
      }

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

    // element access
    constexpr
    const value_type*
    data() const noexcept
      { return this->m_stor;  }

    // N.B. This is a non-standard extension.
    ROCKET_ALWAYS_INLINE
    value_type*
    mut_data()
      { return this->m_stor;  }
  };

template<typename valueT, size_t capacityT, size_t... nestedT>
inline
void
swap(array<valueT, capacityT, nestedT...>& lhs, array<valueT, capacityT, nestedT...>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

}  // namespace rocket
#endif
