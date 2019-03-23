// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_PTR_HPP_
#define ROCKET_UNIQUE_PTR_HPP_

#include "compatibility.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"

namespace rocket {

template<typename elementT, typename deleterT = default_delete<const elementT>> class unique_ptr;

    namespace details_unique_ptr {

    template<typename elementT, typename deleterT,
             typename = void> struct pointer_of : enable_if<true, elementT*>
      {
      };
    template<typename elementT, typename deleterT
             > struct pointer_of<elementT, deleterT,
                                 typename make_void<typename deleterT::pointer>::type> : enable_if<true, typename deleterT::pointer>
      {
      };

    template<typename resultT, typename sourceT,
             typename = void> struct static_cast_or_dynamic_cast_helper
      {
        constexpr resultT operator()(sourceT&& src) const
          {
            return dynamic_cast<resultT>(noadl::forward<sourceT>(src));
          }
      };
    template<typename resultT, typename sourceT
             > struct static_cast_or_dynamic_cast_helper<resultT, sourceT,
                                                         typename make_void<decltype(static_cast<resultT>(::std::declval<sourceT>()))>::type>
      {
        constexpr resultT operator()(sourceT&& src) const
          {
            return static_cast<resultT>(noadl::forward<sourceT>(src));
          }
      };

    template<typename pointerT, typename deleterT> class stored_pointer : private allocator_wrapper_base_for<deleterT>::type
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
          : deleter_base(),
            m_ptr()
          {
          }
        explicit constexpr stored_pointer(const deleter_type& del) noexcept
          : deleter_base(del),
            m_ptr()
          {
          }
        explicit constexpr stored_pointer(deleter_type&& del) noexcept
          : deleter_base(noadl::move(del)),
            m_ptr()
          {
          }
        ~stored_pointer()
          {
            this->reset(pointer());
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
            auto ptr_old = ::std::exchange(this->m_ptr, ptr_new);
            if(!ptr_old) {
              return;
            }
            this->as_deleter()(ptr_old);
          }
        void exchange(stored_pointer& other) noexcept
          {
            ::std::swap(this->m_ptr, other.m_ptr);
          }
      };

    struct static_caster
      {
        template<typename resultT, typename sourceT> static constexpr resultT do_cast(sourceT&& src)
          {
            return static_cast<resultT>(noadl::forward<sourceT>(src));
          }
      };
    struct dynamic_caster
      {
        template<typename resultT, typename sourceT> static constexpr resultT do_cast(sourceT&& src)
          {
            return dynamic_cast<resultT>(noadl::forward<sourceT>(src));
          }
      };
    struct const_caster
      {
        template<typename resultT, typename sourceT> static constexpr resultT do_cast(sourceT&& src)
          {
            return const_cast<resultT>(noadl::forward<sourceT>(src));
          }
      };

    template<typename resultptrT, typename casterT> struct pointer_cast_helper
      {
        template<typename sourceptrT> resultptrT operator()(sourceptrT&& sptr) const
          {
            auto ptr = casterT::template do_cast<typename resultptrT::pointer>(sptr.get());
            if(!ptr) {
              return nullptr;
            }
            auto tptr = noadl::forward<sourceptrT>(sptr);
            tptr.release();
            return resultptrT(ptr);
          }
      };

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
      : m_sth()
      {
      }
    explicit constexpr unique_ptr(const deleter_type& del) noexcept
      : m_sth(del)
      {
      }
    explicit unique_ptr(pointer ptr) noexcept(is_nothrow_constructible<deleter_type>::value)
      : unique_ptr()
      {
        this->reset(ptr);
      }
    unique_ptr(pointer ptr, const deleter_type& del) noexcept
      : unique_ptr(del)
      {
        this->reset(ptr);
      }
    unique_ptr(unique_ptr&& other) noexcept
      : unique_ptr(noadl::move(other.m_sth.as_deleter()))
      {
        this->reset(other.m_sth.release());
      }
    unique_ptr(unique_ptr&& other, const deleter_type& del) noexcept
      : unique_ptr(del)
      {
        this->reset(other.m_sth.release());
      }
    template<typename yelementT, typename ydeleterT, ROCKET_ENABLE_IF(conjunction<is_convertible<typename unique_ptr<yelementT, ydeleterT>::pointer,
                                                                                                 pointer>,
                                                                                  is_convertible<typename unique_ptr<yelementT, ydeleterT>::deleter_type,
                                                                                                 deleter_type>>::value)
             > unique_ptr(unique_ptr<yelementT, ydeleterT>&& other) noexcept
      : unique_ptr(noadl::move(other.m_sth.as_deleter()))
      {
        this->reset(other.m_sth.release());
      }
    // 23.11.1.2.3, assignment
    unique_ptr& operator=(unique_ptr&& other) noexcept
      {
        allocator_move_assigner<deleter_type, true>()(this->m_sth.as_deleter(), noadl::move(other.m_sth.as_deleter()));
        this->reset(other.m_sth.release());
        return *this;
      }
    template<typename yelementT, typename ydeleterT, ROCKET_ENABLE_IF(conjunction<is_convertible<typename unique_ptr<yelementT, ydeleterT>::pointer,
                                                                                                 pointer>,
                                                                                  is_convertible<typename unique_ptr<yelementT, ydeleterT>::deleter_type,
                                                                                                 deleter_type>>::value)
             > unique_ptr& operator=(unique_ptr<yelementT, ydeleterT>&& other) noexcept
      {
        allocator_move_assigner<deleter_type, true>()(this->m_sth.as_deleter(), noadl::move(other.m_sth.as_deleter()));
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
        this->m_sth.reset(ptr_new);
        return *this;
      }

