// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CHECKED_PTR_HPP_
#define ROCKET_CHECKED_PTR_HPP_

#include "utilities.hpp"
#include "throw.hpp"

namespace rocket {

template<typename pointerT> class checked_ptr;
template<typename charT, typename traitsT> class basic_tinyfmt;

template<typename pointerT> class checked_ptr
  {
    template<typename> friend class checked_ptr;

  public:
    using pointer  = pointerT;

  private:
    pointer m_ptr;

  public:
    constexpr checked_ptr(nullptr_t = nullptr) noexcept
      :
        m_ptr()
      {
      }
    template<typename ypointerT, ROCKET_ENABLE_IF(is_constructible<pointer, ypointerT&&>::value),
                                 ROCKET_DISABLE_IF(is_base_of<checked_ptr, typename remove_cvref<ypointerT>::type>::value)>
        constexpr checked_ptr(ypointerT&& ptr) noexcept(is_nothrow_constructible<pointer, ypointerT&&>::value)
      :
        m_ptr(noadl::forward<ypointerT>(ptr))
      {
      }
    template<typename ypointerT, ROCKET_ENABLE_IF(is_constructible<pointer, const ypointerT&>::value)>
        constexpr checked_ptr(const checked_ptr<ypointerT>& other) noexcept(is_nothrow_constructible<pointer, const ypointerT&>::value)
      :
        m_ptr(other.m_ptr)
      {
      }
    template<typename ypointerT, ROCKET_ENABLE_IF(is_constructible<pointer, ypointerT&&>::value)>
        constexpr checked_ptr(checked_ptr<ypointerT>&& other) noexcept(is_nothrow_constructible<pointer, ypointerT&&>::value)
      :
        m_ptr(::std::exchange(other.m_ptr, ypointerT()))
      {
      }
    checked_ptr& operator=(nullptr_t) noexcept
      {
        this->m_ptr = pointer();
        return *this;
      }
    template<typename ypointerT, ROCKET_ENABLE_IF(is_assignable<pointer&, ypointerT&&>::value),
                                 ROCKET_DISABLE_IF(is_base_of<checked_ptr, typename remove_cvref<ypointerT>::type>::value)>
        checked_ptr& operator=(ypointerT&& ptr) noexcept(is_nothrow_assignable<pointer, ypointerT&&>::value)
      {
        this->m_ptr = noadl::forward<ypointerT>(ptr);
        return *this;
      }
    template<typename ypointerT, ROCKET_ENABLE_IF(is_assignable<pointer&, const ypointerT&>::value)>
        checked_ptr& operator=(const checked_ptr<ypointerT>& other) noexcept(is_nothrow_assignable<pointer&, const ypointerT&>::value)
      {
        this->m_ptr = other.m_ptr;
        return *this;
      }
    template<typename ypointerT, ROCKET_ENABLE_IF(is_assignable<pointer&, ypointerT&&>::value)>
        checked_ptr& operator=(checked_ptr<ypointerT>&& other) noexcept(is_nothrow_assignable<pointer&, ypointerT&&>::value)
      {
        this->m_ptr = ::std::exchange(other.m_ptr, ypointerT());
        return *this;
      }

  public:
    explicit constexpr operator bool () const noexcept
      {
        return bool(this->m_ptr);
      }
    constexpr operator const pointer& () const noexcept
      {
        return this->m_ptr;
      }
    operator pointer& () noexcept
      {
        return this->m_ptr;
      }

    constexpr const pointer& get() const noexcept
      {
        return this->m_ptr;
      }
    pointer& get() noexcept
      {
        return this->m_ptr;
      }

    constexpr decltype(*(::std::declval<const pointer&>())) operator*() const
      {
        if(!this->m_ptr) {
          noadl::sprintf_and_throw<invalid_argument>("checked_ptr: attempt to dereference a null `%s`",
                                                     typeid(pointer).name());
        }
        return *(this->m_ptr);
      }
    const pointer& operator->() const
      {
        if(!this->m_ptr) {
          noadl::sprintf_and_throw<invalid_argument>("checked_ptr: attempt to access the member through a null `%s`",
                                                     typeid(pointer).name());
        }
        return this->m_ptr;
      }

    checked_ptr& swap(checked_ptr& other) noexcept
      {
        xswap(this->m_ptr, other.m_ptr);
        return *this;
      }
  };

template<typename pointerT>
    inline void swap(checked_ptr<pointerT>& lhs, checked_ptr<pointerT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

template<typename xpointerT, typename ypointerT>
    constexpr bool operator==(const checked_ptr<xpointerT>& lhs, const checked_ptr<ypointerT>& rhs) noexcept
  {
    return lhs.get() == rhs.get();
  }
template<typename xpointerT, typename ypointerT>
    constexpr bool operator!=(const checked_ptr<xpointerT>& lhs, const checked_ptr<ypointerT>& rhs) noexcept
  {
    return lhs.get() != rhs.get();
  }
template<typename xpointerT, typename ypointerT>
    constexpr bool operator<(const checked_ptr<xpointerT>& lhs, const checked_ptr<ypointerT>& rhs)
  {
    return lhs.get() < rhs.get();
  }
template<typename xpointerT, typename ypointerT>
    constexpr bool operator>(const checked_ptr<xpointerT>& lhs, const checked_ptr<ypointerT>& rhs)
  {
    return lhs.get() > rhs.get();
  }
template<typename xpointerT, typename ypointerT>
    constexpr bool operator<=(const checked_ptr<xpointerT>& lhs, const checked_ptr<ypointerT>& rhs)
  {
    return lhs.get() <= rhs.get();
  }
template<typename xpointerT, typename ypointerT>
    constexpr bool operator>=(const checked_ptr<xpointerT>& lhs, const checked_ptr<ypointerT>& rhs)
  {
    return lhs.get() >= rhs.get();
  }

template<typename pointerT>
    constexpr bool operator==(const checked_ptr<pointerT>& lhs, nullptr_t) noexcept
  {
    return +!lhs;
  }
template<typename pointerT>
    constexpr bool operator!=(const checked_ptr<pointerT>& lhs, nullptr_t) noexcept
  {
    return !!lhs;
  }

template<typename pointerT>
    constexpr bool operator==(nullptr_t, const checked_ptr<pointerT>& rhs) noexcept
  {
    return +!rhs;
  }
template<typename pointerT>
    constexpr bool operator!=(nullptr_t, const checked_ptr<pointerT>& rhs) noexcept
  {
    return !!rhs;
  }

template<typename charT, typename traitsT, typename pointerT>
    inline basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt,
                                                     const checked_ptr<pointerT>& rhs)
  {
    return fmt << rhs.get();
  }


}  // namespace rocket

#endif
