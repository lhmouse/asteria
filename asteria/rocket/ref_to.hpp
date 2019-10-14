// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REF_TO_HPP_
#define ROCKET_REF_TO_HPP_

#include "utilities.hpp"

namespace rocket {

template<typename valueT> class ref_to
  {
    static_assert(!is_reference<valueT>::value, "`valueT` must not be a reference type.");
    static_assert(!is_void<valueT>::value, "`valueT` must not be a `void` type.");

    template<typename> friend class ref_to;

  public:
    // types
    using value_type  = valueT;
    using pointer     = value_type*;
    using reference   = value_type&;

  private:
    value_type* m_ptr;

  public:
    // construct/copy/destroy
    template<typename otherT, ROCKET_ENABLE_IF(is_convertible<otherT*, valueT*>::value)>
        explicit constexpr ref_to(otherT& other) noexcept
      :
        m_ptr(::std::addressof(other))
      { }
    template<typename otherT, ROCKET_ENABLE_IF(is_convertible<otherT*, valueT*>::value)>
        constexpr ref_to(ref_to<otherT> other) noexcept
      :
        m_ptr(other.m_ptr)
      { }

  public:
    // access
    constexpr reference get() const noexcept
      {
        return *(this->m_ptr);
      }
    constexpr pointer ptr() const noexcept
      {
        return this->m_ptr;
      }

    constexpr operator reference () const noexcept
      {
        return this->get();
      }
    constexpr pointer operator->() const noexcept
      {
        return this->ptr();
      }
  };

template<typename valueT> constexpr ref_to<const valueT> cref(valueT& value) noexcept
  {
    return ref_to<const valueT>(value);
  }
template<typename valueT> constexpr ref_to<const valueT> cref(ref_to<valueT> value) noexcept
  {
    return ref_to<const valueT>(value);
  }
template<typename valueT> constexpr ref_to<const valueT> cref(valueT&& value) noexcept
  = delete;

template<typename valueT> constexpr ref_to<valueT> ref(valueT& value) noexcept
  {
    return ref_to<valueT>(value);
  }
template<typename valueT> constexpr ref_to<valueT> ref(ref_to<valueT> value) noexcept
  {
    return ref_to<valueT>(value);
  }
template<typename valueT> constexpr ref_to<valueT> ref(valueT&& value) noexcept
  = delete;

}  // namespace rocket

#endif
