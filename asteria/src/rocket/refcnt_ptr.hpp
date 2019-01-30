// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFCNT_PTR_HPP_
#define ROCKET_REFCNT_PTR_HPP_

#include "compatibility.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "reference_counter.hpp"

namespace rocket {

template<typename elementT, typename deleterT = default_delete<const elementT>>
 class refcnt_base;

template<typename elementT>
 class refcnt_ptr;

    namespace details_refcnt_ptr {

    template<typename resultT, typename sourceT, typename = void>
     struct static_cast_or_dynamic_cast_helper
      {
        constexpr resultT operator()(sourceT &&src) const
          {
            return dynamic_cast<resultT>(::std::forward<sourceT>(src));
          }
      };
    template<typename resultT, typename sourceT>
     struct static_cast_or_dynamic_cast_helper<resultT, sourceT, typename make_void<decltype(static_cast<resultT>(::std::declval<sourceT>()))>::type>
      {
        constexpr resultT operator()(sourceT &&src) const
          {
            return static_cast<resultT>(::std::forward<sourceT>(src));
          }
      };

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

    template<typename elementT>
     class stored_pointer
      {
      public:
        using element_type  = elementT;
        using pointer       = element_type *;

      private:
        template<typename yelementT, typename deleterT>
         static constexpr const deleterT & do_locate_deleter(const refcnt_base<yelementT, deleterT> &base) noexcept
          {
            return base.as_deleter();
          }

      private:
        pointer m_ptr;

      public:
        constexpr stored_pointer() noexcept
          : m_ptr()
          {
          }
        ~stored_pointer()
          {
            this->reset(pointer());
          }

        stored_pointer(const stored_pointer &)
          = delete;
        stored_pointer & operator=(const stored_pointer &)
          = delete;

      public:
        bool unique() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(!ptr) {
              return false;
            }
            return ptr->reference_counter_base::unique();
          }
        long use_count() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            return ptr->reference_counter_base::use_count();
          }
        constexpr pointer get() const noexcept
          {
            return this->m_ptr;
          }
        pointer copy_release() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(ptr) {
              ptr->reference_counter_base::add_reference();
            }
            return ptr;
          }
        pointer release() noexcept
          {
            const auto ptr = this->m_ptr;
            if(ptr) {
              this->m_ptr = pointer();
            }
            return ptr;
          }
        void reset(pointer ptr_new) noexcept
          {
            const auto ptr = noadl::exchange(this->m_ptr, ptr_new);
            if(!ptr) {
              return;
            }
            if(!ptr->reference_counter_base::drop_reference()) {
              return;
            }
            // Copy-construct a deleter from the object, which is used to delete the object thereafter.
            auto tdel = stored_pointer::do_locate_deleter(*ptr);
            ::std::move(tdel)(ptr);
          }
        void exchange(stored_pointer &other) noexcept
          {
            ::std::swap(this->m_ptr, other.m_ptr);
          }
      };

    struct static_caster
      {
        template<typename resultT, typename sourceT>
         static constexpr resultT do_cast(sourceT &&src)
          {
            return static_cast<resultT>(::std::forward<sourceT>(src));
          }
      };
    struct dynamic_caster
      {
        template<typename resultT, typename sourceT>
         static constexpr resultT do_cast(sourceT &&src)
          {
            return dynamic_cast<resultT>(::std::forward<sourceT>(src));
          }
      };
    struct const_caster
      {
        template<typename resultT, typename sourceT>
         static constexpr resultT do_cast(sourceT &&src)
          {
            return const_cast<resultT>(::std::forward<sourceT>(src));
          }
      };

    template<typename resultptrT, typename casterT>
     struct pointer_cast_helper
      {
        template<typename sourceptrT>
         resultptrT operator()(sourceptrT &&sptr) const
          {
            const auto ptr = casterT::template do_cast<typename resultptrT::pointer>(sptr.get());
            if(!ptr) {
              return nullptr;
            }
            auto tptr = ::std::forward<sourceptrT>(sptr);
            tptr.release();
            return resultptrT(ptr);
          }
      };

    }

