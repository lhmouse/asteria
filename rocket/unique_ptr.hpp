// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_PTR_
#define ROCKET_UNIQUE_PTR_

#include "fwd.hpp"
#include "xassert.hpp"
#include "xthrow.hpp"
#include "xallocator.hpp"
namespace rocket {

template<typename elementT, typename deleterT = default_delete<const elementT>>
class unique_ptr;

template<typename charT>
class basic_tinyfmt;

#include "details/unique_ptr.ipp"

template<typename elementT, typename deleterT>
class unique_ptr
  {
    static_assert(!is_array<elementT>::value, "invalid element type");
    static_assert(!is_reference<elementT>::value, "invalid element type");
    template<typename, typename> friend class unique_ptr;

  public:
    using element_type  = elementT;
    using deleter_type  = deleterT;
    using pointer       = typename details_unique_ptr::pointer_of<elementT, deleterT>::type;

  private:
    details_unique_ptr::stored_pointer<pointer, deleter_type> m_sth;

  public:
    // 23.11.1.2.1, constructors
    ROCKET_CONSTEXPR_INLINE
    unique_ptr(nullptr_t = nullptr)
      noexcept(is_nothrow_constructible<deleter_type>::value)
      :
        m_sth()
      { }

    explicit constexpr
    unique_ptr(const deleter_type& del)
      noexcept
      :
        m_sth(nullptr, del)
      { }

    explicit constexpr
    unique_ptr(pointer ptr)
      noexcept(is_nothrow_constructible<deleter_type>::value)
      :
        m_sth(ptr)
      { }

    constexpr
    unique_ptr(pointer ptr, const deleter_type& del)
      noexcept
      :
        m_sth(ptr, del)
      { }

    template<typename yelementT, typename ydeleterT,
    ROCKET_ENABLE_IF(is_convertible<typename unique_ptr<yelementT,
                                     ydeleterT>::pointer, pointer>::value),
    ROCKET_ENABLE_IF(is_constructible<deleter_type,
             typename unique_ptr<yelementT, ydeleterT>::deleter_type&&>::value)>
    unique_ptr(unique_ptr<yelementT, ydeleterT>&& other)
      noexcept
      :
        m_sth(other.m_sth.release(), move(other.m_sth.as_deleter()))
      { }

    unique_ptr(unique_ptr&& other)
      noexcept
      :
        m_sth(other.m_sth.release(), move(other.m_sth.as_deleter()))
      { }

    unique_ptr(unique_ptr&& other, const deleter_type& del)
      noexcept
      :
        m_sth(other.m_sth.release(), del)
      { }

    // 23.11.1.2.3, assignment
    unique_ptr&
    operator=(unique_ptr&& other)
      & noexcept
      {
        this->m_sth.as_deleter() = move(other.m_sth.as_deleter());
        this->reset(other.m_sth.release());
        return *this;
      }

    template<typename yelementT, typename ydeleterT,
    ROCKET_ENABLE_IF(is_convertible<typename unique_ptr<yelementT,
                                     ydeleterT>::pointer, pointer>::value),
    ROCKET_ENABLE_IF(is_assignable<deleter_type&,
             typename unique_ptr<yelementT, ydeleterT>::deleter_type&&>::value)>
    unique_ptr&
    operator=(unique_ptr<yelementT, ydeleterT>&& other)
      & noexcept
      {
        this->m_sth.as_deleter() = move(other.m_sth.as_deleter());
        this->reset(other.m_sth.release());
        return *this;
      }

    unique_ptr&
    swap(unique_ptr& other)
      noexcept
      {
        noadl::xswap(this->m_sth.as_deleter(), other.m_sth.as_deleter());
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }

  public:
    // 23.11.1.2.4, observers
    ROCKET_PURE constexpr
    pointer
    get()
      const noexcept
      { return this->m_sth.get();  }

    ROCKET_PURE
    typename add_lvalue_reference<element_type>::type
    operator*()
      const
      {
        auto ptr = this->get();
        ROCKET_ASSERT(ptr);
        return *ptr;
      }

    ROCKET_PURE
    pointer
    operator->()
      const noexcept
      {
        auto ptr = this->get();
        ROCKET_ASSERT(ptr);
        return ptr;
      }

    ROCKET_PURE explicit constexpr
    operator bool()
      const noexcept
      { return static_cast<bool>(this->get());  }

    ROCKET_PURE constexpr
    operator pointer()
      const noexcept
      { return this->get();  }

    constexpr
    const deleter_type&
    get_deleter()
      const noexcept
      { return this->m_sth.as_deleter();  }

