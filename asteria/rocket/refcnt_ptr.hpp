// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFCNT_PTR_HPP_
#define ROCKET_REFCNT_PTR_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"
#include "reference_counter.hpp"
#include "xallocator.hpp"

namespace rocket {

template<typename elementT,
         typename deleterT = default_delete<const elementT>>
class refcnt_base;

template<typename elementT>
class refcnt_ptr;

template<typename charT, typename traitsT>
class basic_tinyfmt;

#include "details/refcnt_ptr.ipp"

template<typename elementT, typename deleterT>
class refcnt_base
  : public virtual details_refcnt_ptr::refcnt_cJveMKH5bI7L,
    private virtual allocator_wrapper_base_for<deleterT>::type
  {
  public:
    using element_type  = elementT;
    using deleter_type  = deleterT;
    using pointer       = elementT*;

  private:
    using deleter_base  = typename allocator_wrapper_base_for<deleter_type>::type;

  protected:
    [[noreturn]] ROCKET_NEVER_INLINE void
    do_throw_bad_cast(const type_info& to, const type_info& from) const
      {
        noadl::sprintf_and_throw<domain_error>(
              "refcnt_base: bad dynamic cast to type `%s` from type `%s`",
              to.name(), from.name());
      }

  public:
    const deleter_type&
    as_deleter() const noexcept
      { return static_cast<const deleter_base&>(*this);  }

    deleter_type&
    as_deleter() noexcept
      { return static_cast<deleter_base&>(*this);  }

    bool
    unique() const noexcept
      { return this->refcnt_cJveMKH5bI7L::unique();  }

    long
    use_count() const noexcept
      { return this->refcnt_cJveMKH5bI7L::use_count();  }

    long
    add_reference() const noexcept
      { return this->refcnt_cJveMKH5bI7L::add_reference();  }

    long
    drop_reference() const noexcept
      { return this->refcnt_cJveMKH5bI7L::drop_reference();  }

    template<typename yelementT = elementT, typename selfT = refcnt_base>
    refcnt_ptr<const yelementT>
    share_this() const
      {
        auto yptr = noadl::static_or_dynamic_cast<const yelementT*, const selfT*>(this);
        if(!yptr)
          this->do_throw_bad_cast(typeid(yelementT), typeid(*this));

        yptr->refcnt_cJveMKH5bI7L::add_reference();
        return refcnt_ptr<const yelementT>(yptr);
      }

    template<typename yelementT = elementT, typename selfT = refcnt_base>
    refcnt_ptr<yelementT>
    share_this()
      {
        auto yptr = noadl::static_or_dynamic_cast<yelementT*, selfT*>(this);
        if(!yptr)
          this->do_throw_bad_cast(typeid(yelementT), typeid(*this));

        yptr->refcnt_cJveMKH5bI7L::add_reference();
        return refcnt_ptr<yelementT>(yptr);
      }
  };

template<typename elementT>
class refcnt_ptr
  {
    static_assert(!is_array<elementT>::value, "invalid element type");

    template<typename>
    friend class refcnt_ptr;

  public:
    using element_type  = elementT;
    using pointer       = elementT*;

  private:
    details_refcnt_ptr::stored_pointer<element_type> m_sth;

  public:
    constexpr
    refcnt_ptr(nullptr_t = nullptr) noexcept
      : m_sth()
      { }

    explicit constexpr
    refcnt_ptr(pointer ptr) noexcept
      : m_sth(ptr)
      { }

    template<typename yelementT,
    ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer,
                                    pointer>::value)>
    refcnt_ptr(const refcnt_ptr<yelementT>& other) noexcept
      : m_sth(other.m_sth.fork())
      { }

    template<typename yelementT,
    ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer,
                                    pointer>::value)>
    refcnt_ptr(refcnt_ptr<yelementT>&& other) noexcept
      : m_sth(other.m_sth.release())
      { }

    refcnt_ptr(const refcnt_ptr& other) noexcept
      : m_sth(other.m_sth.fork())
      { }

    refcnt_ptr(refcnt_ptr&& other) noexcept
      : m_sth(other.m_sth.release())
      { }

    refcnt_ptr&
    operator=(nullptr_t) noexcept
      { this->reset();
        return *this;  }

    template<typename yelementT,
    ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer,
                                    pointer>::value)>
    refcnt_ptr&
    operator=(const refcnt_ptr<yelementT>& other) noexcept
      { this->reset(other.m_sth.fork());
        return *this;  }

    template<typename yelementT,
    ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer,
                                    pointer>::value)>
    refcnt_ptr&
    operator=(refcnt_ptr<yelementT>&& other) noexcept
      { this->reset(other.m_sth.release());
        return *this;  }

    refcnt_ptr&
    operator=(const refcnt_ptr& other) noexcept
      { this->reset(other.m_sth.fork());
        return *this;  }

    refcnt_ptr&
    operator=(refcnt_ptr&& other) noexcept
      { this->reset(other.m_sth.release());
        return *this;  }

    refcnt_ptr&
    swap(refcnt_ptr& other) noexcept
      { this->m_sth.exchange_with(other.m_sth);
        return *this;  }

  public:
    // 23.11.1.2.4, observers
    bool
    unique() const noexcept
      { return this->m_sth.unique();  }

    long
    use_count() const noexcept
      { return this->m_sth.use_count();  }

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

    // 23.11.1.2.5, modifiers
    pointer
    release() noexcept
      { return this->m_sth.release();  }

    refcnt_ptr&
    reset(pointer ptr = nullptr) noexcept
      {
        this->m_sth.reset(ptr);
        return *this;
      }
  };

