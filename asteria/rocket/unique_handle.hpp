// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_HANDLE_HPP_
#define ROCKET_UNIQUE_HANDLE_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "allocator_utilities.hpp"
#include "utilities.hpp"
#include "tinyfmt.hpp"

namespace rocket {

template<typename handleT, typename closerT> class unique_handle;
class std_file_closer;

/* Requirements:
 * 1. Handles must be trivial types other than arrays.
 * 2. Closers shall be copy-constructible.
 *    The following operations are required, all of which, as well as copy/move construction/assignment and swap, shall not throw exceptions.
 *    1) `Closer().null()` returns a handle value called the 'null handle value'.
 *    2) `Closer().is_null()` returns `true` if the argument is a 'null handle value' and `false` otherwise.
 *       N.B. There could more than one null handle values. It is required that `Closer().is_null(Closer().null())` is always `true`.
 *    3) `Closer().close(hv)` closes the handle `hv`. Null handle values will not be passed to this function.
 */

    namespace details_unique_handle {

    template<typename handleT, typename closerT>
        class stored_handle : private allocator_wrapper_base_for<closerT>::type
      {
      public:
        using handle_type  = handleT;
        using closer_type  = closerT;

      private:
        using closer_base = typename allocator_wrapper_base_for<closerT>::type;

      private:
        handle_type m_hv;

      public:
        constexpr stored_handle() noexcept(is_nothrow_constructible<closer_type>::value)
          :
            closer_base(),
            m_hv(this->as_closer().null())
          {
          }
        explicit constexpr stored_handle(const closer_type& cl) noexcept
          :
            closer_base(cl),
            m_hv(this->as_closer().null())
          {
          }
        explicit constexpr stored_handle(closer_type&& cl) noexcept
          :
            closer_base(noadl::move(cl)),
            m_hv(this->as_closer().null())
          {
          }
        ~stored_handle()
          {
            this->reset(this->as_closer().null());
          }

        stored_handle(const stored_handle&)
          = delete;
        stored_handle& operator=(const stored_handle&)
          = delete;

      public:
        const closer_type& as_closer() const noexcept
          {
            return static_cast<const closer_base&>(*this);
          }
        closer_type& as_closer() noexcept
          {
            return static_cast<closer_base&>(*this);
          }

