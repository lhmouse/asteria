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
#include "tinyfmt.hpp"

namespace rocket {

template<typename elementT, typename deleterT = default_delete<const elementT>> class refcnt_base;
template<typename elementT> class refcnt_ptr;

    namespace details_refcnt_ptr {

    class reference_counter_base
      {
      private:
        mutable reference_counter<long> m_nref;

      public:
        bool unique() const noexcept
          {
            return this->m_nref.unique();
          }
        long use_count() const noexcept
          {
            return this->m_nref.get();
          }

        void add_reference() const noexcept
          {
            return this->m_nref.increment();
          }
        bool drop_reference() const noexcept
          {
            return this->m_nref.decrement();
          }
      };

    }

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

  private:
    [[noreturn]] ROCKET_NOINLINE void do_throw_bad_cast(const type_info& ytype) const
      {
        noadl::sprintf_and_throw<domain_error>(
          "refcnt_base: The current object cannot be converted to type `%s`, whose most derived type is `%s`.",
          ytype.name(), typeid(*this).name());
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
        auto ptr = noadl::static_or_dynamic_cast<const yelementT*>(this);
        if(!ptr) {
          this->do_throw_bad_cast(typeid(yelementT));
        }
        // Share ownership.
        refcnt_ptr<const yelementT> dptr(ptr);
        this->details_refcnt_ptr::reference_counter_base::add_reference();
        return dptr;
      }
    template<typename yelementT = elementT> refcnt_ptr<yelementT> share_this()
      {
        auto ptr = noadl::static_or_dynamic_cast<yelementT*>(this);
        if(!ptr) {
          this->do_throw_bad_cast(typeid(yelementT));
        }
        // Share ownership.
        refcnt_ptr<yelementT> dptr(ptr);
        this->details_refcnt_ptr::reference_counter_base::add_reference();
        return dptr;
      }
  };

    namespace details_refcnt_ptr {

    template<typename elementT, typename deleterT>
        constexpr deleterT copy_deleter(const refcnt_base<elementT, deleterT>& base) noexcept
      {
        return base.as_deleter();
      }

    template<typename elementT> class stored_pointer
      {
      public:
        using element_type  = elementT;
        using pointer       = element_type*;

      private:
        pointer m_ptr;

      public:
        constexpr stored_pointer() noexcept
          :
            m_ptr()
          {
          }
        ~stored_pointer()
          {
            this->reset(pointer());
#ifdef ROCKET_DEBUG
            if(is_trivially_destructible<pointer>::value)
              ::std::memset(static_cast<void*>(::std::addressof(this->m_ptr)), 0xF6, sizeof(m_ptr));
#endif
          }

        stored_pointer(const stored_pointer&)
          = delete;
        stored_pointer& operator=(const stored_pointer&)
          = delete;

      public:
        bool unique() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return false;
            }
            return ptr->reference_counter_base::unique();
          }
        long use_count() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            return ptr->reference_counter_base::use_count();
          }
        constexpr pointer get() const noexcept
          {
            return this->m_ptr;
          }
        pointer release() noexcept
          {
            return ::std::exchange(this->m_ptr, pointer());
          }
        pointer fork() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return pointer();
            }
            ptr->reference_counter_base::add_reference();
            return ptr;
          }
        void reset(pointer ptr_new) noexcept
          {
            auto ptr = ::std::exchange(this->m_ptr, ptr_new);
            if(!ptr) {
              return;
            }
            if(ROCKET_EXPECT(!ptr->reference_counter_base::drop_reference())) {
              return;
            }
            copy_deleter</*noadl*/>(*ptr)(ptr);
          }
        void exchange_with(stored_pointer& other) noexcept
          {
            noadl::adl_swap(this->m_ptr, other.m_ptr);
          }
      };

    template<typename targetT, typename sourceT, typename casterT>
        refcnt_ptr<targetT> pointer_cast_aux(const refcnt_ptr<sourceT>& sptr, casterT&& caster)
      {
        refcnt_ptr<targetT> dptr;
        // Try casting.
        auto ptr = noadl::forward<casterT>(caster)(sptr.get());
        if(ptr) {
          // Share ownership.
          dptr.reset(ptr);
          ptr->reference_counter_base::add_reference();
        }
        return dptr;
      }
    template<typename targetT, typename sourceT, typename casterT>
        refcnt_ptr<targetT> pointer_cast_aux(refcnt_ptr<sourceT>&& sptr, casterT&& caster)
      {
        refcnt_ptr<targetT> dptr;
        // Try casting.
        auto ptr = noadl::forward<casterT>(caster)(sptr.get());
        if(ptr) {
          // Transfer ownership.
          dptr.reset(ptr);
          sptr.release();
        }
        return dptr;
      }

    }  // namespace details_refcnt_ptr

template<typename elementT> class refcnt_ptr
  {
    static_assert(!is_array<elementT>::value, "`elementT` must not be an array type.");

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

    void swap(refcnt_ptr& other) noexcept
      {
        this->m_sth.exchange_with(other.m_sth);
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
    return lhs.swap(rhs);
  }

template<typename charT, typename traitsT, typename elementT>
    inline basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt,
                                                     const refcnt_ptr<elementT>& rhs)
  {
    return fmt << rhs.get();
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

template<typename elementT, typename... paramsT>
    inline refcnt_ptr<elementT> make_refcnt(paramsT&&... params)
  {
    return refcnt_ptr<elementT>(new elementT(noadl::forward<paramsT>(params)...));
  }

}  // namespace rocket

#endif
