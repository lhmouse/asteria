// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_PTR_HPP_
#define ROCKET_UNIQUE_PTR_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"
#include "xallocator.hpp"

namespace rocket {

template<typename elementT, typename deleterT = default_delete<const elementT>>
class unique_ptr;

template<typename charT, typename traitsT>
class basic_tinyfmt;

#include "details/unique_ptr.ipp"

template<typename elementT, typename deleterT>
class unique_ptr
  {
    static_assert(!is_array<elementT>::value, "invalid element type");

    template<typename, typename>
    friend class unique_ptr;

  public:
    using element_type  = elementT;
    using deleter_type  = deleterT;
    using pointer       = typename details_unique_ptr::pointer_of<elementT, deleterT>::type;

  private:
    details_unique_ptr::stored_pointer<pointer, deleter_type> m_sth;

  public:
    // 23.11.1.2.1, constructors
    constexpr
    unique_ptr(nullptr_t = nullptr) noexcept(is_nothrow_constructible<deleter_type>::value)
      : m_sth()
      { }

    explicit constexpr
    unique_ptr(const deleter_type& del) noexcept
      : m_sth(nullptr, del)
      { }

    explicit constexpr
    unique_ptr(pointer ptr) noexcept(is_nothrow_constructible<deleter_type>::value)
      : m_sth(ptr)
      { }

    constexpr
    unique_ptr(pointer ptr, const deleter_type& del) noexcept
      : m_sth(ptr, del)
      { }

    template<typename yelementT, typename ydeleterT,
    ROCKET_ENABLE_IF(is_convertible<typename unique_ptr<yelementT, ydeleterT>::pointer,
                                    pointer>::value),
    ROCKET_ENABLE_IF(is_constructible<deleter_type, typename unique_ptr<yelementT,
                                      ydeleterT>::deleter_type&&>::value)>
    unique_ptr(unique_ptr<yelementT, ydeleterT>&& other) noexcept
      : m_sth(other.m_sth.release(), ::std::move(other.m_sth.as_deleter()))
      { }

    unique_ptr(unique_ptr&& other) noexcept
      : m_sth(other.m_sth.release(), ::std::move(other.m_sth.as_deleter()))
      { }

    unique_ptr(unique_ptr&& other, const deleter_type& del) noexcept
      : m_sth(other.m_sth.release(), del)
      { }

    // 23.11.1.2.3, assignment
    unique_ptr&
    operator=(unique_ptr&& other) noexcept
      { this->m_sth.as_deleter() = ::std::move(other.m_sth.as_deleter());
        this->reset(other.m_sth.release());
        return *this;  }

    template<typename yelementT, typename ydeleterT,
    ROCKET_ENABLE_IF(is_convertible<typename unique_ptr<yelementT, ydeleterT>::pointer,
                                    pointer>::value),
    ROCKET_ENABLE_IF(is_assignable<deleter_type&, typename unique_ptr<yelementT,
                                   ydeleterT>::deleter_type&&>::value)>
    unique_ptr&
    operator=(unique_ptr<yelementT, ydeleterT>&& other) noexcept
      { this->m_sth.as_deleter() = ::std::move(other.m_sth.as_deleter());
        this->reset(other.m_sth.release());
        return *this;  }

    unique_ptr&
    swap(unique_ptr& other) noexcept
      { noadl::xswap(this->m_sth.as_deleter(), other.m_sth.as_deleter());
        this->m_sth.exchange_with(other.m_sth);
        return *this;  }

  public:
    // 23.11.1.2.4, observers
    constexpr pointer
    get() const noexcept
      { return this->m_sth.get();  }

    typename add_lvalue_reference<element_type>::type
    operator*() const
      {
        auto ptr = this->get();
        ROCKET_ASSERT(ptr);
        return *ptr;
      }

    pointer
    operator->() const noexcept
      {
        auto ptr = this->get();
        ROCKET_ASSERT(ptr);
        return ptr;
      }

    explicit constexpr operator
    bool() const noexcept
      { return bool(this->get());  }

    constexpr operator
    pointer() const noexcept
      { return this->get();  }

    constexpr const deleter_type&
    get_deleter() const noexcept
      { return this->m_sth.as_deleter();  }

