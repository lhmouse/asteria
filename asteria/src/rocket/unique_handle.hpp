// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_HANDLE_HPP_
#define ROCKET_UNIQUE_HANDLE_HPP_

#include <type_traits> // so many...
#include <utility> // std::move(), std::swap(), std::declval()
#include "compatibility.h"
#include "assert.hpp"
#include "allocator_utilities.hpp"
#include "utilities.hpp"

/* Requirements:
 * 1. Handles must be trivial types other than arrays.
 * 2. Closers shall be copy-constructible and move-constructible.
 *    The following operations are required, all of which, as well as copy/move construction/assignment and swap, shall not throw exceptions.
 *    1) `Closer().null()` returns a handle value called the 'null handle value'.
 *    2) `Closer().is_null()` returns `true` if the argument is a 'null handle value' and `false` otherwise.
 *       N.B. There could more than one null handle values. It is required that `Closer().is_null(Closer().null())` is always `true`.
 *    3) `Closer().close(h)` closes the handle `h`. Null handle values will not be passed to this function.
 */

namespace rocket {

using ::std::enable_if;
using ::std::is_array;
using ::std::is_trivial;
using ::std::is_nothrow_constructible;
using ::std::decay;
using ::std::conditional;
using ::std::false_type;
using ::std::true_type;
using ::std::add_lvalue_reference;

template<typename handleT, typename closerT>
  class unique_handle;

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
        handle_type m_h;

      public:
        constexpr stored_handle() noexcept(is_nothrow_constructible<closer_type>::value)
          : closer_base(),
            m_h(this->as_closer().null())
          {
          }
        explicit constexpr stored_handle(const closer_type &cl) noexcept
          : closer_base(cl),
            m_h(this->as_closer().null())
          {
          }
        explicit constexpr stored_handle(closer_type &&cl) noexcept
          : closer_base(::std::move(cl)),
            m_h(this->as_closer().null())
          {
          }
        ~stored_handle()
          {
            this->reset(this->as_closer().null());
          }

        stored_handle(const stored_handle &)
          = delete;
        stored_handle & operator=(const stored_handle &)
          = delete;

      public:
        const closer_type & as_closer() const noexcept
          {
            return static_cast<const closer_base &>(*this);
          }
        closer_type & as_closer() noexcept
          {
            return static_cast<closer_base &>(*this);
          }

        constexpr handle_type get() const noexcept
          {
            return this->m_h;
          }
        handle_type release() noexcept
          {
            return noadl::exchange(this->m_h, this->as_closer().null());
          }
        void reset(handle_type h_new) noexcept
          {
            const auto h_old = noadl::exchange(this->m_h, h_new);
            if(this->as_closer().is_null(h_old)) {
              return;
            }
            this->as_closer().close(h_old);
          }
        void exchange(stored_handle &other) noexcept
          {
            ::std::swap(this->m_h, other.m_h);
          }
      };

  template<typename ...unusedT>
    struct make_void
      {
        using type = void;
      };

  template<typename handleT, typename = void>
    struct dereference_helper
      {
        using result_type = handleT;

        constexpr result_type operator()(handleT h) const
          {
            return h;
          }
      };
  template<typename pointerT>
    struct dereference_helper<pointerT, typename make_void<decltype(*(::std::declval<pointerT>()))>::type>
      {
        using result_type = decltype(*(::std::declval<pointerT>()));

        constexpr result_type operator()(pointerT p) const
          {
            return *p;
          }
      };

}

template<typename handleT, typename closerT>
  class unique_handle
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
        : m_sth()
        {
        }
      explicit constexpr unique_handle(const closer_type &cl) noexcept
        : m_sth(cl)
        {
        }
      explicit unique_handle(handle_type h) noexcept(is_nothrow_constructible<closer_type>::value)
        : unique_handle()
        {
          this->reset(h);
        }
      unique_handle(handle_type h, const closer_type &cl) noexcept
        : unique_handle(cl)
        {
          this->reset(h);
        }
      unique_handle(unique_handle &&other) noexcept
        : unique_handle(::std::move(other.m_sth.as_closer()))
        {
          this->reset(other.m_sth.release());
        }
      unique_handle(unique_handle &&other, const closer_type &cl) noexcept
        : unique_handle(cl)
        {
          this->reset(other.m_sth.release());
        }
      // 23.11.1.2.3, assignment
      unique_handle & operator=(unique_handle &&other) noexcept
        {
          allocator_move_assigner<closer_type, true>()(this->m_sth.as_closer(), ::std::move(other.m_sth.as_closer()));
          this->reset(other.m_sth.release());
          return *this;
        }

    public:
      // 23.11.1.2.4, observers
      template<typename checkT = handleT>
        constexpr typename details_unique_handle::dereference_helper<handleT>::result_type operator*() const noexcept
          {
            return details_unique_handle::dereference_helper<handleT>()(this->m_sth.get());
          }
      constexpr handle_type operator->() const noexcept
        {
          return this->m_sth.get();
        }
      constexpr handle_type get() const noexcept
        {
          return this->m_sth.get();
        }
      constexpr const closer_type & get_closer() const noexcept
        {
          return this->m_sth.as_closer();
        }
      closer_type & get_closer() noexcept
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
      unique_handle & reset() noexcept
        {
          this->m_sth.reset(this->m_sth.as_closer().null());
          return *this;
        }
      unique_handle & reset(handle_type h_new) noexcept
        {
          this->m_sth.reset(h_new);
          return *this;
        }

      void swap(unique_handle &other) noexcept
        {
          allocator_swapper<closer_type, true>()(this->m_sth.as_closer(), other.m_sth.as_closer());
          this->m_sth.exchange(other.m_sth);
        }
    };

template<typename ...paramsT>
  inline bool operator==(const unique_handle<paramsT...> &lhs, const unique_handle<paramsT...> &rhs) noexcept
    {
      return lhs.get() == rhs.get();
    }
template<typename ...paramsT>
  inline bool operator!=(const unique_handle<paramsT...> &lhs, const unique_handle<paramsT...> &rhs) noexcept
    {
      return lhs.get() != rhs.get();
    }
template<typename ...paramsT>
  inline bool operator<(const unique_handle<paramsT...> &lhs, const unique_handle<paramsT...> &rhs)
    {
      return lhs.get() < rhs.get();
    }
template<typename ...paramsT>
  inline bool operator>(const unique_handle<paramsT...> &lhs, const unique_handle<paramsT...> &rhs)
    {
      return lhs.get() > rhs.get();
    }
template<typename ...paramsT>
  inline bool operator<=(const unique_handle<paramsT...> &lhs, const unique_handle<paramsT...> &rhs)
    {
      return lhs.get() <= rhs.get();
    }
template<typename ...paramsT>
  inline bool operator>=(const unique_handle<paramsT...> &lhs, const unique_handle<paramsT...> &rhs)
    {
      return lhs.get() >= rhs.get();
    }

template<typename ...paramsT>
  inline void swap(unique_handle<paramsT...> &lhs, unique_handle<paramsT...> &rhs) noexcept
    {
      lhs.swap(rhs);
    }

}

#endif
