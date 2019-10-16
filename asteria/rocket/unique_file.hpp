// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_FILE_HPP_
#define ROCKET_UNIQUE_FILE_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "allocator_utilities.hpp"
#include "utilities.hpp"
#include "char_traits.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>,
         typename allocT = allocator<charT>> class basic_unique_file;

    namespace details_unique_file {

    template<typename allocT, typename traitsT>
        class basic_smart_buffer : private allocator_wrapper_base_for<allocT>::type
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
        storage_pointer m_sptr;  // pointer to allocated storage
        size_type m_bpos;  // beginning of buffered data
        size_type m_epos;  // end of buffered data
        size_type m_send;  // size of allocated storage

      public:
        explicit constexpr basic_smart_buffer(const allocator_type& alloc) noexcept
          :
            allocator_base(alloc),
            m_sptr(), m_bpos(), m_epos(), m_send()
          { }
        explicit constexpr basic_smart_buffer(allocator_type&& alloc) noexcept
          :
            allocator_base(noadl::move(alloc)),
            m_sptr(), m_bpos(), m_epos(), m_send()
          { }
        ~basic_smart_buffer()
          {
            this->deallocate();
          }

        basic_smart_buffer(const basic_smart_buffer&)
          = delete;
        basic_smart_buffer& operator=(const basic_smart_buffer&)
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
            return noadl::unfancy(this->m_sptr) + this->m_bpos;
          }
        value_type* data() noexcept
          {
            return noadl::unfancy(this->m_sptr) + this->m_bpos;
          }
        size_type size() const noexcept
          {
            return this->m_epos - this->m_bpos;
          }

        void clear() noexcept
          {
            // Reposition to the beginning.
            this->m_bpos = 0;
            this->m_epos = 0;
          }
        void deallocate() noexcept
          {
            // If the pointer is null we assume all offsets are zero.
            auto sptr = ::std::exchange(this->m_sptr, storage_pointer());
            if(!sptr) {
              return;
            }
            // Deallocate the buffer and reset stream offsets.
            allocator_traits<allocator_type>::deallocate(this->as_allocator(), sptr, this->m_send);
            this->m_bpos = 0;
            this->m_epos = 0;
            this->m_send = 0;
          }
        void exchange_with(basic_smart_buffer& other) noexcept
          {
            noadl::adl_swap(this->m_sptr, other.m_sptr);
            noadl::adl_swap(this->m_bpos, other.m_bpos);
            noadl::adl_swap(this->m_epos, other.m_epos);
            noadl::adl_swap(this->m_send, other.m_send);
          }

        value_type* append(const value_type* sadd, size_type nadd)
          {
            // Get the insertion area.
            value_type* aptr;
            if(ROCKET_EXPECT(nadd <= this->m_send - this->m_epos)) {
              // Set `aptr` to the end when there is enough room.
              aptr = noadl::unfancy(this->m_sptr) + this->m_epos;
            }
            else {
              size_type nused = this->m_epos - this->m_bpos;
              if(ROCKET_EXPECT(nadd <= this->m_send - nused)) {
                // Move the string forwards to make some room in the end.
                aptr = noadl::unfancy(this->m_sptr);
                traits_type::move(aptr, aptr + this->m_bpos, nused);
              }
              else {
                // Allocate a new buffer large enough for the string.
                size_type nmax = allocator_traits<allocator_type>::max_size(this->as_allocator());
                if(nmax - nused < nadd) {
                  noadl::sprintf_and_throw<length_error>("basic_smart_buffer: max size exceeded (`%llu` + `%llu` > `%llu`)",
                                                         static_cast<unsigned long long>(nused), static_cast<unsigned long long>(nadd),
                                                         static_cast<unsigned long long>(nmax));
                }
                auto send = (nused + nadd) | BUFSIZ;
                auto sptr = allocator_traits<allocator_type>::allocate(this->as_allocator(), send);
                aptr = noadl::unfancy(sptr);
                if(this->m_sptr) {
                  // Move the old string into the new buffer and deallocate the old buffer.
                  traits_type::copy(aptr, noadl::unfancy(this->m_sptr) + this->m_bpos, nused);
                  allocator_traits<allocator_type>::deallocate(this->as_allocator(), this->m_sptr, this->m_send);
                }
                this->m_sptr = noadl::move(sptr);
                this->m_send = send;
              }
              // Set up new offsets.
              this->m_bpos = 0;
              this->m_epos = nused;
              aptr += nused;
            }
            // Copy the new string to the area denoted by `sadd`.
            ROCKET_ASSERT(nadd <= this->m_send - this->m_epos);
            traits_type::copy(aptr, sadd, nadd);
            this->m_epos += nadd;
            // Return a pointer to the copy of `sadd`.
            return aptr;
          }
        value_type* consume(size_type nbump) noexcept
          {
            // Update the beginning position only.
            ROCKET_ASSERT(nbump <= this->m_epos - this->m_bpos);
            this->m_bpos += nbump;
            // Return a pointer to the new beginning.
            value_type* aptr = noadl::unfancy(this->m_sptr) + this->m_bpos;
            return aptr;
          }
      };

    class smart_handle
      {
      public:
        using file_handle  = ::std::FILE*;
        using closer_type  = int (*)(::std::FILE*);

      private:
        file_handle m_fp;  // file pointer
        closer_type m_cl;  // closer callback (if it is null then the file is not closed)

      public:
        constexpr smart_handle() noexcept
          :
            m_fp(), m_cl()
          { }
        ~smart_handle()
          {
            this->reset(nullptr, nullptr);
          }

        smart_handle(const smart_handle&)
          = delete;
        smart_handle& operator=(const smart_handle&)
          = delete;

      public:
        /*constexpr*/ file_handle get() const noexcept
          {
            return this->m_fp;
          }
        file_handle release() noexcept
          {
            return ::std::exchange(this->m_fp, nullptr);
          }
        void reset(file_handle fp, const closer_type& cl) noexcept
          {
            auto fp_old = ::std::exchange(this->m_fp, fp);
            auto cl_old = ::std::exchange(this->m_cl, cl);
            if(!fp_old || !cl_old) {
              return;
            }
            (*cl_old)(fp_old);
          }
        void exchange_with(smart_handle& other) noexcept
          {
            noadl::adl_swap(this->m_fp, other.m_fp);
            noadl::adl_swap(this->m_cl, other.m_cl);
          }
      };

    }

