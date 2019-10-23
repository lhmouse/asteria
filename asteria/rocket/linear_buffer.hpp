// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_LINEAR_BUFFER_HPP_
#define ROCKET_LINEAR_BUFFER_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "allocator_utilities.hpp"
#include "utilities.hpp"
#include "char_traits.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>,
         typename allocT = allocator<charT>> class basic_linear_buffer;

    namespace details_linear_buffer {

    template<typename allocT, typename traitsT>
        class basic_storage : private allocator_wrapper_base_for<allocT>::type
      {
      public:
        using allocator_type  = allocT;
        using traits_type     = traitsT;
        using value_type      = typename allocator_type::value_type;
        using size_type       = typename allocator_traits<allocator_type>::size_type;

      private:
        using allocator_base   = typename allocator_wrapper_base_for<allocator_type>::type;
        using storage_pointer  = typename allocator_traits<allocator_type>::pointer;

      private:
        storage_pointer m_ptr = { };  // pointer to allocated storage
        size_type m_cap = 0;  // size of allocated storage

      public:
        explicit constexpr basic_storage(const allocator_type& alloc) noexcept
          :
            allocator_base(alloc)
          {
          }
        explicit constexpr basic_storage(allocator_type&& alloc) noexcept
          :
            allocator_base(noadl::move(alloc))
          {
          }
        ~basic_storage()
          {
            this->deallocate();
          }

        basic_storage(const basic_storage&)
          = delete;
        basic_storage& operator=(const basic_storage&)
          = delete;

      public:
        const allocator_type& as_allocator() const noexcept
          {
            return static_cast<const allocator_base&>(*this);
          }
        allocator_type& as_allocator() noexcept
          {
            return static_cast<allocator_base&>(*this);
          }

        const value_type* data() const noexcept
          {
            return noadl::unfancy(this->m_ptr);
          }
        value_type* mut_data() noexcept
          {
            return noadl::unfancy(this->m_ptr);
          }
        size_type max_size() const noexcept
          {
            return allocator_traits<allocator_type>::max_size(this->as_allocator());
          }
        size_type capacity() const noexcept
          {
            return this->m_cap;
          }

        ROCKET_NOINLINE size_type reserve(size_type goff, size_type eoff, size_type nadd)
          {
            ROCKET_ASSERT(goff <= eoff);
            ROCKET_ASSERT(eoff <= this->m_cap);
            // Get a plain pointer to the beginning to allocated storage.
            auto pbuf = noadl::unfancy(this->m_ptr);
            // Get the number of characters that have been buffered.
            auto nused = eoff - goff;
            if(ROCKET_EXPECT(nadd <= this->m_cap - nused)) {
              // If the buffer is large enough for the combined data, shift the string to the beginning
              // to make some room after its the end.
              traits_type::move(pbuf, pbuf + goff, nused);
            }
            else {
              // If the buffer is not large enough, allocate a new one.
              auto nmax = this->max_size();
              if(nmax - nused < nadd) {
                noadl::sprintf_and_throw<length_error>("linear_buffer: max size exceeded (`%llu` + `%llu` > `%llu`)",
                                                       static_cast<unsigned long long>(nused), static_cast<unsigned long long>(nadd),
                                                       static_cast<unsigned long long>(nmax));
              }
              auto cap_new = (nused + nadd) | 0x100;
              auto ptr_new = allocator_traits<allocator_type>::allocate(this->as_allocator(), cap_new);
              auto pbuf_new = noadl::unfancy(ptr_new);
#ifdef ROCKET_DEBUG
              traits_type::assign(pbuf_new, cap_new, value_type(0xA5A5A5A5));
#endif
              // Copy the old string into the new buffer and deallocate the old one.
              if(ROCKET_UNEXPECT(nused)) {
                traits_type::copy(pbuf_new, pbuf + goff, nused);
              }
              if(ROCKET_UNEXPECT(pbuf)) {
#ifdef ROCKET_DEBUG
                traits_type::assign(pbuf, this->m_cap, value_type(0xE9E9E9E9));
#endif
                allocator_traits<allocator_type>::deallocate(this->as_allocator(), this->m_ptr, this->m_cap);
              }
              pbuf = pbuf_new;
              // Set up the new buffer.
              this->m_ptr = noadl::move(ptr_new);
              this->m_cap = cap_new;
            }
            // In either case, the string has been moved to the beginning of the buffer.
            ROCKET_ASSERT(nused + nadd <= this->m_cap);
            return nused;
          }
        void deallocate() noexcept
          {
            // If the pointer is null we assume the capacity is zero.
            auto ptr = ::std::exchange(this->m_ptr, storage_pointer());
            if(!ptr) {
              return;
            }
            // Deallocate the buffer and reset stream offsets.
            allocator_traits<allocator_type>::deallocate(this->as_allocator(), ptr, this->m_cap);
            this->m_cap = 0;
          }
        void exchange_with(basic_storage& other) noexcept
          {
            xswap(this->m_ptr, other.m_ptr);
            xswap(this->m_cap, other.m_cap);
          }
      };

    }

