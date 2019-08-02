// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFCNT_OBJECT_HPP_
#define ROCKET_REFCNT_OBJECT_HPP_

#include "refcnt_ptr.hpp"

namespace rocket {

template<typename elementT> class refcnt_object
  {
    template<typename> friend class refcnt_object;

  public:
    using element_type  = elementT;
    using pointer       = typename refcnt_ptr<element_type>::pointer;

  private:
    refcnt_ptr<element_type> m_ptr;

  public:
    template<typename... paramsT, ROCKET_ENABLE_IF(is_constructible<element_type,
                                                                    paramsT&&...>::value)> explicit refcnt_object(paramsT&&... params)
      : m_ptr(noadl::make_refcnt<element_type>(noadl::forward<paramsT>(params)...))
      {
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_object<yelementT>::pointer,
                                                                 pointer>::value)> refcnt_object(const refcnt_ptr<yelementT>& ptr) noexcept
      : m_ptr((ROCKET_ASSERT(ptr), ptr))
      {
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_object<yelementT>::pointer,
                                                                 pointer>::value)> refcnt_object(refcnt_ptr<yelementT>&& ptr) noexcept
      : m_ptr((ROCKET_ASSERT(ptr), noadl::move(ptr)))
      {
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_object<yelementT>::pointer,
                                                                 pointer>::value)> refcnt_object(const refcnt_object<yelementT>& other) noexcept
      : m_ptr(other.m_ptr)
      {
      }
    refcnt_object(const refcnt_object& other) noexcept
      : m_ptr(other.m_ptr)
      {
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_convertible<typename refcnt_object<yelementT>::pointer,
                                                                 pointer>::value)> refcnt_object& operator=(const refcnt_object<yelementT>& other) noexcept
      {
        this->m_ptr = other.m_ptr;
        return *this;
      }
    refcnt_object& operator=(const refcnt_object& other) noexcept
      {
        this->m_ptr = other.m_ptr;
        return *this;
      }
    refcnt_object& operator=(refcnt_object&& other) noexcept
      {
        this->m_ptr.swap(other.m_ptr);  // We shall not leave a null pointer in `other`.
        return *this;
      }

  public:
    bool unique() const noexcept
      {
        return this->m_ptr.unique();
      }
    long use_count() const noexcept
      {
        return this->m_ptr.use_count();
      }

    constexpr const element_type& get() const noexcept
      {
        return *(this->m_ptr);
      }
    element_type& mut() noexcept
      {
        return *(this->m_ptr);
      }
    constexpr pointer ptr() const noexcept
      {
        return this->m_ptr.get();
      }

    void swap(refcnt_object& other) noexcept
      {
        this->m_ptr.swap(other.m_ptr);
      }

    constexpr operator const refcnt_ptr<element_type>& () const noexcept
      {
        return this->m_ptr;
      }
    constexpr operator const element_type& () const noexcept
      {
        return this->get();
      }
    constexpr const element_type& operator*() const noexcept
      {
        return this->get();
      }
    constexpr pointer operator->() const noexcept
      {
        return this->ptr();
      }
  };

template<typename elementT> void swap(refcnt_object<elementT>& lhs, refcnt_object<elementT>& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

template<typename charT, typename traitsT,
         typename elementT> basic_ostream<charT, traitsT>& operator<<(basic_ostream<charT, traitsT>& os,
                                                                      const refcnt_object<elementT>& rhs)
  {
    return os << rhs.get();
  }


}  // namespace rocket

#endif
