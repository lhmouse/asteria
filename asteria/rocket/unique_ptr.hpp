// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_PTR_HPP_
#define ROCKET_UNIQUE_PTR_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "tinyfmt.hpp"

namespace rocket {

template<typename elementT, typename deleterT = default_delete<const elementT>> class unique_ptr;

    namespace details_unique_ptr {

    template<typename elementT, typename deleterT, typename = void>
        struct pointer_of : enable_if<1, elementT*>
      {
      };
    template<typename elementT, typename deleterT>
        struct pointer_of<elementT, deleterT,
                          typename make_void<typename deleterT::pointer>::type> : enable_if<1, typename deleterT::pointer>
      {
      };

    template<typename pointerT, typename deleterT>
        class stored_pointer : private allocator_wrapper_base_for<deleterT>::type
      {
      public:
        using pointer       = pointerT;
        using deleter_type  = deleterT;

      private:
        using deleter_base = typename allocator_wrapper_base_for<deleterT>::type;

      private:
        pointer m_ptr;

      public:
        constexpr stored_pointer() noexcept(is_nothrow_constructible<deleter_type>::value)
          :
            deleter_base(),
            m_ptr()
          {
          }
        explicit constexpr stored_pointer(const deleter_type& del) noexcept
          :
            deleter_base(del),
            m_ptr()
          {
          }
        explicit constexpr stored_pointer(deleter_type&& del) noexcept
          :
            deleter_base(noadl::move(del)),
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
        const deleter_type& as_deleter() const noexcept
          {
            return static_cast<const deleter_base&>(*this);
          }
        deleter_type& as_deleter() noexcept
          {
            return static_cast<deleter_base&>(*this);
          }

        constexpr pointer get() const noexcept
          {
            return this->m_ptr;
          }
        pointer release() noexcept
          {
            return ::std::exchange(this->m_ptr, pointer());
          }
        void reset(pointer ptr_new) noexcept
          {
            auto ptr = ::std::exchange(this->m_ptr, ptr_new);
            if(ptr)
              this->as_deleter()(ptr);
          }
        void exchange_with(stored_pointer& other) noexcept
          {
            xswap(this->m_ptr, other.m_ptr);
          }
      };

