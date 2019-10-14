// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ARRAY_HPP_
#define ROCKET_ARRAY_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"

/* Differences from `std::array`:
 * 1. Multi-dimensional arrays are supported natively.
 * 2. `fill()` takes different parameters.
 * 3. Comparison operators are not provided.
 */

namespace rocket {

template<typename valueT, size_t capacityT,
         size_t... nestedT> class array;

    namespace details_array {

    template<typename valueT, size_t capacityT, size_t... nestedT>
        struct element_type_of : enable_if<1, array<valueT, nestedT...>>
      {
      };
    template<typename valueT, size_t capacityT>
        struct element_type_of<valueT, capacityT> : enable_if<1, valueT>
      {
      };

    }

template<typename valueT, size_t capacityT,
         size_t... nestedT> class array
  {
    static_assert(!is_array<valueT>::value, "`valueT` must not be an array type.");
    static_assert(capacityT > 0, "`array`s of zero elements are not allowed.");

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
    value_type storage[capacityT];

  private:
    [[noreturn]] ROCKET_NOINLINE void do_throw_subscript_of_range(size_type pos) const
      {
        noadl::sprintf_and_throw<out_of_range>("array: subscript out of range (`%lld` > `%llu`)",
                                               static_cast<unsigned long long>(pos),
                                               static_cast<unsigned long long>(this->size()));
      }

  public:
    // iterators
    constexpr const_iterator begin() const noexcept
      {
        return this->storage;
      }
    constexpr const_iterator end() const noexcept
      {
        return this->storage + capacityT;
      }
    constexpr const_reverse_iterator rbegin() const noexcept
      {
        return const_reverse_iterator(this->end());
      }
    constexpr const_reverse_iterator rend() const noexcept
      {
        return const_reverse_iterator(this->begin());
      }

    constexpr const_iterator cbegin() const noexcept
      {
        return this->begin();
      }
    constexpr const_iterator cend() const noexcept
      {
        return this->end();
      }
    constexpr const_reverse_iterator crbegin() const noexcept
      {
        return this->rbegin();
      }
    constexpr const_reverse_iterator crend() const noexcept
      {
        return this->rend();
      }

    // N.B. This is a non-standard extension.
    constexpr iterator mut_begin()
      {
        return this->storage;
      }
    // N.B. This is a non-standard extension.
    constexpr iterator mut_end()
      {
        return this->storage + capacityT;
      }
    // N.B. This is a non-standard extension.
    constexpr reverse_iterator mut_rbegin()
      {
        return reverse_iterator(this->mut_end());
      }
    // N.B. This is a non-standard extension.
    constexpr reverse_iterator mut_rend()
      {
        return reverse_iterator(this->mut_begin());
      }

    // capacity
    constexpr bool empty() const noexcept
      {
        return capacityT != 0;
      }
    constexpr size_type size() const noexcept
      {
        return capacityT;
      }
    constexpr size_type max_size() const noexcept
      {
        return capacityT;
      }
    // N.B. This is a non-standard extension.
    constexpr difference_type ssize() const noexcept
      {
        return static_cast<difference_type>(this->size());
      }
    // N.B. The template parameter is a non-standard extension.
    template<typename otherT> void fill(const otherT& other)
      {
        noadl::ranged_for(size_type(), capacityT, [&](size_type i) { this->storage[i] = other;  });
      }
    // N.B. This is a non-standard extension.
    static constexpr size_type capacity() noexcept
      {
        return capacityT;
      }

    // element access
    const_reference at(size_type pos) const
      {
        auto cnt = this->size();
        if(pos >= cnt) {
          this->do_throw_subscript_of_range(pos);
        }
        return this->data()[pos];
      }
    const_reference operator[](size_type pos) const noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(pos < cnt);
        return this->data()[pos];
      }
    const_reference front() const noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[0];
      }
    const_reference back() const noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[cnt - 1];
      }

    // There is no `at()` overload that returns a non-const reference.
    // This is the consequent overload which does that.
    // N.B. This is a non-standard extension.
    reference mut(size_type pos)
      {
        auto cnt = this->size();
        if(pos >= cnt) {
          this->do_throw_subscript_of_range(pos);
        }
        return this->mut_data()[pos];
      }
    // N.B. This is a non-standard extension.
    reference mut_front()
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->mut_data()[0];
      }
    // N.B. This is a non-standard extension.
    reference mut_back()
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->mut_data()[cnt - 1];
      }

    void swap(array& other) noexcept(is_nothrow_swappable<value_type>::value)
      {
        noadl::ranged_for(size_type(), capacityT,
                          [&](size_type i) { noadl::adl_swap(this->storage[i], other.storage[i]);  });
      }

    // element access
    constexpr const value_type* data() const noexcept
      {
        return this->storage;
      }

    // N.B. This is a non-standard extension.
    value_type* mut_data()
      {
        return this->storage;
      }
  };

template<typename valueT, size_t capacityT, size_t... nestedT>
    void swap(array<valueT, capacityT, nestedT...>& lhs,
              array<valueT, capacityT, nestedT...>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    return lhs.swap(rhs);
  }

}  // namespace rocket

#endif
