// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_LINEAR_BUFFER_HPP_
#  error Please include <rocket/linear_buffer.hpp> instead.
#endif

namespace details_linear_buffer {

template<typename allocT, typename traitsT>
class basic_storage
  : private allocator_wrapper_base_for<allocT>::type
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
    explicit constexpr
    basic_storage(const allocator_type& alloc) noexcept
      : allocator_base(alloc)
      { }

    explicit constexpr
    basic_storage(allocator_type&& alloc) noexcept
      : allocator_base(::std::move(alloc))
      { }

    ~basic_storage()
      { this->deallocate();  }

    basic_storage(const basic_storage&)
      = delete;

    basic_storage&
    operator=(const basic_storage&)
      = delete;

  public:
    const allocator_type&
    as_allocator() const noexcept
      { return static_cast<const allocator_base&>(*this);  }

    allocator_type&
    as_allocator() noexcept
      { return static_cast<allocator_base&>(*this);  }

    const value_type*
    data() const noexcept
      { return noadl::unfancy(this->m_ptr);  }

    value_type*
    mut_data() noexcept
      { return noadl::unfancy(this->m_ptr);  }

    size_type
    max_size() const noexcept
      { return allocator_traits<allocator_type>::max_size(*this);  }

    size_type
    capacity() const noexcept
      { return this->m_cap;  }

    ROCKET_NEVER_INLINE size_type
    reserve(size_type goff, size_type eoff, size_type nadd)
      {
        ROCKET_ASSERT(goff <= eoff);
        ROCKET_ASSERT(eoff <= this->m_cap);

        // Get a plain pointer to the beginning to allocated storage.
        auto pbuf = noadl::unfancy(this->m_ptr);

        // Get the number of characters that have been buffered.
        auto nused = eoff - goff;
        if(ROCKET_EXPECT(nadd <= this->m_cap - nused)) {
          // If the buffer is large enough for the combined data, shift the string to
          // the beginning to make some room after its the end.
          traits_type::move(pbuf, pbuf + goff, nused);
        }
        else {
          // If the buffer is not large enough, allocate a new one.
          auto nmax = this->max_size();
          if(nmax - nused < nadd)
            noadl::sprintf_and_throw<length_error>(
                  "linear_buffer: max size exceeded (`%lld` + `%lld` > `%lld`)",
                  static_cast<long long>(nused), static_cast<long long>(nadd),
                  static_cast<long long>(nmax));

          size_type cap_new = (nused + nadd) | 0x1000;
          auto ptr_new = allocator_traits<allocator_type>::allocate(*this, cap_new);
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
            allocator_traits<allocator_type>::deallocate(*this, this->m_ptr, this->m_cap);
          }
          pbuf = pbuf_new;

          // Set up the new buffer.
          this->m_ptr = ::std::move(ptr_new);
          this->m_cap = cap_new;
        }

        // In either case, the string has been moved to the beginning of the buffer.
        ROCKET_ASSERT(nused + nadd <= this->m_cap);
        return nused;
      }

    void
    deallocate() noexcept
      {
        // If the pointer is null we assume the capacity is zero.
        auto ptr = ::std::exchange(this->m_ptr, nullptr);
        if(!ptr)
          return;

        // Deallocate the buffer and reset stream offsets.
        allocator_traits<allocator_type>::deallocate(*this, ptr, this->m_cap);
        this->m_cap = 0;
      }

    void
    exchange_with(basic_storage& other) noexcept
      {
        ::std::swap(this->m_ptr, other.m_ptr);
        ::std::swap(this->m_cap, other.m_cap);
      }
  };

}  // namespace details_linear_buffer
