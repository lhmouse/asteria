// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFCNT_PTR_HPP_
#define ROCKET_REFCNT_PTR_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "reference_counter.hpp"

namespace rocket {

template<typename elementT, typename deleterT = default_delete<const elementT>> class refcnt_base;
template<typename elementT> class refcnt_ptr;
template<typename charT, typename traitsT> class basic_tinyfmt;

#include "details/refcnt_ptr.tcc"

template<typename elementT, typename deleterT>
    class refcnt_base : public virtual details_refcnt_ptr::reference_counter_base,
                        private virtual allocator_wrapper_base_for<deleterT>::type
  {
  public:
    using element_type  = elementT;
    using deleter_type  = deleterT;
    using pointer       = elementT*;

  private:
    using deleter_base  = typename allocator_wrapper_base_for<deleter_type>::type;

  protected:
    [[noreturn]] ROCKET_NOINLINE void do_throw_bad_cast(const type_info& to, const type_info& from) const
      {
        noadl::sprintf_and_throw<domain_error>("refcnt_base: bad dynamic cast to type `%s` from type `%s`",
                                               to.name(), from.name());
      }

  public:
    const deleter_type& as_deleter() const noexcept
      {
        return static_cast<const deleter_base&>(*this);
      }
    deleter_type& as_deleter() noexcept
      {
        return static_cast<deleter_base&>(*this);
      }

    bool unique() const noexcept
      {
        return this->details_refcnt_ptr::reference_counter_base::unique();
      }
    long use_count() const noexcept
      {
        return this->details_refcnt_ptr::reference_counter_base::use_count();
      }

    void add_reference() const noexcept
      {
        return this->details_refcnt_ptr::reference_counter_base::add_reference();
      }
    bool drop_reference() const noexcept
      {
        return this->details_refcnt_ptr::reference_counter_base::drop_reference();
      }

    template<typename yelementT = elementT> refcnt_ptr<const yelementT> share_this() const
      {
        refcnt_ptr<const yelementT> dptr(noadl::static_or_dynamic_cast<const yelementT*>(this));
        if(!dptr)
          this->do_throw_bad_cast(typeid(yelementT), typeid(*this));
        dptr->details_refcnt_ptr::reference_counter_base::add_reference();
        return dptr;
      }
    template<typename yelementT = elementT> refcnt_ptr<yelementT> share_this()
      {
        refcnt_ptr<yelementT> dptr(noadl::static_or_dynamic_cast<yelementT*>(this));
        if(!dptr)
          this->do_throw_bad_cast(typeid(yelementT), typeid(*this));
        dptr->details_refcnt_ptr::reference_counter_base::add_reference();
        return dptr;
      }
  };

template<typename elementT> class refcnt_ptr
  {
    static_assert(!is_array<elementT>::value, "invalid element type");
    template<typename> friend class refcnt_ptr;

  public:
    using element_type  = elementT;
    using pointer       = elementT*;

  private:
    details_refcnt_ptr::stored_pointer<element_type> m_sth;

  public:
    constexpr refcnt_ptr(nullptr_t = nullptr) noexcept
      :
        m_sth()
      {
      }
    explicit refcnt_ptr(pointer ptr) noexcept
      :
        refcnt_ptr()
      {
        this->reset(ptr);
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer, pointer>::value)>
        refcnt_ptr(const refcnt_ptr<yelementT>& other) noexcept
      :
        refcnt_ptr()
      {
        this->reset(other.m_sth.fork());
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer, pointer>::value)>
        refcnt_ptr(refcnt_ptr<yelementT>&& other) noexcept
      :
        refcnt_ptr()
      {
        this->reset(other.m_sth.release());
      }
    refcnt_ptr(const refcnt_ptr& other) noexcept
      :
        refcnt_ptr()
      {
        this->reset(other.m_sth.fork());
      }
    refcnt_ptr(refcnt_ptr&& other) noexcept
      :
        refcnt_ptr()
      {
        this->reset(other.m_sth.release());
      }
    refcnt_ptr& operator=(nullptr_t) noexcept
      {
        this->reset();
        return *this;
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer, pointer>::value)>
        refcnt_ptr& operator=(const refcnt_ptr<yelementT>& other) noexcept
      {
        this->reset(other.m_sth.fork());
        return *this;
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer, pointer>::value)>
        refcnt_ptr& operator=(refcnt_ptr<yelementT>&& other) noexcept
      {
        this->reset(other.m_sth.release());
        return *this;
      }
    refcnt_ptr& operator=(const refcnt_ptr& other) noexcept
      {
        this->reset(other.m_sth.fork());
        return *this;
      }
    refcnt_ptr& operator=(refcnt_ptr&& other) noexcept
      {
        this->reset(other.m_sth.release());
        return *this;
      }

  public:
    // 23.11.1.2.4, observers
    bool unique() const noexcept
      {
        return this->m_sth.unique();
      }
    long use_count() const noexcept
      {
        return this->m_sth.use_count();
      }
    constexpr pointer get() const noexcept
      {
        return this->m_sth.get();
      }
    typename add_lvalue_reference<element_type>::type operator*() const
      {
        auto ptr = this->get();
        ROCKET_ASSERT(ptr);
        return *ptr;
      }
    pointer operator->() const noexcept
      {
        auto ptr = this->get();
        ROCKET_ASSERT(ptr);
        return ptr;
      }
    explicit constexpr operator bool () const noexcept
      {
        return bool(this->get());
      }
    constexpr operator pointer () const noexcept
      {
        return this->get();
      }

    // 23.11.1.2.5, modifiers
    pointer release() noexcept
      {
        return this->m_sth.release();
      }
    refcnt_ptr& reset(pointer ptr = pointer()) noexcept
      {
        return this->m_sth.reset(ptr), *this;
      }

    refcnt_ptr& swap(refcnt_ptr& other) noexcept
      {
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }
  };

template<typename xelementT, typename yelementT>
    constexpr bool operator==(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs) noexcept
  {
    return lhs.get() == rhs.get();
  }
template<typename xelementT, typename yelementT>
    constexpr bool operator!=(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs) noexcept
  {
    return lhs.get() != rhs.get();
  }
template<typename xelementT, typename yelementT>
    constexpr bool operator<(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs)
  {
    return lhs.get() < rhs.get();
  }
template<typename xelementT, typename yelementT>
    constexpr bool operator>(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs)
  {
    return lhs.get() > rhs.get();
  }
template<typename xelementT, typename yelementT>
    constexpr bool operator<=(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs)
  {
    return lhs.get() <= rhs.get();
  }
template<typename xelementT, typename yelementT>
    constexpr bool operator>=(const refcnt_ptr<xelementT>& lhs, const refcnt_ptr<yelementT>& rhs)
  {
    return lhs.get() >= rhs.get();
  }

template<typename elementT>
    constexpr bool operator==(const refcnt_ptr<elementT>& lhs, nullptr_t) noexcept
  {
    return +!lhs;
  }
template<typename elementT>
    constexpr bool operator!=(const refcnt_ptr<elementT>& lhs, nullptr_t) noexcept
  {
    return !!lhs;
  }

template<typename elementT>
    constexpr bool operator==(nullptr_t, const refcnt_ptr<elementT>& rhs) noexcept
  {
    return +!rhs;
  }
template<typename elementT>
    constexpr bool operator!=(nullptr_t, const refcnt_ptr<elementT>& rhs) noexcept
  {
    return !!rhs;
  }

template<typename elementT>
    inline void swap(refcnt_ptr<elementT>& lhs,
                     refcnt_ptr<elementT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

template<typename targetT, typename sourceT>
    inline refcnt_ptr<targetT> static_pointer_cast(const refcnt_ptr<sourceT>& sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_aux<targetT>(sptr,
                               [](sourceT* ptr) { return static_cast<targetT*>(ptr);  });
  }
template<typename targetT, typename sourceT>
     inline refcnt_ptr<targetT> dynamic_pointer_cast(const refcnt_ptr<sourceT>& sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_aux<targetT>(sptr,
                               [](sourceT* ptr) { return dynamic_cast<targetT*>(ptr);  });
  }
template<typename targetT, typename sourceT>
    inline refcnt_ptr<targetT> const_pointer_cast(const refcnt_ptr<sourceT>& sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_aux<targetT>(sptr,
                               [](sourceT* ptr) { return const_cast<targetT*>(ptr);  });
  }

template<typename targetT, typename sourceT>
    inline refcnt_ptr<targetT> static_pointer_cast(refcnt_ptr<sourceT>&& sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_aux<targetT>(noadl::move(sptr),
                               [](sourceT* ptr) { return static_cast<targetT*>(ptr);  });
  }
template<typename targetT, typename sourceT>
    inline refcnt_ptr<targetT> dynamic_pointer_cast(refcnt_ptr<sourceT>&& sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_aux<targetT>(noadl::move(sptr),
                               [](sourceT* ptr) { return dynamic_cast<targetT*>(ptr);  });
  }
template<typename targetT, typename sourceT>
    inline refcnt_ptr<targetT> const_pointer_cast(refcnt_ptr<sourceT>&& sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_aux<targetT>(noadl::move(sptr),
                               [](sourceT* ptr) { return const_cast<targetT*>(ptr);  });
  }

template<typename charT, typename traitsT, typename elementT>
    inline basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt,
                                                     const refcnt_ptr<elementT>& rhs)
  {
    return fmt << rhs.get();
  }

template<typename elementT, typename... paramsT>
    inline refcnt_ptr<elementT> make_refcnt(paramsT&&... params)
  {
    return refcnt_ptr<elementT>(new elementT(noadl::forward<paramsT>(params)...));
  }

}  // namespace rocket

#endif
