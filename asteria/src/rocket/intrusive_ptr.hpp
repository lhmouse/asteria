/// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_INTRUSIVE_PTR_HPP_
#define ROCKET_INTRUSIVE_PTR_HPP_

#include <atomic> // std::atomic<>
#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::declval()
#include <iosfwd> // std::basic_ostream<>
#include <cstddef> // std::nullptr_t
#include <typeinfo>
#include "compatibility.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"

namespace rocket {

using ::std::atomic;
using ::std::remove_reference;
using ::std::remove_pointer;
using ::std::enable_if;
using ::std::conditional;
using ::std::is_nothrow_constructible;
using ::std::is_convertible;
using ::std::is_array;
using ::std::is_reference;
using ::std::integral_constant;
using ::std::add_lvalue_reference;
using ::std::basic_ostream;
using ::std::nullptr_t;

template<typename elementT>
  class intrusive_base;

template<typename elementT>
  class intrusive_ptr;

namespace details_intrusive_ptr {

  class refcount_base
    {
    private:
      mutable atomic<long> m_nref;

    public:
      constexpr refcount_base() noexcept
        : m_nref(1)
        {
        }
      constexpr refcount_base(const refcount_base &) noexcept
        : refcount_base()
        {
        }
      refcount_base & operator=(const refcount_base &) noexcept
        {
          return *this;
        }
      virtual ~refcount_base();

    public:
      long reference_count() const noexcept
        {
          return this->m_nref.load(::std::memory_order_relaxed);
        }
      bool try_add_reference() const noexcept
        {
          auto nref_old = this->m_nref.load(::std::memory_order_relaxed);
          do {
            if(nref_old == 0) {
              return false;
            }
            if(this->m_nref.compare_exchange_strong(nref_old, nref_old + 1, ::std::memory_order_relaxed)) {
              return true;
            }
          } while(true);
        }
      void add_reference() const noexcept
        {
          auto nref_old = this->m_nref.fetch_add(1, ::std::memory_order_relaxed);
          ROCKET_ASSERT(nref_old >= 1);
        }
      bool drop_reference() const noexcept
        {
          auto nref_old = this->m_nref.fetch_sub(1, ::std::memory_order_acq_rel);
          if(nref_old > 1) {
            return false;
          }
          ROCKET_ASSERT(nref_old == 1);
          return true;
        }
    };

  template<typename ...unusedT>
    struct make_void
      {
        using type = void;
      };

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
    class stored_pointer
      {
      public:
        using element_type  = elementT;
        using pointer       = element_type *;

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
            return ptr->refcount_base::reference_count();
          }
        pointer get() const noexcept
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
            if(ptr->refcount_base::drop_reference() == false) {
              return;
            }
            delete ptr;
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
          resultptrT operator()(sourceptrT &&iptr) const
            {
              const auto ptr = casterT::template do_cast<typename resultptrT::pointer>(iptr.get());
              if(!ptr) {
                return nullptr;
              }
              auto tptr = ::std::forward<sourceptrT>(iptr);
              tptr.release();
              return resultptrT(ptr);
            }
      };

}

template<typename elementT>
  class intrusive_base : protected virtual details_intrusive_ptr::refcount_base
    {
      template<typename, typename>
        friend class details_intrusive_ptr::stored_pointer;

    private:
      template<typename yelementT, typename cvthisT>
        static intrusive_ptr<yelementT> do_share_this(cvthisT *cvthis);

    public:
      ~intrusive_base() override;

    public:
      bool unique() const noexcept
        {
          return this->refcount_base::reference_count() == 1;
        }
      long use_count() const noexcept
        {
          return this->refcount_base::reference_count();
        }

      template<typename yelementT = elementT>
        intrusive_ptr<const yelementT> share_this() const
          {
            return this->do_share_this<const yelementT>(this);
          }
      template<typename yelementT = elementT>
        intrusive_ptr<yelementT> share_this()
          {
            return this->do_share_this<yelementT>(this);
          }
    };