    void swap(unique_ptr& other) noexcept
      {
        allocator_swapper<deleter_type, true>()(this->m_sth.as_deleter(), other.m_sth.as_deleter());
        this->m_sth.exchange(other.m_sth);
      }
  };

template<typename xelementT, typename xdeleterT,
         typename yelementT, typename ydeleterT> inline bool operator==(const unique_ptr<xelementT, xdeleterT>& lhs,
                                                                        const unique_ptr<yelementT, ydeleterT>& rhs) noexcept
  {
    return lhs.get() == rhs.get();
  }
template<typename xelementT, typename xdeleterT,
         typename yelementT, typename ydeleterT> inline bool operator!=(const unique_ptr<xelementT, xdeleterT>& lhs,
                                                                        const unique_ptr<yelementT, ydeleterT>& rhs) noexcept
  {
    return lhs.get() != rhs.get();
  }
template<typename xelementT, typename xdeleterT,
         typename yelementT, typename ydeleterT> inline bool operator<(const unique_ptr<xelementT, xdeleterT>& lhs,
                                                                       const unique_ptr<yelementT, ydeleterT>& rhs)
  {
    return lhs.get() < rhs.get();
  }
template<typename xelementT, typename xdeleterT,
         typename yelementT, typename ydeleterT> inline bool operator>(const unique_ptr<xelementT, xdeleterT>& lhs,
                                                                       const unique_ptr<yelementT, ydeleterT>& rhs)
  {
    return lhs.get() > rhs.get();
  }
template<typename xelementT, typename xdeleterT,
         typename yelementT, typename ydeleterT> inline bool operator<=(const unique_ptr<xelementT, xdeleterT>& lhs,
                                                                        const unique_ptr<yelementT, ydeleterT>& rhs)
  {
    return lhs.get() <= rhs.get();
  }
template<typename xelementT, typename xdeleterT,
         typename yelementT, typename ydeleterT> inline bool operator>=(const unique_ptr<xelementT, xdeleterT>& lhs,
                                                                        const unique_ptr<yelementT, ydeleterT>& rhs)
  {
    return lhs.get() >= rhs.get();
  }

template<typename elementT, typename deleterT> inline bool operator==(const unique_ptr<elementT, deleterT>& lhs, nullptr_t) noexcept
  {
    return +!(lhs.get());
  }
template<typename elementT, typename deleterT> inline bool operator!=(const unique_ptr<elementT, deleterT>& lhs, nullptr_t) noexcept
  {
    return !!(lhs.get());
  }

template<typename elementT, typename deleterT> inline bool operator==(nullptr_t, const unique_ptr<elementT, deleterT>& rhs) noexcept
  {
    return +!(rhs.get());
  }
template<typename elementT, typename deleterT> inline bool operator!=(nullptr_t, const unique_ptr<elementT, deleterT>& rhs) noexcept
  {
    return !!(rhs.get());
  }

template<typename elementT, typename deleterT> inline void swap(unique_ptr<elementT, deleterT>& lhs,
                                                                unique_ptr<elementT, deleterT>& rhs) noexcept
  {
    lhs.swap(rhs);
  }

template<typename charT, typename traitsT,
         typename elementT, typename deleterT> inline basic_ostream<charT, traitsT>& operator<<(basic_ostream<charT, traitsT>& os,
                                                                                                 const unique_ptr<elementT, deleterT>& rhs)
  {
    return os << rhs.get();
  }

template<typename resultT, typename sourceT> inline unique_ptr<resultT> static_pointer_cast(unique_ptr<sourceT>&& sptr) noexcept
  {
    return details_unique_ptr::pointer_cast_helper<unique_ptr<resultT>, details_unique_ptr::static_caster>()(noadl::move(sptr));
  }
template<typename resultT, typename sourceT> inline unique_ptr<resultT> dynamic_pointer_cast(unique_ptr<sourceT>&& sptr) noexcept
  {
    return details_unique_ptr::pointer_cast_helper<unique_ptr<resultT>, details_unique_ptr::dynamic_caster>()(noadl::move(sptr));
  }
template<typename resultT, typename sourceT> inline unique_ptr<resultT> const_pointer_cast(unique_ptr<sourceT>&& sptr) noexcept
  {
    return details_unique_ptr::pointer_cast_helper<unique_ptr<resultT>, details_unique_ptr::const_caster>()(noadl::move(sptr));
  }

template<typename elementT, typename... paramsT> inline unique_ptr<elementT> make_unique(paramsT&&... params)
  {
    return unique_ptr<elementT>(new elementT(noadl::forward<paramsT>(params)...));
  }

}  // namespace rocket

#endif
