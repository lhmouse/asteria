// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFCOUNTED_OBJECT_HPP_
#define ROCKET_REFCOUNTED_OBJECT_HPP_

#include "refcounted_ptr.hpp"

namespace rocket {

template<typename elementT>
  class refcounted_object
  {
  public:
    using element_type  = elementT;
    using pointer       = elementT *;

  private:
    refcounted_ptr<element_type> m_owns;
    pointer m_rptr;

  public:
    template<typename yelementT, ROCKET_ENABLE_IF(is_base_of<element_type, yelementT>::value)>
      constexpr refcounted_object(reference_wrapper<yelementT> ref) noexcept
      : m_owns(),
        m_rptr(::std::addressof(ref.get()))
      {
      }
    template<typename yelementT, ROCKET_ENABLE_IF(is_base_of<element_type, typename decay<yelementT>::type>::value)>
      explicit refcounted_object(yelementT &&yelem)
      : m_owns(noadl::make_refcounted<typename decay<yelementT>::type>(::std::forward<yelementT>(yelem))),
        m_rptr(this->m_owns.get())
      {
      }

  public:
    bool unique() const noexcept
      {
        return this->m_owns.unique();
      }
    long use_count() const noexcept
      {
        return this->m_owns.use_count();
      }

    constexpr const element_type & get() const noexcept
      {
        return *(this->m_rptr);
      }
    element_type & mut() noexcept
      {
        return *(this->m_rptr);
      }
    constexpr pointer ptr() const noexcept
      {
        return this->m_rptr;
      }
  };

}

#endif
