// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_LINEAR_BUFFER_HPP_
#define ROCKET_LINEAR_BUFFER_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "char_traits.hpp"
#include "xallocator.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>, typename allocT = allocator<charT>>
class basic_linear_buffer;

#include "details/linear_buffer.ipp"

template<typename charT, typename traitsT, typename allocT>
class basic_linear_buffer
  {
    static_assert(!is_array<charT>::value, "invalid character type");
    static_assert(is_trivial<charT>::value, "characters must be trivial");
    static_assert(is_same<typename allocT::value_type, charT>::value, "inappropriate allocator type");

  public:
    using value_type      = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocT;

    using int_type         = typename traits_type::int_type;
    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;

  private:
    details_linear_buffer::basic_storage<allocator_type, traits_type> m_stor;
    size_type m_goff = 0;  // offset of the beginning
    size_type m_eoff = 0;  // offset of the end

  public:
    explicit constexpr
    basic_linear_buffer(const allocator_type& alloc) noexcept
      : m_stor(alloc)
      { }

    basic_linear_buffer(const basic_linear_buffer& other)
      : m_stor(allocator_traits<allocator_type>::select_on_container_copy_construction(
                                                     other.m_stor.as_allocator()))
      { this->putn(other.data(), other.size());  }

    basic_linear_buffer(const basic_linear_buffer& other, const allocator_type& alloc)
      : m_stor(alloc)
      { this->putn(other.data(), other.size());  }

    basic_linear_buffer(basic_linear_buffer&& other) noexcept
      : m_stor(::std::move(other.m_stor.as_allocator()))
      { this->do_exchange_with(other);  }

    basic_linear_buffer(basic_linear_buffer&& other, const allocator_type& alloc)
      noexcept(is_always_equal_allocator<allocator_type>::value)
      : m_stor(alloc)
      {
        if(ROCKET_EXPECT(this->m_stor.as_allocator() == alloc))
          // If the allocators compare equal, they can deallocate memory allocated by each other.
          this->do_exchange_with(other);
        else
          // Otherwise, we have to copy its contents instead.
          this->putn(other.data(), other.size());
      }

    constexpr
    basic_linear_buffer() noexcept(is_nothrow_constructible<allocator_type>::value)
      : basic_linear_buffer(allocator_type())
      { }

    basic_linear_buffer&
    operator=(const basic_linear_buffer& other)
      {
        if(ROCKET_EXPECT(this->m_stor.as_allocator() == other.m_stor.as_allocator())) {
          // If the allocators compare equal, they can deallocate memory allocated by each other.
          noadl::propagate_allocator_on_copy(this->m_stor.as_allocator(), other.m_stor.as_allocator());
          if(ROCKET_EXPECT(this != &other))
            this->do_assign(other);
        }
        else if(allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value) {
          // Otherwise, if the allocator in `*this` will be replaced, the old buffer must be
          // deallocated first.
          this->do_deallocate();
          noadl::propagate_allocator_on_copy(this->m_stor.as_allocator(), other.m_stor.as_allocator());
          this->putn(other.data(), other.size());
        }
        else {
          // Otherwise, copy the contents without deallocating the old buffer.
          this->do_assign(other);
        }
        return *this;
      }

    basic_linear_buffer&
    operator=(basic_linear_buffer&& other)
      noexcept(is_always_equal_allocator<allocator_type>::value ||
               allocator_traits<allocator_type>::propagate_on_container_move_assignment::value)
      {
        if(ROCKET_EXPECT(this->m_stor.as_allocator() == other.m_stor.as_allocator())) {
          // If the allocators compare equal, they can deallocate memory allocated by each other.
          noadl::propagate_allocator_on_move(this->m_stor.as_allocator(), other.m_stor.as_allocator());
          ROCKET_ASSERT(this != &other);
          this->do_exchange_with(other);
        }
        else if(allocator_traits<allocator_type>::propagate_on_container_move_assignment::value) {
          // Otherwise, if the allocator in `*this` will be replaced, the old buffer must be
          // deallocated first.
          this->do_deallocate();
          noadl::propagate_allocator_on_move(this->m_stor.as_allocator(), other.m_stor.as_allocator());
          this->do_exchange_with(other);
        }
        else {
          // Otherwise, copy the contents without deallocating the old buffer.
          this->do_assign(other);
        }
        return *this;
      }

    basic_linear_buffer&
    swap(basic_linear_buffer& other)
      noexcept(is_always_equal_allocator<allocator_type>::value ||
               allocator_traits<allocator_type>::propagate_on_container_swap::value)
      {
        ROCKET_ASSERT((this->m_stor.as_allocator() == other.m_stor.as_allocator()) ||
                      allocator_traits<allocator_type>::propagate_on_container_swap::value);

        // Note that, as with standard containers, if the allocators are not swapped and they compare
        // unequal, the behavior is undefined.
        noadl::propagate_allocator_on_swap(this->m_stor.as_allocator(), other.m_stor.as_allocator());
        this->do_exchange_with(other);
        return *this;
      }

  private:
    void
    do_deallocate() noexcept
      {
        this->m_stor.deallocate();
        this->m_goff = 0;
        this->m_eoff = 0;
      }

    void
    do_exchange_with(basic_linear_buffer& other) noexcept
      {
        this->m_stor.exchange_with(other.m_stor);
        noadl::xswap(this->m_goff, other.m_goff);
        noadl::xswap(this->m_eoff, other.m_eoff);
      }

    void
    do_assign(const basic_linear_buffer& other)
      {
        ROCKET_ASSERT(this != &other);
        this->clear();
        this->putn(other.data(), other.size());
      }

  public:
    constexpr const allocator_type&
    get_allocator() const noexcept
      { return this->m_stor.as_allocator();  }

    allocator_type&
    get_allocator() noexcept
      { return this->m_stor.as_allocator();  }

    bool
    empty() const noexcept
      { return this->m_goff == this->m_eoff;  }

    size_type
    size() const noexcept
      { return this->m_eoff - this->m_goff;  }

    difference_type
    ssize() const noexcept
      { return static_cast<difference_type>(this->m_eoff - this->m_goff);  }

    size_type
    max_size() const noexcept
      { return this->m_stor.max_size();  }

    size_type
    capacity() const noexcept
      { return this->m_stor.capacity();  }

    basic_linear_buffer&
    clear() noexcept
      {
        this->m_goff = 0;
        this->m_eoff = 0;
        return *this;
      }

    // read functions
    const value_type*
    begin() const noexcept
      { return this->m_stor.data() + this->m_goff;  }

    const value_type*
    end() const noexcept
      { return this->m_stor.data() + this->m_eoff;  }

    const value_type*
    data() const noexcept
      { return this->m_stor.data() + this->m_goff;  }

    ROCKET_ALWAYS_INLINE basic_linear_buffer&
    discard(size_type nbump) noexcept
      {
        ROCKET_ASSERT(nbump <= this->m_eoff - this->m_goff);
#ifdef ROCKET_DEBUG
        traits_type::assign(this->mut_begin(), nbump, value_type(0xC7C7C7C7));
#endif
        this->m_goff += nbump;
        return *this;
      }

    size_type
    peekn(value_type* s, size_type n) const noexcept
      {
        size_type nread = noadl::min(n, this->size());
        if(nread <= 0)
          return 0;

        traits_type::copy(s, this->begin(), nread);
        return nread;
      }

    size_type
    getn(value_type* s, size_type n) noexcept
      {
        size_type nread = noadl::min(n, this->size());
        if(nread <= 0)
          return 0;

        traits_type::copy(s, this->begin(), nread);
        this->discard(nread);
        return nread;
      }

    int_type
    peekc() const noexcept
      {
        value_type s[1];
        if(this->peekn(s, 1) == 0)
          return traits_type::eof();

        int_type ch = traits_type::to_int_type(s[0]);
        return ch;
      }

    int_type
    getc() noexcept
      {
        value_type s[1];
        if(this->getn(s, 1) == 0)
          return traits_type::eof();

        int_type ch = traits_type::to_int_type(s[0]);
        return ch;
      }

    // write functions
    value_type*
    mut_begin() noexcept
      { return this->m_stor.mut_data() + this->m_goff;  }

    value_type*
    mut_end() noexcept
      { return this->m_stor.mut_data() + this->m_eoff;  }

    value_type*
    mut_data() noexcept
      { return this->m_stor.mut_data() + this->m_goff;  }

    size_type
    reserve(size_type nbump)
      {
        if(ROCKET_UNEXPECT(nbump > this->m_stor.capacity() - this->m_eoff)) {
          // Reallocate the buffer.
          auto nused = this->m_stor.reserve(this->m_goff, this->m_eoff, nbump);
          // Set up the new offsets as the contents have now been moved to the beginning.
          this->m_goff = 0;
          this->m_eoff = nused;
        }
#ifdef ROCKET_DEBUG
        traits_type::assign(this->mut_end(), nbump, value_type(0xD3D3D3D3));
#endif
        return this->m_stor.capacity() - this->m_eoff;
      }

    ROCKET_ALWAYS_INLINE basic_linear_buffer&
    accept(size_type nbump) noexcept
      {
        ROCKET_ASSERT(nbump <= this->m_stor.capacity() - this->m_eoff);
        this->m_eoff += nbump;
        return *this;
      }

    basic_linear_buffer&
    putc(value_type c)
      {
        this->reserve(1);
        traits_type::assign(this->mut_end()[0], c);
        this->accept(1);
        return *this;
      }

    basic_linear_buffer&
    putn(size_type n, value_type c)
      {
        this->reserve(n);
        traits_type::assign(this->mut_end(), n, c);
        this->accept(n);
        return *this;
      }

    basic_linear_buffer&
    putn(const value_type* s, size_type n)
      {
        this->reserve(n);
        traits_type::copy(this->mut_end(), s, n);
        this->accept(n);
        return *this;
      }

    basic_linear_buffer&
    puts(const value_type* s)
      { return this->putn(s, traits_type::length(s));  }
  };

template<typename charT, typename traitsT, typename allocT>
inline void
swap(basic_linear_buffer<charT, traitsT, allocT>& lhs, basic_linear_buffer<charT, traitsT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

extern template
class basic_linear_buffer<char>;

extern template
class basic_linear_buffer<wchar_t>;

using linear_buffer   = basic_linear_buffer<char>;
using linear_wbuffer  = basic_linear_buffer<wchar_t>;

}  // namespace rocket

#endif
