// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_LINEAR_BUFFER_
#define ROCKET_LINEAR_BUFFER_

#include "fwd.hpp"
#include "xassert.hpp"
#include "xthrow.hpp"
#include "xstring.hpp"
#include "xallocator.hpp"
namespace rocket {

template<typename charT, typename allocT = allocator<charT>>
class basic_linear_buffer;

template<typename charT>
class basic_tinyfmt;

#include "details/linear_buffer.ipp"

template<typename charT, typename allocT>
class basic_linear_buffer
  {
    static_assert(!is_array<charT>::value, "invalid character type");
    static_assert(!is_reference<charT>::value, "invalid character type");
    static_assert(is_trivial<charT>::value, "characters must be trivial");

  public:
    using value_type       = charT;
    using allocator_type   = allocT;
    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;

  private:
    details_linear_buffer::basic_storage<allocator_type> m_stor;
    size_type m_goff = 0;  // offset of the beginning
    size_type m_eoff = 0;  // offset of the end

  public:
    constexpr
    basic_linear_buffer() noexcept(is_nothrow_constructible<allocator_type>::value)
      :
        m_stor()
      { }

    explicit constexpr
    basic_linear_buffer(const allocator_type& alloc) noexcept
      :
        m_stor(alloc)
      { }

    basic_linear_buffer(const basic_linear_buffer& other)
      :
        m_stor(allocator_traits<allocator_type>::select_on_container_copy_construction(
                                                     other.m_stor.as_allocator()))
      { this->putn(other.data(), other.size());  }

    basic_linear_buffer(const basic_linear_buffer& other, const allocator_type& alloc)
      :
        m_stor(alloc)
      { this->putn(other.data(), other.size());  }

    basic_linear_buffer(basic_linear_buffer&& other) noexcept
      :
        m_stor(::std::move(other.m_stor.as_allocator()))
      { this->do_exchange_with(other);  }

    basic_linear_buffer(basic_linear_buffer&& other, const allocator_type& alloc)
      noexcept(is_always_equal_allocator<allocator_type>::value)
      :
        m_stor(alloc)
      {
        if(ROCKET_EXPECT(this->m_stor.as_allocator() == alloc))
          // If the allocators compare equal, they can deallocate memory allocated by each other.
          this->do_exchange_with(other);
        else
          // Otherwise, we have to copy its contents instead.
          this->putn(other.data(), other.size());
      }

    basic_linear_buffer&
    operator=(const basic_linear_buffer& other) &
      {
        if(ROCKET_EXPECT(this->m_stor.as_allocator() == other.m_stor.as_allocator())) {
          // If the allocators compare equal, they can deallocate memory allocated by each other.
          noadl::propagate_allocator_on_copy(this->m_stor.as_allocator(), other.m_stor.as_allocator());
          if(ROCKET_EXPECT(this != &other))
            this->do_copy_from(other);
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
          this->do_copy_from(other);
        }
        return *this;
      }

    basic_linear_buffer&
    operator=(basic_linear_buffer&& other) &
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
          this->do_copy_from(other);
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
    do_copy_from(const basic_linear_buffer& other)
      {
        ROCKET_ASSERT(this != &other);
        this->clear();
        this->putn(other.data(), other.size());
      }

  public:
    constexpr
    const allocator_type&
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

    ROCKET_ALWAYS_INLINE
    basic_linear_buffer&
    discard(size_type nbump) noexcept
      {
        ROCKET_ASSERT(nbump <= this->m_eoff - this->m_goff);
#ifdef ROCKET_DEBUG
        ::memset(this->mut_data(), 0xC7, nbump * sizeof(value_type));
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

        noadl::xmempcpy(s, this->data(), nread);
        return nread;
      }

    size_type
    getn(value_type* s, size_type n) noexcept
      {
        size_type nread = noadl::min(n, this->size());
        if(nread <= 0)
          return 0;

        noadl::xmempcpy(s, this->data(), nread);
        this->discard(nread);
        return nread;
      }

    int
    peekc() const noexcept
      {
        value_type c;
        if(this->peekn(&c, 1) == 0)
          return -1;

        return static_cast<int>(static_cast<typename conditional<
                       is_same<volatile value_type, volatile char>::value,
                       unsigned char, value_type>::type>(c));
      }

    int
    getc() noexcept
      {
        value_type c;
        if(this->getn(&c, 1) == 0)
          return -1;

        return static_cast<int>(static_cast<typename conditional<
                       is_same<volatile value_type, volatile char>::value,
                       unsigned char, value_type>::type>(c));
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
    capacity_after_end() const noexcept
      { return this->m_stor.capacity() - this->m_eoff;  }

    size_type
    reserve_after_end(size_type nbump)
      {
        if(ROCKET_UNEXPECT(nbump > this->capacity_after_end())) {
          // Relocate existent data.
          size_type nused = this->m_stor.reserve(this->m_goff, this->m_eoff, nbump);
          this->m_goff = 0;
          this->m_eoff = nused;
        }
#ifdef ROCKET_DEBUG
        ::memset(this->mut_end(), 0xD3, nbump * sizeof(value_type));
#endif
        return this->capacity_after_end();
      }

    ROCKET_ALWAYS_INLINE
    basic_linear_buffer&
    accept(size_type nbump) noexcept
      {
        ROCKET_ASSERT(nbump <= this->capacity_after_end());
        this->m_eoff += nbump;
        return *this;
      }

    ROCKET_ALWAYS_INLINE
    basic_linear_buffer&
    unaccept(size_type nbump) noexcept
      {
        ROCKET_ASSERT(nbump <= this->m_eoff - this->m_goff);
        this->m_eoff -= nbump;
#ifdef ROCKET_DEBUG
        ::memset(this->mut_end(), 0xB6, nbump * sizeof(value_type));
#endif
        return *this;
      }

    basic_linear_buffer&
    putc(value_type c)
      {
        this->reserve_after_end(1);
        this->mut_end()[0] = c;
        this->accept(1);
        return *this;
      }

    basic_linear_buffer&
    putn(size_type n, value_type c)
      {
        this->reserve_after_end(n);
        noadl::xmempset(this->mut_end(), c, n);
        this->accept(n);
        return *this;
      }

    basic_linear_buffer&
    putn(const value_type* s, size_type n)
      {
        this->reserve_after_end(n);
        noadl::xmempcpy(this->mut_end(), s, n);
        this->accept(n);
        return *this;
      }

    basic_linear_buffer&
    puts(const value_type* s)
      {
        return this->putn(s, noadl::xstrlen(s));
      }
  };

template<typename charT, typename allocT>
inline
void
swap(basic_linear_buffer<charT, allocT>& lhs, basic_linear_buffer<charT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

template<typename charT, typename allocT>
inline
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const basic_linear_buffer<charT, allocT>& buf)
  {
    return fmt.putn(buf.data(), buf.size());
  }

using linear_buffer      = basic_linear_buffer<char>;
using linear_wbuffer     = basic_linear_buffer<wchar_t>;
using linear_u16buffer   = basic_linear_buffer<char16_t>;
using linear_u32wbuffer  = basic_linear_buffer<char32_t>;

extern template class basic_linear_buffer<char>;
extern template class basic_linear_buffer<wchar_t>;
extern template class basic_linear_buffer<char16_t>;
extern template class basic_linear_buffer<char32_t>;

}  // namespace rocket
#endif