    deleter_type&
    get_deleter() noexcept
      { return this->m_sth.as_deleter();  }

    // 23.11.1.2.5, modifiers
    pointer
    release() noexcept
      { return this->m_sth.release();  }

    // N.B. The return type differs from `std::unique_ptr`.
    unique_ptr&
    reset(pointer ptr_new = nullptr) noexcept
      {
        this->m_sth.reset(::std::move(ptr_new));
        return *this;
      }
  };

template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
constexpr bool
operator==(const unique_ptr<xelementT, xdeleterT>& lhs,
           const unique_ptr<yelementT, ydeleterT>& rhs) noexcept
  { return lhs.get() == rhs.get();  }

template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
constexpr bool
operator!=(const unique_ptr<xelementT, xdeleterT>& lhs,
           const unique_ptr<yelementT, ydeleterT>& rhs) noexcept
  { return lhs.get() != rhs.get();  }

template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
constexpr bool
operator<(const unique_ptr<xelementT, xdeleterT>& lhs,
           const unique_ptr<yelementT, ydeleterT>& rhs)
  { return lhs.get() < rhs.get();  }

template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
constexpr bool
operator>(const unique_ptr<xelementT, xdeleterT>& lhs,
           const unique_ptr<yelementT, ydeleterT>& rhs)
  { return lhs.get() > rhs.get();  }

template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
constexpr bool
operator<=(const unique_ptr<xelementT, xdeleterT>& lhs,
           const unique_ptr<yelementT, ydeleterT>& rhs)
  { return lhs.get() <= rhs.get();  }

template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
constexpr bool
operator>=(const unique_ptr<xelementT, xdeleterT>& lhs,
           const unique_ptr<yelementT, ydeleterT>& rhs)
  { return lhs.get() >= rhs.get();  }

template<typename elementT, typename deleterT>
constexpr bool
operator==(const unique_ptr<elementT, deleterT>& lhs, nullptr_t) noexcept
  { return !lhs;  }

template<typename elementT, typename deleterT>
constexpr bool
operator!=(const unique_ptr<elementT, deleterT>& lhs, nullptr_t) noexcept
  { return !!lhs;  }

template<typename elementT, typename deleterT>
constexpr bool
operator==(nullptr_t, const unique_ptr<elementT, deleterT>& rhs) noexcept
  { return !rhs;  }

template<typename elementT, typename deleterT>
constexpr bool
operator!=(nullptr_t, const unique_ptr<elementT, deleterT>& rhs) noexcept
  { return !!rhs;  }

template<typename elementT, typename deleterT>
inline void
swap(unique_ptr<elementT, deleterT>& lhs, unique_ptr<elementT, deleterT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

template<typename charT, typename traitsT, typename elementT, typename deleterT>
inline basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt,
           const unique_ptr<elementT, deleterT>& rhs)
  { return fmt << rhs.get();  }

template<typename elementT, typename... paramsT>
inline unique_ptr<elementT>
make_unique(paramsT&&... params)
  { return unique_ptr<elementT>(new elementT(::std::forward<paramsT>(params)...));  }

template<typename targetT, typename sourceT>
inline unique_ptr<targetT>
static_pointer_cast(unique_ptr<sourceT>&& sptr) noexcept
  { return details_unique_ptr::pointer_cast_aux<targetT>(::std::move(sptr),
               [](sourceT* ptr) { return static_cast<targetT*>(ptr);  });  }

template<typename targetT, typename sourceT>
inline unique_ptr<targetT>
dynamic_pointer_cast(unique_ptr<sourceT>&& sptr) noexcept
  { return details_unique_ptr::pointer_cast_aux<targetT>(::std::move(sptr),
               [](sourceT* ptr) { return dynamic_cast<targetT*>(ptr);  });  }

template<typename targetT, typename sourceT>
inline unique_ptr<targetT>
const_pointer_cast(unique_ptr<sourceT>&& sptr) noexcept
  { return details_unique_ptr::pointer_cast_aux<targetT>(::std::move(sptr),
               [](sourceT* ptr) { return const_cast<targetT*>(ptr);  });  }

}  // namespace rocket

#endif
