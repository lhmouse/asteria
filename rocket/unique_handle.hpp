// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_HANDLE_
#define ROCKET_UNIQUE_HANDLE_

#include "fwd.hpp"
#include "xassert.hpp"
#include "xallocator.hpp"
namespace rocket {

// Requirements:
// 1. Handles must be trivial types other than arrays.
// 2. Closers shall be copy-constructible.
//    The following operations are required, all of which, as well as copy/move
//    construction/assignment and swap,
//    shall not throw exceptions.
//    1) `Closer().null()` returns a handle value called the 'null handle value'.
//    2) `Closer().is_null()` returns `true` if the argument is a 'null handle
//       value' and `false` otherwise.
//       N.B. There could more than one null handle value. It is required that
//            `Closer().is_null(Closer().null())` is always `true`.
//    3) `Closer().close(hv)` closes the handle `hv`. Null handle values will
//       not be passed to this function.
template<typename handleT, typename closerT>
class unique_handle;

template<typename charT>
class basic_tinyfmt;

#include "details/unique_handle.ipp"

template<typename handleT, typename closerT>
class unique_handle
  {
    static_assert(!is_array<handleT>::value, "invalid handle type");
    static_assert(!is_reference<handleT>::value, "invalid handle type");

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
      { }

    explicit constexpr unique_handle(const closer_type& cl) noexcept
      :
        m_sth(cl.null(), cl)
      { }

    explicit constexpr unique_handle(handle_type hv) noexcept(is_nothrow_constructible<closer_type>::value)
      :
        m_sth(move(hv))
      { }

    constexpr unique_handle(handle_type hv, const closer_type& cl) noexcept
      :
        m_sth(move(hv), cl)
      { }

    unique_handle(unique_handle&& other) noexcept
      :
        unique_handle(other.m_sth.release(), move(other.m_sth.as_closer()))
      { }

    unique_handle(unique_handle&& other, const closer_type& cl) noexcept
      :
        unique_handle(other.m_sth.release(), cl)
      { }

    // 23.11.1.2.3, assignment
    unique_handle&
    operator=(unique_handle&& other) & noexcept
      {
        this->m_sth.as_closer() = move(other.m_sth.as_closer());
        this->reset(other.m_sth.release());
        return *this;
      }

    unique_handle&
    swap(unique_handle& other) noexcept
      {
        noadl::xswap(this->m_sth.as_closer(), other.m_sth.as_closer());
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }

  public:
    // 23.11.1.2.4, observers
    constexpr
    handle_type
    get() const noexcept
      { return this->m_sth.get();  }

    constexpr
    const closer_type&
    get_closer() const noexcept
      { return this->m_sth.as_closer();  }

    closer_type&
    get_closer() noexcept
      { return this->m_sth.as_closer();  }

    ROCKET_PURE constexpr
    explicit operator bool() const noexcept
      { return not this->m_sth.as_closer().is_null(this->m_sth.get());  }

    ROCKET_PURE constexpr
    operator const handle_type&() const noexcept
      { return this->m_sth.get();  }

    // 23.11.1.2.5, modifiers
    handle_type
    release() noexcept
      { return this->m_sth.release();  }

    // N.B. The return type differs from `std::unique_ptr`.
    unique_handle&
    reset() noexcept
      {
        this->m_sth.reset(this->m_sth.as_closer().null());
        return *this;
      }

    unique_handle&
    reset(handle_type hv_new) noexcept
      {
        this->m_sth.reset(move(hv_new));
        return *this;
      }

    unique_handle&
    reset(handle_type hv_new, const closer_type& cl) noexcept
      {
        this->m_sth.reset(move(hv_new));
        this->m_sth.as_closer() = cl;
        return *this;
      }
  };

template<typename handT, typename closT>
constexpr
bool
operator==(const unique_handle<handT, closT>& lhs, const unique_handle<handT, closT>& rhs)
  { return lhs.get() == rhs.get();  }

template<typename handT, typename closT>
constexpr
bool
operator!=(const unique_handle<handT, closT>& lhs, const unique_handle<handT, closT>& rhs)
  { return lhs.get() != rhs.get();  }

template<typename handT, typename closT>
constexpr
bool
operator<(const unique_handle<handT, closT>& lhs, const unique_handle<handT, closT>& rhs)
  { return lhs.get() < rhs.get();  }

template<typename handT, typename closT>
constexpr
bool
operator>(const unique_handle<handT, closT>& lhs, const unique_handle<handT, closT>& rhs)
  { return lhs.get() > rhs.get();  }

template<typename handT, typename closT>
constexpr
bool
operator<=(const unique_handle<handT, closT>& lhs, const unique_handle<handT, closT>& rhs)
  { return lhs.get() <= rhs.get();  }

template<typename handT, typename closT>
constexpr
bool
operator>=(const unique_handle<handT, closT>& lhs, const unique_handle<handT, closT>& rhs)
  { return lhs.get() >= rhs.get();  }

template<typename handT, typename closT>
inline
void
swap(unique_handle<handT, closT>& lhs, unique_handle<handT, closT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

template<typename charT, typename handleT, typename closerT>
inline
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const unique_handle<handleT, closerT>& rhs)
  { return fmt << rhs.get();  }

template<typename handleT, typename closerT>
inline
unique_handle<handleT, details_unique_handle::closer_wrapper<handleT, closerT&&>>
make_unique_handle(handleT hv, closerT&& cl) noexcept
  {
    using closer_wrapper = details_unique_handle::closer_wrapper<handleT, closerT&&>;
    return unique_handle<handleT, closer_wrapper>(hv, forward<closerT>(cl));
  }

}  // namespace rocket
#endif
