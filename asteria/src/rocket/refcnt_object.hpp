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
    template<typename yelementT, ROCKET_ENABLE_IF(is_base_of<element_type,
                                                             typename decay<yelementT>::type>::value)> explicit refcnt_object(yelementT &&yelem)
      : m_ptr(noadl::make_refcnt<typename decay<yelementT>::type>(::std::forward<yelementT>(yelem)))
      {
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_base_of<element_type,
                                                             yelementT>::value)> refcnt_object(const refcnt_object<yelementT> &other) noexcept
      : m_ptr(other.m_ptr)
      {
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_base_of<element_type,
                                                             yelementT>::value)> refcnt_object(refcnt_object<yelementT> &&other) noexcept
      : m_ptr(::std::move(other.m_ptr))
      {
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_base_of<element_type, typename decay<yelementT>::type>::value)> refcnt_object & operator=(yelementT &&yelem)
      {
        this->m_ptr = noadl::make_refcnt<typename decay<yelementT>::type>(::std::forward<yelementT>(yelem));
        return *this;
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_base_of<element_type,
                                                             yelementT>::value)> refcnt_object & operator=(const refcnt_object<yelementT> &other) noexcept
      {
        this->m_ptr = other.m_ptr;
        return *this;
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_base_of<element_type,
                                                             yelementT>::value)> refcnt_object & operator=(refcnt_object<yelementT> &&other) noexcept
      {
        this->m_ptr = ::std::move(other.m_ptr);
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

    constexpr const element_type & get() const noexcept
      {
        return *(this->m_ptr);
      }
    element_type & mut() noexcept
      {
        return *(this->m_ptr);
      }
    constexpr pointer ptr() const noexcept
      {
        return this->m_ptr;
      }

    constexpr const element_type & operator*() const noexcept
      {
        return this->get();
      }
    constexpr pointer operator->() const noexcept
      {
        return this->ptr();
      }
  };

}

#endif