    deleter_type&
    get_deleter()
      noexcept
      { return this->m_sth.as_deleter();  }

    // 23.11.1.2.5, modifiers
    pointer
    release()
      noexcept
      { return this->m_sth.release();  }

    // N.B. The return type differs from `std::unique_ptr`.
    unique_ptr&
    reset(pointer ptr_new = nullptr)
      noexcept
      {
        this->m_sth.reset(move(ptr_new));
        return *this;
      }
  };

template<typename xelemT, typename xdelT, typename yelemT, typename ydelT>
constexpr
bool
operator==(const unique_ptr<xelemT, xdelT>& lhs, const unique_ptr<yelemT, ydelT>& rhs)
  noexcept
  { return lhs.get() == rhs.get();  }

template<typename xelemT, typename xdelT, typename yelemT, typename ydelT>
constexpr
bool
operator!=(const unique_ptr<xelemT, xdelT>& lhs, const unique_ptr<yelemT, ydelT>& rhs)
  noexcept
  { return lhs.get() != rhs.get();  }

template<typename xelemT, typename xdelT, typename yelemT, typename ydelT>
constexpr
bool
operator<(const unique_ptr<xelemT, xdelT>& lhs, const unique_ptr<yelemT, ydelT>& rhs)
  { return lhs.get() < rhs.get();  }

template<typename xelemT, typename xdelT, typename yelemT, typename ydelT>
constexpr
bool
operator>(const unique_ptr<xelemT, xdelT>& lhs, const unique_ptr<yelemT, ydelT>& rhs)
  { return lhs.get() > rhs.get();  }

template<typename xelemT, typename xdelT, typename yelemT, typename ydelT>
constexpr
bool
operator<=(const unique_ptr<xelemT, xdelT>& lhs, const unique_ptr<yelemT, ydelT>& rhs)
  { return lhs.get() <= rhs.get();  }

template<typename xelemT, typename xdelT, typename yelemT, typename ydelT>
constexpr
bool
operator>=(const unique_ptr<xelemT, xdelT>& lhs, const unique_ptr<yelemT, ydelT>& rhs)
  { return lhs.get() >= rhs.get();  }

template<typename elemT, typename delT>
constexpr
bool
operator==(const unique_ptr<elemT, delT>& lhs, nullptr_t)
  noexcept
  { return !lhs;  }

template<typename elemT, typename delT>
constexpr
bool
operator!=(const unique_ptr<elemT, delT>& lhs, nullptr_t)
  noexcept
  { return !!lhs;  }

template<typename elemT, typename delT>
constexpr
bool
operator==(nullptr_t, const unique_ptr<elemT, delT>& rhs)
  noexcept
  { return !rhs;  }

template<typename elemT, typename delT>
constexpr
bool
operator!=(nullptr_t, const unique_ptr<elemT, delT>& rhs)
  noexcept
  { return !!rhs;  }

template<typename elemT, typename delT>
inline
void
swap(unique_ptr<elemT, delT>& lhs, unique_ptr<elemT, delT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

template<typename charT, typename elementT, typename deleterT>
inline
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const unique_ptr<elementT, deleterT>& rhs)
  { return fmt << rhs.get();  }

template<typename elementT, typename... paramsT>
inline
unique_ptr<elementT>
make_unique(paramsT&&... params)
  { return unique_ptr<elementT>(new elementT(forward<paramsT>(params)...));  }

template<typename targetT, typename sourceT>
ROCKET_ALWAYS_INLINE
unique_ptr<targetT>
static_pointer_cast(unique_ptr<sourceT>&& sptr)
  noexcept
  {
    unique_ptr<targetT> dptr(static_cast<targetT*>(sptr.get()));
    if(dptr)
      sptr.release();
    return dptr;
  }

template<typename targetT, typename sourceT>
ROCKET_ALWAYS_INLINE
unique_ptr<targetT>
dynamic_pointer_cast(unique_ptr<sourceT>&& sptr)
  noexcept
  {
    unique_ptr<targetT> dptr(dynamic_cast<targetT*>(sptr.get()));
    if(dptr)
      sptr.release();
    return dptr;
  }

template<typename targetT, typename sourceT>
ROCKET_ALWAYS_INLINE
unique_ptr<targetT>
const_pointer_cast(unique_ptr<sourceT>&& sptr)
  noexcept
  {
    unique_ptr<targetT> dptr(const_cast<targetT*>(sptr.get()));
    if(dptr)
      sptr.release();
    return dptr;
  }

}  // namespace rocket
#endif