template<typename charT, typename traitsT, typename allocT>
    class basic_linear_buffer
  {
    static_assert(!is_array<charT>::value, "`charT` must not be an array type.");
    static_assert(is_trivial<charT>::value, "`charT` must be a trivial type.");
    static_assert(is_same<typename allocT::value_type, charT>::value,
                  "`allocT::value_type` must denote the same type as `charT`.");

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
    explicit constexpr basic_linear_buffer(const allocator_type& alloc) noexcept
      :
        m_stor(alloc)
      {
      }
    basic_linear_buffer(const basic_linear_buffer& other)
      :
        m_stor(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_stor.as_allocator()))
      {
        // Copy the contents from `other`.
        this->putn(other.data(), other.size());
      }
    basic_linear_buffer(const basic_linear_buffer& other, const allocator_type& alloc)
      :
        m_stor(alloc)
      {
        // Copy the contents from `other`.
        this->putn(other.data(), other.size());
      }
    basic_linear_buffer(basic_linear_buffer&& other) noexcept
      :
        m_stor(noadl::move(other.m_stor.as_allocator()))
      {
        // Steal the buffer from `other`.
        this->do_exchange_with(other);
      }
    basic_linear_buffer(basic_linear_buffer&& other, const allocator_type& alloc)
                    noexcept(is_std_allocator<allocator_type>::value /* TODO: is_always_equal */)
      :
        m_stor(alloc)
      {
        if(ROCKET_EXPECT(this->m_stor.as_allocator() == alloc)) {
          // If the allocators compare equal, they can deallocate memory allocated by each other.
          this->do_exchange_with(other);
        }
        else {
          // Otherwise, we have to copy its contents instead.
          this->putn(other.data(), other.size());
        }
      }
    constexpr basic_linear_buffer() noexcept(is_nothrow_constructible<allocator_type>::value)
      :
        basic_linear_buffer(allocator_type())
      {
      }
    basic_linear_buffer& operator=(const basic_linear_buffer& other)
      {
        if(ROCKET_EXPECT(this->m_stor.as_allocator() == other.m_stor.as_allocator())) {
          // If the allocators compare equal, they can deallocate memory allocated by each other.
          noadl::propagate_allocator_on_copy(this->m_stor.as_allocator(), other.m_stor.as_allocator());
          if(ROCKET_EXPECT(this != &other))
            this->do_assign(other);
        }
        else if(allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value) {
          // Otherwise, if the allocator in `*this` will be replaced anyway, the old buffer must be deallocated first.
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
    basic_linear_buffer& operator=(basic_linear_buffer&& other)
                    noexcept(is_std_allocator<allocator_type>::value /* TODO: is_always_equal */ ||
                             allocator_traits<allocator_type>::propagate_on_container_move_assignment::value)
      {
        if(ROCKET_EXPECT(this->m_stor.as_allocator() == other.m_stor.as_allocator())) {
          // If the allocators compare equal, they can deallocate memory allocated by each other.
          noadl::propagate_allocator_on_move(this->m_stor.as_allocator(), other.m_stor.as_allocator());
          ROCKET_ASSERT(this != &other);
          this->do_exchange_with(other);
        }
        else if(allocator_traits<allocator_type>::propagate_on_container_move_assignment::value) {
          // Otherwise, if the allocator in `*this` will be replaced anyway, the old buffer must be deallocated first.
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

  private:
    void do_deallocate() noexcept
      {
        this->m_stor.deallocate();
        this->m_goff = 0;
        this->m_eoff = 0;
      }
    void do_exchange_with(basic_linear_buffer& other) noexcept
      {
        this->m_stor.exchange_with(other.m_stor);
        xswap(this->m_goff, other.m_goff);
        xswap(this->m_eoff, other.m_eoff);
      }

    void do_assign(const basic_linear_buffer& other)
      {
        ROCKET_ASSERT(this != &other);
        this->clear();
        this->putn(other.data(), other.size());
      }

  public:
    constexpr const allocator_type& get_allocator() const noexcept
      {
        return this->m_stor.as_allocator();
      }
    allocator_type& get_allocator() noexcept
      {
        return this->m_stor.as_allocator();
      }

    bool empty() const noexcept
      {
        return this->m_goff == this->m_eoff;
      }
    size_type size() const noexcept
      {
        return this->m_eoff - this->m_goff;
      }
    difference_type ssize() const noexcept
      {
        return static_cast<difference_type>(this->m_eoff - this->m_goff);
      }
    size_type max_size() const noexcept
      {
        return this->m_stor.max_size();
      }
    size_type capacity() const noexcept
      {
        return this->m_stor.capacity();
      }
    basic_linear_buffer& clear() noexcept
      {
        this->m_goff = 0;
        this->m_eoff = 0;
        return *this;
      }

    // read functions
    const value_type* begin() const noexcept
      {
        return this->m_stor.data() + this->m_goff;
      }
    const value_type* end() const noexcept
      {
        return this->m_stor.data() + this->m_eoff;
      }
    const value_type* data() const noexcept
      {
        return this->m_stor.data() + this->m_goff;
      }
    basic_linear_buffer& discard(size_type nbump) noexcept
      {
        ROCKET_ASSERT(nbump <= this->m_eoff - this->m_goff);
#ifdef ROCKET_DEBUG
        traits_type::assign(this->mut_begin(), nbump, value_type(0xC7C7C7C7));
#endif
        this->m_goff += nbump;
        return *this;
      }

    size_type peekn(value_type* s, size_type n) const noexcept
      {
        auto nread = noadl::min(n, this->size());
        if(nread == 0) {
          return 0;
        }
        traits_type::copy(s, this->begin(), nread);
        return nread;
      }
    size_type getn(value_type* s, size_type n) noexcept
      {
        auto nread = noadl::min(n, this->size());
        if(nread == 0) {
          return 0;
        }
        traits_type::copy(s, this->begin(), nread);
        this->discard(nread);
        return nread;
      }
    int_type peekc() const noexcept
      {
        value_type s[1];
        if(this->peekn(s, 1) == 0) {
          return traits_type::eof();
        }
        return traits_type::to_int_type(s[0]);
      }
    int_type getc() noexcept
      {
        value_type s[1];
        if(this->getn(s, 1) == 0) {
          return traits_type::eof();
        }
        return traits_type::to_int_type(s[0]);
      }

    // write functions
    value_type* mut_begin() noexcept
      {
        return this->m_stor.mut_data() + this->m_goff;
      }
    value_type* mut_end() noexcept
      {
        return this->m_stor.mut_data() + this->m_eoff;
      }
    value_type* mut_data() noexcept
      {
        return this->m_stor.mut_data() + this->m_goff;
      }
    size_type reserve(size_type nbump)
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
    basic_linear_buffer& accept(size_type nbump) noexcept
      {
        ROCKET_ASSERT(nbump <= this->m_stor.capacity() - this->m_eoff);
        this->m_eoff += nbump;
        return *this;
      }

    basic_linear_buffer& putn(const value_type* s, size_type n)
      {
        this->reserve(n);
        traits_type::copy(this->mut_end(), s, n);
        this->accept(n);
        return *this;
      }
    basic_linear_buffer& putc(value_type c)
      {
        this->putn(::std::addressof(c), 1);
        return *this;
      }

    basic_linear_buffer& swap(basic_linear_buffer& other)
                    noexcept(is_std_allocator<allocator_type>::value /* TODO: is_always_equal */ ||
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
  };

template<typename charT, typename traitsT, typename allocT>
    inline void swap(basic_linear_buffer<charT, traitsT, allocT>& lhs,
                     basic_linear_buffer<charT, traitsT, allocT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

extern template class basic_linear_buffer<char>;
extern template class basic_linear_buffer<wchar_t>;

using linear_buffer   = basic_linear_buffer<char>;
using linear_wbuffer  = basic_linear_buffer<wchar_t>;

}  // namespace rocket

#endif