template<typename elementT>
  class intrusive_ptr
    {
      static_assert(is_array<elementT>::value == false, "`elementT` must not be an array type.");

      template<typename>
        friend class intrusive_ptr;

    public:
      using element_type  = elementT;
      using pointer       = element_type *;

    private:
      details_intrusive_ptr::stored_pointer<element_type> m_sth;

    public:
      constexpr intrusive_ptr(nullptr_t = nullptr) noexcept
        : m_sth()
        {
        }
      explicit intrusive_ptr(pointer ptr) noexcept
        : intrusive_ptr()
        {
          this->reset(ptr);
        }
      intrusive_ptr(const intrusive_ptr &other) noexcept
        : intrusive_ptr()
        {
          this->reset(other.m_sth.copy_release());
        }
      intrusive_ptr(intrusive_ptr &&other) noexcept
        : intrusive_ptr()
        {
          this->reset(other.m_sth.release());
        }
      template<typename yelementT, typename enable_if<is_convertible<typename intrusive_ptr<yelementT>::pointer, pointer>::value>::type * = nullptr>
        intrusive_ptr(const intrusive_ptr<yelementT> &other) noexcept
          : intrusive_ptr()
          {
            this->reset(other.m_sth.copy_release());
          }
      template<typename yelementT, typename enable_if<is_convertible<typename intrusive_ptr<yelementT>::pointer, pointer>::value>::type * = nullptr>
        intrusive_ptr(intrusive_ptr<yelementT> &&other) noexcept
          : intrusive_ptr()
          {
            this->reset(other.m_sth.release());
          }
      intrusive_ptr & operator=(nullptr_t) noexcept
        {
          this->reset();
          return *this;
        }
      intrusive_ptr & operator=(const intrusive_ptr &other) noexcept
        {
          this->reset(other.m_sth.copy_release());
          return *this;
        }
      intrusive_ptr & operator=(intrusive_ptr &&other) noexcept
        {
          this->reset(other.m_sth.release());
          return *this;
        }
      template<typename yelementT, typename enable_if<is_convertible<typename intrusive_ptr<yelementT>::pointer, pointer>::value>::type * = nullptr>
        intrusive_ptr & operator=(const intrusive_ptr<yelementT> &other) noexcept
          {
            this->reset(other.m_sth.copy_release());
            return *this;
          }
      template<typename yelementT, typename enable_if<is_convertible<typename intrusive_ptr<yelementT>::pointer, pointer>::value>::type * = nullptr>
        intrusive_ptr & operator=(intrusive_ptr<yelementT> &&other) noexcept
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
      pointer get() const noexcept
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
      explicit operator bool () const noexcept
        {
          const auto ptr = this->get();
          return !!ptr;
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

      void swap(intrusive_ptr &other) noexcept
        {
          this->m_sth.exchange(other.m_sth);
        }
    };

template<typename xelementT, typename yelementT>
  inline bool operator==(const intrusive_ptr<xelementT> &lhs, const intrusive_ptr<yelementT> &rhs) noexcept
    {
      return lhs.get() == rhs.get();
    }
template<typename xelementT, typename yelementT>
  inline bool operator!=(const intrusive_ptr<xelementT> &lhs, const intrusive_ptr<yelementT> &rhs) noexcept
    {
      return lhs.get() != rhs.get();
    }
template<typename xelementT, typename yelementT>
  inline bool operator<(const intrusive_ptr<xelementT> &lhs, const intrusive_ptr<yelementT> &rhs)
    {
      return lhs.get() < rhs.get();
    }
template<typename xelementT, typename yelementT>
  inline bool operator>(const intrusive_ptr<xelementT> &lhs, const intrusive_ptr<yelementT> &rhs)
    {
      return lhs.get() > rhs.get();
    }
template<typename xelementT, typename yelementT>
  inline bool operator<=(const intrusive_ptr<xelementT> &lhs, const intrusive_ptr<yelementT> &rhs)
    {
      return lhs.get() <= rhs.get();
    }
template<typename xelementT, typename yelementT>
  inline bool operator>=(const intrusive_ptr<xelementT> &lhs, const intrusive_ptr<yelementT> &rhs)
    {
      return lhs.get() >= rhs.get();
    }

template<typename elementT>
  inline bool operator==(const intrusive_ptr<elementT> &lhs, nullptr_t) noexcept
    {
      return !(lhs.get());
    }
template<typename elementT>
  inline bool operator!=(const intrusive_ptr<elementT> &lhs, nullptr_t) noexcept
    {
      return !!(lhs.get());
    }

template<typename elementT>
  inline bool operator==(nullptr_t, const intrusive_ptr<elementT> &rhs) noexcept
    {
      return !(rhs.get());
    }
template<typename elementT>
  inline bool operator!=(nullptr_t, const intrusive_ptr<elementT> &rhs) noexcept
    {
      return !!(rhs.get());
    }

template<typename elementT>
  inline void swap(intrusive_ptr<elementT> &lhs, intrusive_ptr<elementT> &rhs) noexcept
    {
      lhs.swap(rhs);
    }

template<typename elementT>
  template<typename yelementT, typename cvthisT>
    inline intrusive_ptr<yelementT> intrusive_base<elementT>::do_share_this(cvthisT *cvthis)
      {
        const auto ptr = details_intrusive_ptr::static_cast_or_dynamic_cast_helper<yelementT *, intrusive_base *>()(+cvthis);
        if(!ptr) {
          noadl::throw_domain_error("intrusive_base: The current object cannot be converted to type `%s`, whose most derived type is `%s`.",
                                    typeid(yelementT).name(), typeid(*cvthis).name());
        }
        cvthis->refcount_base::add_reference();
        return intrusive_ptr<yelementT>(ptr);
      }

template<typename elementT>
  intrusive_base<elementT>::~intrusive_base()
    = default;

template<typename resultT, typename sourceT>
  inline intrusive_ptr<resultT> static_pointer_cast(const intrusive_ptr<sourceT> &iptr)
    {
      return details_intrusive_ptr::pointer_cast_helper<intrusive_ptr<resultT>, details_intrusive_ptr::static_caster>()(iptr);
    }
template<typename resultT, typename sourceT>
  inline intrusive_ptr<resultT> static_pointer_cast(intrusive_ptr<sourceT> &&iptr)
    {
      return details_intrusive_ptr::pointer_cast_helper<intrusive_ptr<resultT>, details_intrusive_ptr::static_caster>()(::std::move(iptr));
    }

template<typename resultT, typename sourceT>
  inline intrusive_ptr<resultT> dynamic_pointer_cast(const intrusive_ptr<sourceT> &iptr)
    {
      return details_intrusive_ptr::pointer_cast_helper<intrusive_ptr<resultT>, details_intrusive_ptr::dynamic_caster>()(iptr);
    }
template<typename resultT, typename sourceT>
  inline intrusive_ptr<resultT> dynamic_pointer_cast(intrusive_ptr<sourceT> &&iptr)
    {
      return details_intrusive_ptr::pointer_cast_helper<intrusive_ptr<resultT>, details_intrusive_ptr::dynamic_caster>()(::std::move(iptr));
    }

template<typename resultT, typename sourceT>
  inline intrusive_ptr<resultT> const_pointer_cast(const intrusive_ptr<sourceT> &iptr)
    {
      return details_intrusive_ptr::pointer_cast_helper<intrusive_ptr<resultT>, details_intrusive_ptr::const_caster>()(iptr);
    }
template<typename resultT, typename sourceT>
  inline intrusive_ptr<resultT> const_pointer_cast(intrusive_ptr<sourceT> &&iptr)
    {
      return details_intrusive_ptr::pointer_cast_helper<intrusive_ptr<resultT>, details_intrusive_ptr::const_caster>()(::std::move(iptr));
    }

template<typename elementT, typename ...paramsT>
  inline intrusive_ptr<elementT> make_intrusive(paramsT &&...params)
    {
      return intrusive_ptr<elementT>(new elementT(::std::forward<paramsT>(params)...));
    }

template<typename charT, typename traitsT, typename elementT>
  inline basic_ostream<charT, traitsT> & operator<<(basic_ostream<charT, traitsT> &os, const intrusive_ptr<elementT> &iptr)
    {
      return os << iptr.get();
    }

}

#endif