template<typename xelementT, typename yelementT>
constexpr bool
operator==(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs) noexcept
  { return lhs.get() == rhs.get();  }

template<typename xelementT, typename yelementT>
constexpr bool
operator!=(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs) noexcept
  { return lhs.get() != rhs.get();  }

template<typename xelementT, typename yelementT>
constexpr bool
operator<(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs)
  { return lhs.get() < rhs.get();  }

template<typename xelementT, typename yelementT>
constexpr bool
operator>(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs)
  { return lhs.get() > rhs.get();  }

template<typename xelementT, typename yelementT>
constexpr bool
operator<=(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs)
  { return lhs.get() <= rhs.get();  }

template<typename xelementT, typename yelementT>
constexpr bool
operator>=(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs)
  { return lhs.get() >= rhs.get();  }

template<typename elementT>
constexpr bool
operator==(const refcnt_ptr<elementT>& lhs, nullptr_t) noexcept
  { return !lhs;  }

template<typename elementT>
constexpr bool
operator!=(const refcnt_ptr<elementT>& lhs, nullptr_t) noexcept
  { return !!lhs;  }

template<typename elementT>
constexpr bool
operator==(nullptr_t, const refcnt_ptr<elementT>& rhs) noexcept
  { return !rhs;  }

template<typename elementT>
constexpr bool
operator!=(nullptr_t, const refcnt_ptr<elementT>& rhs) noexcept
  { return !!rhs;  }

template<typename elementT>
inline void
swap(refcnt_ptr<elementT>& lhs, refcnt_ptr<elementT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

template<typename charT, typename traitsT, typename elementT>
inline basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const refcnt_ptr<elementT>& rhs)
  { return fmt << rhs.get();  }

template<typename elementT, typename... paramsT>
inline refcnt_ptr<elementT>
make_refcnt(paramsT&&... params)
  { return refcnt_ptr<elementT>(new elementT(::std::forward<paramsT>(params)...));  }

template<typename targetT, typename sourceT>
inline refcnt_ptr<targetT>
static_pointer_cast(const refcnt_ptr<sourceT>& sptr) noexcept
  { return details_refcnt_ptr::pointer_cast_aux<targetT>(sptr,
               [](sourceT* ptr) { return static_cast<targetT*>(ptr);  });  }

template<typename targetT, typename sourceT>
inline refcnt_ptr<targetT>
dynamic_pointer_cast(const refcnt_ptr<sourceT>& sptr) noexcept
  { return details_refcnt_ptr::pointer_cast_aux<targetT>(sptr,
               [](sourceT* ptr) { return dynamic_cast<targetT*>(ptr);  });  }

template<typename targetT, typename sourceT>
inline refcnt_ptr<targetT>
const_pointer_cast(const refcnt_ptr<sourceT>& sptr) noexcept
  { return details_refcnt_ptr::pointer_cast_aux<targetT>(sptr,
               [](sourceT* ptr) { return const_cast<targetT*>(ptr);  });  }

template<typename targetT, typename sourceT>
inline refcnt_ptr<targetT>
static_pointer_cast(refcnt_ptr<sourceT>&& sptr) noexcept
  { return details_refcnt_ptr::pointer_cast_aux<targetT>(::std::move(sptr),
               [](sourceT* ptr) { return static_cast<targetT*>(ptr);  });  }

template<typename targetT, typename sourceT>
inline refcnt_ptr<targetT>
dynamic_pointer_cast(refcnt_ptr<sourceT>&& sptr) noexcept
  { return details_refcnt_ptr::pointer_cast_aux<targetT>(::std::move(sptr),
               [](sourceT* ptr) { return dynamic_cast<targetT*>(ptr);  });  }

template<typename targetT, typename sourceT>
inline refcnt_ptr<targetT>
const_pointer_cast(refcnt_ptr<sourceT>&& sptr) noexcept
  { return details_refcnt_ptr::pointer_cast_aux<targetT>(::std::move(sptr),
               [](sourceT* ptr) { return const_cast<targetT*>(ptr);  });  }

}  // namespace rocket

#endif