template<typename charT, typename traitsT, typename allocT> class basic_unique_file
  {
    static_assert(!is_array<charT>::value, "`charT` must not be an array type.");
    static_assert(is_trivial<charT>::value, "`charT` must be a trivial type.");
    static_assert(is_same<typename allocT::value_type, charT>::value,
                  "`allocT::value_type` must denote the same type as `charT`.");

  public:
    using value_type      = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocT;

    using smart_buffer  = details_unique_file::basic_smart_buffer<allocator_type, traits_type>;
    using smart_handle  = details_unique_file::smart_handle;

    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;
    using file_handle      = typename smart_handle::file_handle;
    using closer_type      = typename smart_handle::closer_type;

  private:
    smart_buffer m_buf;
    smart_handle m_fp;

  public:
    constexpr basic_unique_file(smart_handle&& fp, const allocator_type& alloc = allocator_type()) noexcept
      :
        m_buf(alloc), m_fp(noadl::move(fp))
      { }
    explicit constexpr basic_unique_file(const allocator_type& alloc) noexcept
      :
        basic_unique_file(smart_handle(), alloc)
      { }
    constexpr basic_unique_file() noexcept(is_nothrow_constructible<allocator_type>::value)
      :
        basic_unique_file(allocator_type())
      { }
    basic_unique_file(file_handle fp, const closer_type& cl, const allocator_type& alloc = allocator_type()) noexcept
      :
        basic_unique_file(alloc)
      {
        this->reset(fp, cl);
      }
    basic_unique_file(const char* path, const char* mode, const allocator_type& alloc = allocator_type())
      :
        basic_unique_file(alloc)
      {
        this->open(path, mode);
      }
    basic_unique_file(basic_unique_file&& other) noexcept
      :
        basic_unique_file(noadl::move(other.m_buf.as_allocator()))
      {
        this->m_buf.exchange_with(other.m_buf);
        this->m_fp.exchange_with(other.m_fp);
      }
    basic_unique_file(basic_unique_file&& other, const allocator_type& alloc) /* noexcept(TODO: is_always_equal) */
      :
        basic_unique_file(alloc)
      {
        // Compare the nasty allocators, ...
        if(ROCKET_EXPECT(this->m_buf.as_allocator() == other.m_buf.as_allocator())) {
          // 0) if they are equal, they can deallocate memory allocated by each other.
          // Steal the buffer from `other`.
          this->m_buf.exchange_with(other.m_buf);
        }
        else {
          // 1) if they are unequal, the buffer can't be stolen because `*this` doesn't know how to deallocate it.
          // Copy buffered contents from `other`, which is subject to exceptions.
          this->m_buf.append(other.m_buf.data(), other.m_buf.size());
        }
        this->m_fp.exchange_with(other.m_fp);
      }
    basic_unique_file& operator=(basic_unique_file&& other) /* noexcept(TODO: is_always_equal) */
      {
        // Compare the nasty allocators, ...
        if(ROCKET_EXPECT(this->m_buf.as_allocator() == other.m_buf.as_allocator())) {
          // 0) if they are equal, they can deallocate memory allocated by each other.
          // Replace the allocator and steal the buffer from `other`.
          noadl::propagate_allocator_on_move(this->m_buf.as_allocator(), noadl::move(other.m_buf.as_allocator()));
          this->m_buf.exchange_with(other.m_buf);
        }
        else if(allocator_traits<allocT>::propagate_on_container_move_assignment::value) {
          // 1) if the allocator will be replaced, we can steal the buffer, but don't pass the old buffer back to `other`.
          // Deallocate the old buffer.
          this->m_buf.deallocate();
          // Replace the allocator and steal the buffer from `other`.
          noadl::propagate_allocator_on_move(this->m_buf.as_allocator(), noadl::move(other.m_buf.as_allocator()));
          this->m_buf.exchange_with(other.m_buf);
        }
        else {
          // 2) if the allocator doesn't equal the one from `other` and will not be replaced, we continue to use it.
          // Copy buffered contents from `other`, which is subject to exceptions.
          this->m_buf.clear();
          this->m_buf.append(other.m_buf.data(), other.m_buf.size());
        }
        this->m_fp.exchange_with(other.m_fp);
        return *this;
      }

  public:
    constexpr file_handle get_handle() const noexcept
      {
        return this->m_fp.get();
      }
    constexpr operator bool () const noexcept
      {
        return bool(this->m_fp.get());
      }

    // N.B. The return type differs from `std::basic_string`.
    const allocator_type& get_allocator() const noexcept
      {
        return this->m_sth.as_allocator();
      }
    allocator_type& get_allocator() noexcept
      {
        return this->m_sth.as_allocator();
      }

    void swap(basic_unique_file& other) noexcept
      {
        // Be advised that, as standard containers, if the allocators compare unequal, the behavior is undefined.
        ROCKET_ASSERT(this->m_buf.as_allocator() == other.m_buf.as_allocator());
        noadl::propagate_allocator_on_swap(this->m_buf.as_allocator(), noadl::move(other.m_buf.as_allocator()));
        this->m_buf.exchange_with(other.m_buf);
        this->m_fp.exchange_with(other.m_fp);
      }
  };

template<typename charT, typename traitsT, typename allocT>
    inline void swap(basic_unique_file<charT, traitsT, allocT>& lhs,
                     basic_unique_file<charT, traitsT, allocT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    return lhs.swap(rhs);
  }

template class basic_unique_file<char>;
template class basic_unique_file<wchar_t>;

using unique_file   = basic_unique_file<char>;
using unique_wfile  = basic_unique_file<wchar_t>;

}  // namespace rocket

#endif