    template<typename targetT, typename sourceT,
             typename casterT> unique_ptr<targetT> pointer_cast_aux(unique_ptr<sourceT>&& sptr, casterT&& caster)
      {
        unique_ptr<targetT> dptr;
        // Try casting.
        auto ptr = noadl::forward<casterT>(caster)(sptr.get());
        if(ptr) {
          // Transfer ownership.
          dptr.reset(ptr);
          sptr.release();
        }
        return dptr;
      }

    }  // namespace details_unique_ptr

template<typename elementT, typename deleterT> class unique_ptr
  {
    static_assert(!is_array<elementT>::value, "`elementT` must not be an array type.");

    template<typename, typename> friend class unique_ptr;

  public:
    using element_type  = elementT;
    using deleter_type  = deleterT;
    using pointer       = typename details_unique_ptr::pointer_of<element_type, deleter_type>::type;

  private:
    details_unique_ptr::stored_pointer<pointer, deleter_type> m_sth;

  public:
    // 23.11.1.2.1, constructors
    constexpr unique_ptr(nullptr_t = nullptr) noexcept(is_nothrow_constructible<deleter_type>::value)
      :
        m_sth()
      {
      }
    explicit constexpr unique_ptr(const deleter_type& del) noexcept
      :
        m_sth(del)
      {
      }
    explicit unique_ptr(pointer ptr) noexcept(is_nothrow_constructible<deleter_type>::value)
      :
        unique_ptr()
      {
        this->reset(ptr);
      }
    unique_ptr(pointer ptr, const deleter_type& del) noexcept
      :
        unique_ptr(del)
      {
        this->reset(ptr);
      }
    template<typename yelementT, typename ydeleterT,
             ROCKET_ENABLE_IF(conjunction<is_convertible<typename unique_ptr<yelementT, ydeleterT>::pointer, pointer>,
                                          is_convertible<typename unique_ptr<yelementT, ydeleterT>::deleter_type, deleter_type>>::value)>
        unique_ptr(unique_ptr<yelementT, ydeleterT>&& other) noexcept
      :
        unique_ptr(noadl::move(other.m_sth.as_deleter()))
      {
        this->reset(other.m_sth.release());
      }
    unique_ptr(unique_ptr&& other) noexcept
      :
        unique_ptr(noadl::move(other.m_sth.as_deleter()))
      {
        this->reset(other.m_sth.release());
      }
    unique_ptr(unique_ptr&& other, const deleter_type& del) noexcept
      :
        unique_ptr(del)
      {
        this->reset(other.m_sth.release());
      }
    // 23.11.1.2.3, assignment
    unique_ptr& operator=(unique_ptr&& other) noexcept
      {
        this->m_sth.as_deleter() = noadl::move(other.m_sth.as_deleter());
        this->reset(other.m_sth.release());
        return *this;
      }
    template<typename yelementT, typename ydeleterT,
             ROCKET_ENABLE_IF(conjunction<is_convertible<typename unique_ptr<yelementT, ydeleterT>::pointer, pointer>,
                                          is_convertible<typename unique_ptr<yelementT, ydeleterT>::deleter_type, deleter_type>>::value)>
        unique_ptr& operator=(unique_ptr<yelementT, ydeleterT>&& other) noexcept
      {
        this->m_sth.as_deleter() = noadl::move(other.m_sth.as_deleter());
        this->reset(other.m_sth.release());
        return *this;
      }

  public:
    // 23.11.1.2.4, observers
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
    constexpr const deleter_type& get_deleter() const noexcept
      {
        return this->m_sth.as_deleter();
      }
    deleter_type& get_deleter() noexcept
      {
        return this->m_sth.as_deleter();
      }

    // 23.11.1.2.5, modifiers
    pointer release() noexcept
      {
        return this->m_sth.release();
      }
    // N.B. The return type differs from `std::unique_ptr`.
    unique_ptr& reset(pointer ptr_new = pointer()) noexcept
      {
        return this->m_sth.reset(ptr_new), *this;
      }

    unique_ptr& swap(unique_ptr& other) noexcept
      {
        xswap(this->m_sth.as_deleter(), other.m_sth.as_deleter());
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }
  };

template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
    constexpr bool operator==(const unique_ptr<xelementT, xdeleterT>& lhs,
                              const unique_ptr<yelementT, ydeleterT>& rhs) noexcept
  {
    return lhs.get() == rhs.get();
  }
template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
    constexpr bool operator!=(const unique_ptr<xelementT, xdeleterT>& lhs,
                              const unique_ptr<yelementT, ydeleterT>& rhs) noexcept
  {
    return lhs.get() != rhs.get();
  }
template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
    constexpr bool operator<(const unique_ptr<xelementT, xdeleterT>& lhs,
                             const unique_ptr<yelementT, ydeleterT>& rhs)
  {
    return lhs.get() < rhs.get();
  }
template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
    constexpr bool operator>(const unique_ptr<xelementT, xdeleterT>& lhs,
                             const unique_ptr<yelementT, ydeleterT>& rhs)
  {
    return lhs.get() > rhs.get();
  }
template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
    constexpr bool operator<=(const unique_ptr<xelementT, xdeleterT>& lhs,
                              const unique_ptr<yelementT, ydeleterT>& rhs)
  {
    return lhs.get() <= rhs.get();
  }
template<typename xelementT, typename xdeleterT, typename yelementT, typename ydeleterT>
    constexpr bool operator>=(const unique_ptr<xelementT, xdeleterT>& lhs,
                              const unique_ptr<yelementT, ydeleterT>& rhs)
  {
    return lhs.get() >= rhs.get();
  }

template<typename elementT, typename deleterT>
    constexpr bool operator==(const unique_ptr<elementT, deleterT>& lhs, nullptr_t) noexcept
  {
    return +!lhs;
  }
template<typename elementT, typename deleterT>
    constexpr bool operator!=(const unique_ptr<elementT, deleterT>& lhs, nullptr_t) noexcept
  {
    return !!lhs;
  }

template<typename elementT, typename deleterT>
    constexpr bool operator==(nullptr_t, const unique_ptr<elementT, deleterT>& rhs) noexcept
  {
    return +!rhs;
  }
template<typename elementT, typename deleterT>
    constexpr bool operator!=(nullptr_t, const unique_ptr<elementT, deleterT>& rhs) noexcept
  {
    return !!rhs;
  }

template<typename elementT, typename deleterT>
    inline void swap(unique_ptr<elementT, deleterT>& lhs,
                     unique_ptr<elementT, deleterT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

template<typename charT, typename traitsT, typename elementT, typename deleterT>
    inline basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt,
                                                     const unique_ptr<elementT, deleterT>& rhs)
  {
    return fmt << rhs.get();
  }

template<typename targetT, typename sourceT>
    inline unique_ptr<targetT> static_pointer_cast(unique_ptr<sourceT>&& sptr) noexcept
  {
    return details_unique_ptr::pointer_cast_aux<targetT>(noadl::move(sptr),
                               [](sourceT* ptr) { return static_cast<targetT*>(ptr);  });
  }
template<typename targetT, typename sourceT>
    inline unique_ptr<targetT> dynamic_pointer_cast(unique_ptr<sourceT>&& sptr) noexcept
  {
    return details_unique_ptr::pointer_cast_aux<targetT>(noadl::move(sptr),
                               [](sourceT* ptr) { return dynamic_cast<targetT*>(ptr);  });
  }
template<typename targetT, typename sourceT>
    inline unique_ptr<targetT> const_pointer_cast(unique_ptr<sourceT>&& sptr) noexcept
  {
    return details_unique_ptr::pointer_cast_aux<targetT>(noadl::move(sptr),
                               [](sourceT* ptr) { return const_cast<targetT*>(ptr);  });
  }

template<typename elementT, typename... paramsT>
    inline unique_ptr<elementT> make_unique(paramsT&&... params)
  {
    return unique_ptr<elementT>(new elementT(noadl::forward<paramsT>(params)...));
  }

}  // namespace rocket

#endif