        constexpr handle_type get() const noexcept
          {
            return this->m_hv;
          }
        handle_type release() noexcept
          {
            return ::std::exchange(this->m_hv, this->as_closer().null());
          }
        void reset(handle_type hv_new) noexcept
          {
            auto hv_old = ::std::exchange(this->m_hv, hv_new);
            if(!this->as_closer().is_null(hv_old))
              this->as_closer().close(hv_old);
          }
        void exchange_with(stored_handle& other) noexcept
          {
            xswap(this->m_hv, other.m_hv);
          }
      };

    }  // namespace details_unique_handle

template<typename handleT, typename closerT> class unique_handle
  {
    static_assert(!is_array<handleT>::value, "`handleT` must not be an array type.");
    static_assert(is_trivial<handleT>::value, "`handleT` must be a trivial type.");

  public:
    using handle_type  = handleT;
    using closer_type  = closerT;

  private:
    details_unique_handle::stored_handle<handle_type, closer_type> m_sth;

  public:
    // 23.11.1.2.1, constructors
    constexpr unique_handle() noexcept(is_nothrow_constructible<closer_type>::value)
      :
        m_sth()
      {
      }
    explicit constexpr unique_handle(const closer_type& cl) noexcept
      :
        m_sth(cl)
      {
      }
    explicit unique_handle(handle_type hv) noexcept(is_nothrow_constructible<closer_type>::value)
      :
        unique_handle()
      {
        this->reset(hv);
      }
    unique_handle(handle_type hv, const closer_type& cl) noexcept
      :
        unique_handle(cl)
      {
        this->reset(hv);
      }
    unique_handle(unique_handle&& other) noexcept
      :
        unique_handle(noadl::move(other.m_sth.as_closer()))
      {
        this->reset(other.m_sth.release());
      }
    unique_handle(unique_handle&& other, const closer_type& cl) noexcept
      :
        unique_handle(cl)
      {
        this->reset(other.m_sth.release());
      }
    // 23.11.1.2.3, assignment
    unique_handle& operator=(unique_handle&& other) noexcept
      {
        this->m_sth.as_closer() = noadl::move(other.m_sth.as_closer());
        this->reset(other.m_sth.release());
        return *this;
      }

  public:
    // 23.11.1.2.4, observers
    constexpr handle_type get() const noexcept
      {
        return this->m_sth.get();
      }
    constexpr const closer_type& get_closer() const noexcept
      {
        return this->m_sth.as_closer();
      }
    closer_type& get_closer() noexcept
      {
        return this->m_sth.as_closer();
      }
    explicit constexpr operator bool () const noexcept
      {
        return !(this->m_sth.as_closer().is_null(this->m_sth.get()));
      }
    constexpr operator handle_type () const noexcept
      {
        return this->m_sth.get();
      }

    // 23.11.1.2.5, modifiers
    handle_type release() noexcept
      {
        return this->m_sth.release();
      }
    // N.B. The return type differs from `std::unique_ptr`.
    unique_handle& reset() noexcept
      {
        this->m_sth.reset(this->m_sth.as_closer().null());
        return *this;
      }
    unique_handle& reset(handle_type hv_new) noexcept
      {
        this->m_sth.reset(hv_new);
        return *this;
      }

    unique_handle& swap(unique_handle& other) noexcept
      {
        xswap(this->m_sth.as_closer(), other.m_sth.as_closer());
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }
  };

template<typename handleT, typename closerT>
    constexpr bool operator==(const unique_handle<handleT, closerT>& lhs,
                              const unique_handle<handleT, closerT>& rhs)
  {
    return lhs.get() == rhs.get();
  }
template<typename handleT, typename closerT>
    constexpr bool operator!=(const unique_handle<handleT, closerT>& lhs,
                              const unique_handle<handleT, closerT>& rhs)
  {
    return lhs.get() != rhs.get();
  }
template<typename handleT, typename closerT>
    constexpr bool operator<(const unique_handle<handleT, closerT>& lhs,
                             const unique_handle<handleT, closerT>& rhs)
  {
    return lhs.get() < rhs.get();
  }
template<typename handleT, typename closerT>
    constexpr bool operator>(const unique_handle<handleT, closerT>& lhs,
                             const unique_handle<handleT, closerT>& rhs)
  {
    return lhs.get() > rhs.get();
  }
template<typename handleT, typename closerT>
    constexpr bool operator<=(const unique_handle<handleT, closerT>& lhs,
                              const unique_handle<handleT, closerT>& rhs)
  {
    return lhs.get() <= rhs.get();
  }
template<typename handleT, typename closerT>
    constexpr bool operator>=(const unique_handle<handleT, closerT>& lhs,
                              const unique_handle<handleT, closerT>& rhs)
  {
    return lhs.get() >= rhs.get();
  }

template<typename handleT, typename closerT>
    inline void swap(unique_handle<handleT, closerT>& lhs,
                     unique_handle<handleT, closerT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

template<typename charT, typename traitsT, typename handleT, typename closerT>
    inline basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt,
                                                     const unique_handle<handleT, closerT>& rhs)
  {
    return fmt << rhs.get();
  }

class std_file_closer
  {
  public:
    using file_handle      = ::FILE*;
    using closer_function  = int (::FILE*);

  private:
    closer_function* m_cl;

  public:
    constexpr std_file_closer(closer_function* cl) noexcept
      :
        m_cl(cl)
      {
      }

  public:
    constexpr file_handle null() const noexcept
      {
        return nullptr;
      }
    constexpr bool is_null(file_handle fp) const noexcept
      {
        return fp == nullptr;
      }
    void close(file_handle fp) const noexcept
      {
        if(this->m_cl)
          (*(this->m_cl))(fp);
      }
  };

using unique_std_file  = unique_handle<std_file_closer>;

}  // namespace rocket

#endif
