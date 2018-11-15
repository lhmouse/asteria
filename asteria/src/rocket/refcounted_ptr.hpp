// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFCOUNTED_PTR_HPP_
#define ROCKET_REFCOUNTED_PTR_HPP_

#include <memory> // std::default_delete<>
#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::declval()
#include <iosfwd> // std::basic_ostream<>
#include <cstddef> // std::nullptr_t
#include <typeinfo>
#include "compatibility.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "reference_counter.hpp"

namespace rocket {

using ::std::default_delete;
using ::std::enable_if;
using ::std::is_nothrow_constructible;
using ::std::is_convertible;
using ::std::is_array;
using ::std::add_lvalue_reference;
using ::std::basic_ostream;
using ::std::nullptr_t;

template<typename elementT, typename deleterT = default_delete<const elementT>>
  class refcounted_base;

template<typename elementT>
  class refcounted_ptr;

    namespace details_refcounted_ptr {

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

    template<typename elementT>
      class stored_pointer;

    class refcount_base
      {
      private:
        mutable reference_counter<long> m_nref;

      public:
        bool unique() const noexcept
          {
            return this->m_nref.get() == 1;
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
          static constexpr const deleterT & do_locate_deleter(const refcounted_base<yelementT, deleterT> &base) noexcept
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
        long use_count() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            return ptr->refcount_base::use_count();
          }
        constexpr pointer get() const noexcept
          {
            return this->m_ptr;
          }
        pointer copy_release() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(ptr) {
              ptr->refcount_base::add_reference();
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
            if(!ptr->refcount_base::drop_reference()) {
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
  class refcounted_base : protected virtual details_refcounted_ptr::refcount_base, private virtual allocator_wrapper_base_for<deleterT>::type
  {
    template<typename>
      friend class details_refcounted_ptr::stored_pointer;

  public:
    using element_type  = elementT;
    using deleter_type  = deleterT;

  protected:
    using refcount_base  = details_refcounted_ptr::refcount_base;
    using deleter_base   = typename allocator_wrapper_base_for<deleter_type>::type;

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
        return this->refcount_base::unique();
      }
    long use_count() const noexcept
      {
        return this->refcount_base::use_count();
      }

    void add_reference() const noexcept
      {
        return this->refcount_base::add_reference();
      }
    bool drop_reference() const noexcept
      {
        return this->refcount_base::drop_reference();
      }

    template<typename yelementT = elementT>
      refcounted_ptr<const yelementT> share_this() const
      {
        const auto ptr = details_refcounted_ptr::static_cast_or_dynamic_cast_helper<const yelementT *, const refcounted_base *>(this);
        if(!ptr) {
          noadl::throw_domain_error("refcounted_base: The current object cannot be converted to type `%s`, whose most derived type is `%s`.",
                                    typeid(yelementT).name(), typeid(*this).name());
        }
        this->refcount_base::add_reference();
        return refcounted_ptr<const yelementT>(ptr);
      }
    template<typename yelementT = elementT>
      refcounted_ptr<yelementT> share_this()
      {
        const auto ptr = details_refcounted_ptr::static_cast_or_dynamic_cast_helper<yelementT *, refcounted_base *>(this);
        if(!ptr) {
          noadl::throw_domain_error("refcounted_base: The current object cannot be converted to type `%s`, whose most derived type is `%s`.",
                                    typeid(yelementT).name(), typeid(*this).name());
        }
        this->refcount_base::add_reference();
        return refcounted_ptr<yelementT>(ptr);
      }
  };

template<typename elementT>
  class refcounted_ptr
  {
    static_assert(!is_array<elementT>::value, "`elementT` must not be an array type.");

    template<typename>
      friend class refcounted_ptr;

  public:
    using element_type  = elementT;
    using pointer       = element_type *;

  private:
    details_refcounted_ptr::stored_pointer<element_type> m_sth;

  public:
    constexpr refcounted_ptr(nullptr_t = nullptr) noexcept
      : m_sth()
      {
      }
    explicit refcounted_ptr(pointer ptr) noexcept
      : refcounted_ptr()
      {
        this->reset(ptr);
      }
    refcounted_ptr(const refcounted_ptr &other) noexcept
      : refcounted_ptr()
      {
        this->reset(other.m_sth.copy_release());
      }
    refcounted_ptr(refcounted_ptr &&other) noexcept
      : refcounted_ptr()
      {
        this->reset(other.m_sth.release());
      }
    template<typename yelementT,
      typename enable_if<is_convertible<typename refcounted_ptr<yelementT>::pointer, pointer>::value>::type * = nullptr>
        refcounted_ptr(const refcounted_ptr<yelementT> &other) noexcept
      : refcounted_ptr()
      {
        this->reset(other.m_sth.copy_release());
      }
    template<typename yelementT,
      typename enable_if<is_convertible<typename refcounted_ptr<yelementT>::pointer, pointer>::value>::type * = nullptr>
        refcounted_ptr(refcounted_ptr<yelementT> &&other) noexcept
      : refcounted_ptr()
      {
        this->reset(other.m_sth.release());
      }
    refcounted_ptr & operator=(nullptr_t) noexcept
      {
        this->reset();
        return *this;
      }
    refcounted_ptr & operator=(const refcounted_ptr &other) noexcept
      {
        this->reset(other.m_sth.copy_release());
        return *this;
      }
    refcounted_ptr & operator=(refcounted_ptr &&other) noexcept
      {
        this->reset(other.m_sth.release());
        return *this;
      }
    template<typename yelementT,
      typename enable_if<is_convertible<typename refcounted_ptr<yelementT>::pointer, pointer>::value>::type * = nullptr>
        refcounted_ptr & operator=(const refcounted_ptr<yelementT> &other) noexcept
      {
        this->reset(other.m_sth.copy_release());
        return *this;
      }
    template<typename yelementT,
      typename enable_if<is_convertible<typename refcounted_ptr<yelementT>::pointer, pointer>::value>::type * = nullptr>
        refcounted_ptr & operator=(refcounted_ptr<yelementT> &&other) noexcept
      {
        this->reset(other.m_sth.release());
        return *this;
      }

  public:
    // 23.11.1.2.4, observers
    bool unique() const noexcept
      {
        return this->m_sth.use_count() == 1;
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
        return !!(this->get());
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

    void swap(refcounted_ptr &other) noexcept
      {
        this->m_sth.exchange(other.m_sth);
      }
  };

template<typename xelementT, typename yelementT>
  inline bool operator==(const refcounted_ptr<xelementT> &lhs, const refcounted_ptr<yelementT> &rhs) noexcept
  {
    return lhs.get() == rhs.get();
  }
template<typename xelementT, typename yelementT>
  inline bool operator!=(const refcounted_ptr<xelementT> &lhs, const refcounted_ptr<yelementT> &rhs) noexcept
  {
    return lhs.get() != rhs.get();
  }
template<typename xelementT, typename yelementT>
  inline bool operator<(const refcounted_ptr<xelementT> &lhs, const refcounted_ptr<yelementT> &rhs)
  {
    return lhs.get() < rhs.get();
  }
template<typename xelementT, typename yelementT>
  inline bool operator>(const refcounted_ptr<xelementT> &lhs, const refcounted_ptr<yelementT> &rhs)
  {
    return lhs.get() > rhs.get();
  }
template<typename xelementT, typename yelementT>
  inline bool operator<=(const refcounted_ptr<xelementT> &lhs, const refcounted_ptr<yelementT> &rhs)
  {
    return lhs.get() <= rhs.get();
  }
template<typename xelementT, typename yelementT>
  inline bool operator>=(const refcounted_ptr<xelementT> &lhs, const refcounted_ptr<yelementT> &rhs)
  {
    return lhs.get() >= rhs.get();
  }

template<typename elementT>
  inline bool operator==(const refcounted_ptr<elementT> &lhs, nullptr_t) noexcept
  {
    return !(lhs.get());
  }
template<typename elementT>
  inline bool operator!=(const refcounted_ptr<elementT> &lhs, nullptr_t) noexcept
  {
    return !!(lhs.get());
  }

template<typename elementT>
  inline bool operator==(nullptr_t, const refcounted_ptr<elementT> &rhs) noexcept
  {
    return !(rhs.get());
  }
template<typename elementT>
  inline bool operator!=(nullptr_t, const refcounted_ptr<elementT> &rhs) noexcept
  {
    return !!(rhs.get());
  }

template<typename elementT>
  inline void swap(refcounted_ptr<elementT> &lhs, refcounted_ptr<elementT> &rhs) noexcept
  {
    lhs.swap(rhs);
  }

template<typename charT, typename traitsT, typename elementT>
  inline basic_ostream<charT, traitsT> & operator<<(basic_ostream<charT, traitsT> &os, const refcounted_ptr<elementT> &rhs)
  {
    return os << rhs.get();
  }

template<typename resultT, typename sourceT>
  inline refcounted_ptr<resultT> static_pointer_cast(const refcounted_ptr<sourceT> &sptr) noexcept
  {
    return details_refcounted_ptr::pointer_cast_helper<refcounted_ptr<resultT>, details_refcounted_ptr::static_caster>()(sptr);
  }
template<typename resultT, typename sourceT>
  inline refcounted_ptr<resultT> dynamic_pointer_cast(const refcounted_ptr<sourceT> &sptr) noexcept
  {
    return details_refcounted_ptr::pointer_cast_helper<refcounted_ptr<resultT>, details_refcounted_ptr::dynamic_caster>()(sptr);
  }
template<typename resultT, typename sourceT>
  inline refcounted_ptr<resultT> const_pointer_cast(const refcounted_ptr<sourceT> &sptr) noexcept
  {
    return details_refcounted_ptr::pointer_cast_helper<refcounted_ptr<resultT>, details_refcounted_ptr::const_caster>()(sptr);
  }

template<typename resultT, typename sourceT>
  inline refcounted_ptr<resultT> static_pointer_cast(refcounted_ptr<sourceT> &&sptr) noexcept
  {
    return details_refcounted_ptr::pointer_cast_helper<refcounted_ptr<resultT>, details_refcounted_ptr::static_caster>()(::std::move(sptr));
  }
template<typename resultT, typename sourceT>
  inline refcounted_ptr<resultT> dynamic_pointer_cast(refcounted_ptr<sourceT> &&sptr) noexcept
  {
    return details_refcounted_ptr::pointer_cast_helper<refcounted_ptr<resultT>, details_refcounted_ptr::dynamic_caster>()(::std::move(sptr));
  }
template<typename resultT, typename sourceT>
  inline refcounted_ptr<resultT> const_pointer_cast(refcounted_ptr<sourceT> &&sptr) noexcept
  {
    return details_refcounted_ptr::pointer_cast_helper<refcounted_ptr<resultT>, details_refcounted_ptr::const_caster>()(::std::move(sptr));
  }

template<typename elementT, typename ...paramsT>
  inline refcounted_ptr<elementT> make_refcounted(paramsT &&...params)
  {
    return refcounted_ptr<elementT>(new elementT(::std::forward<paramsT>(params)...));
  }

}

#endif