template<typename elementT, typename deleterT>
 class refcnt_base : protected virtual details_refcnt_ptr::reference_counter_base, private virtual allocator_wrapper_base_for<deleterT>::type
  {
    template<typename>
     friend class details_refcnt_ptr::stored_pointer;

  public:
    using element_type  = elementT;
    using deleter_type  = deleterT;
    using pointer       = elementT *;

  protected:
    using deleter_base  = typename allocator_wrapper_base_for<deleter_type>::type;

  private:
    [[noreturn]] ROCKET_NOINLINE void do_throw_bad_cast(const type_info &ytype) const
      {
        noadl::throw_domain_error("refcnt_base: The current object cannot be converted to type `%s`, whose most derived type is `%s`.",
                                  ytype.name(), typeid(*this).name());
      }

  public:
    const deleter_type & as_deleter() const noexcept
      {
        return static_cast<const deleter_base &>(*this);
      }
    deleter_type & as_deleter() noexcept
      {
        return static_cast<deleter_base &>(*this);
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

    template<typename yelementT = elementT>
     refcnt_ptr<const yelementT> share_this() const
      {
        const auto ptr = details_refcnt_ptr::static_cast_or_dynamic_cast_helper<const yelementT *, const refcnt_base *>(this);
        if(!ptr) {
          this->do_throw_bad_cast(typeid(yelementT));
        }
        this->details_refcnt_ptr::reference_counter_base::add_reference();
        return refcnt_ptr<const yelementT>(ptr);
      }
    template<typename yelementT = elementT>
     refcnt_ptr<yelementT> share_this()
      {
        const auto ptr = details_refcnt_ptr::static_cast_or_dynamic_cast_helper<yelementT *, refcnt_base *>(this);
        if(!ptr) {
          this->do_throw_bad_cast(typeid(yelementT));
        }
        this->details_refcnt_ptr::reference_counter_base::add_reference();
        return refcnt_ptr<yelementT>(ptr);
      }
  };

template<typename elementT>
 class refcnt_ptr
  {
    static_assert(!is_array<elementT>::value, "`elementT` must not be an array type.");

    template<typename>
     friend class refcnt_ptr;

  public:
    using element_type  = elementT;
    using pointer       = elementT *;

  private:
    details_refcnt_ptr::stored_pointer<element_type> m_sth;

  public:
    constexpr refcnt_ptr(nullptr_t = nullptr) noexcept
      : m_sth()
      {
      }
    explicit refcnt_ptr(pointer ptr) noexcept
      : refcnt_ptr()
      {
        this->reset(ptr);
      }
    refcnt_ptr(const refcnt_ptr &other) noexcept
      : refcnt_ptr()
      {
        this->reset(other.m_sth.copy_release());
      }
    refcnt_ptr(refcnt_ptr &&other) noexcept
      : refcnt_ptr()
      {
        this->reset(other.m_sth.release());
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer, pointer>::value)>
     refcnt_ptr(const refcnt_ptr<yelementT> &other) noexcept
      : refcnt_ptr()
      {
        this->reset(other.m_sth.copy_release());
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer, pointer>::value)>
     refcnt_ptr(refcnt_ptr<yelementT> &&other) noexcept
      : refcnt_ptr()
      {
        this->reset(other.m_sth.release());
      }
    refcnt_ptr & operator=(nullptr_t) noexcept
      {
        this->reset();
        return *this;
      }
    refcnt_ptr & operator=(const refcnt_ptr &other) noexcept
      {
        this->reset(other.m_sth.copy_release());
        return *this;
      }
    refcnt_ptr & operator=(refcnt_ptr &&other) noexcept
      {
        this->reset(other.m_sth.release());
        return *this;
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer, pointer>::value)>
     refcnt_ptr & operator=(const refcnt_ptr<yelementT> &other) noexcept
      {
        this->reset(other.m_sth.copy_release());
        return *this;
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_ptr<yelementT>::pointer, pointer>::value)>
     refcnt_ptr & operator=(refcnt_ptr<yelementT> &&other) noexcept
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
        const auto ptr = this->get();
        ROCKET_ASSERT(ptr);
        return *ptr;
      }
    pointer operator->() const noexcept
      {
        const auto ptr = this->get();
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
        const auto ptr = this->m_sth.release();
        return ptr;
      }
    void reset(pointer ptr = pointer()) noexcept
      {
        this->m_sth.reset(ptr);
      }

    void swap(refcnt_ptr &other) noexcept
      {
        this->m_sth.exchange(other.m_sth);
      }
  };

template<typename xelementT, typename yelementT>
 inline bool operator==(const refcnt_ptr<xelementT> &lhs, const refcnt_ptr<yelementT> &rhs) noexcept
  {
    return lhs.get() == rhs.get();
  }
template<typename xelementT, typename yelementT>
 inline bool operator!=(const refcnt_ptr<xelementT> &lhs, const refcnt_ptr<yelementT> &rhs) noexcept
  {
    return lhs.get() != rhs.get();
  }
template<typename xelementT, typename yelementT>
 inline bool operator<(const refcnt_ptr<xelementT> &lhs, const refcnt_ptr<yelementT> &rhs)
  {
    return lhs.get() < rhs.get();
  }
template<typename xelementT, typename yelementT>
 inline bool operator>(const refcnt_ptr<xelementT> &lhs, const refcnt_ptr<yelementT> &rhs)
  {
    return lhs.get() > rhs.get();
  }
template<typename xelementT, typename yelementT>
 inline bool operator<=(const refcnt_ptr<xelementT> &lhs, const refcnt_ptr<yelementT> &rhs)
  {
    return lhs.get() <= rhs.get();
  }
template<typename xelementT, typename yelementT>
 inline bool operator>=(const refcnt_ptr<xelementT> &lhs, const refcnt_ptr<yelementT> &rhs)
  {
    return lhs.get() >= rhs.get();
  }

template<typename elementT>
 inline bool operator==(const refcnt_ptr<elementT> &lhs, nullptr_t) noexcept
  {
    return +!(lhs.get());
  }
template<typename elementT>
 inline bool operator!=(const refcnt_ptr<elementT> &lhs, nullptr_t) noexcept
  {
    return bool(lhs.get());
  }

template<typename elementT>
 inline bool operator==(nullptr_t, const refcnt_ptr<elementT> &rhs) noexcept
  {
    return +!(rhs.get());
  }
template<typename elementT>
 inline bool operator!=(nullptr_t, const refcnt_ptr<elementT> &rhs) noexcept
  {
    return bool(rhs.get());
  }

template<typename elementT>
 inline void swap(refcnt_ptr<elementT> &lhs, refcnt_ptr<elementT> &rhs) noexcept
  {
    lhs.swap(rhs);
  }

template<typename charT, typename traitsT, typename elementT>
 inline basic_ostream<charT, traitsT> & operator<<(basic_ostream<charT, traitsT> &os, const refcnt_ptr<elementT> &rhs)
  {
    return os << rhs.get();
  }

template<typename resultT, typename sourceT>
 inline refcnt_ptr<resultT> static_pointer_cast(const refcnt_ptr<sourceT> &sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_helper<refcnt_ptr<resultT>, details_refcnt_ptr::static_caster>()(sptr);
  }
template<typename resultT, typename sourceT>
 inline refcnt_ptr<resultT> dynamic_pointer_cast(const refcnt_ptr<sourceT> &sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_helper<refcnt_ptr<resultT>, details_refcnt_ptr::dynamic_caster>()(sptr);
  }
template<typename resultT, typename sourceT>
 inline refcnt_ptr<resultT> const_pointer_cast(const refcnt_ptr<sourceT> &sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_helper<refcnt_ptr<resultT>, details_refcnt_ptr::const_caster>()(sptr);
  }

template<typename resultT, typename sourceT>
 inline refcnt_ptr<resultT> static_pointer_cast(refcnt_ptr<sourceT> &&sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_helper<refcnt_ptr<resultT>, details_refcnt_ptr::static_caster>()(::std::move(sptr));
  }
template<typename resultT, typename sourceT>
 inline refcnt_ptr<resultT> dynamic_pointer_cast(refcnt_ptr<sourceT> &&sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_helper<refcnt_ptr<resultT>, details_refcnt_ptr::dynamic_caster>()(::std::move(sptr));
  }
template<typename resultT, typename sourceT>
 inline refcnt_ptr<resultT> const_pointer_cast(refcnt_ptr<sourceT> &&sptr) noexcept
  {
    return details_refcnt_ptr::pointer_cast_helper<refcnt_ptr<resultT>, details_refcnt_ptr::const_caster>()(::std::move(sptr));
  }

template<typename elementT, typename ...paramsT>
 inline refcnt_ptr<elementT> make_refcnt(paramsT &&...params)
  {
    return refcnt_ptr<elementT>(new elementT(::std::forward<paramsT>(params)...));
  }

}

#endif
