// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFERENCE_WRAPPER_
#define ROCKET_REFERENCE_WRAPPER_

#include "fwd.hpp"
namespace rocket {

template<typename valueT>
class reference_wrapper
  {
    static_assert(!is_reference<valueT>::value, "invalid element type");
    static_assert(!is_void<valueT>::value, "cannot form references to void");

    template<typename>
    friend class reference_wrapper;

  public:
    // types
    using value_type  = valueT;
    using pointer     = value_type*;
    using reference   = value_type&;

  private:
    value_type* m_ptr;

  public:
    // construct/copy/destroy
    template<typename otherT,
    ROCKET_ENABLE_IF(is_convertible<otherT*, valueT*>::value)>
    explicit constexpr
    reference_wrapper(otherT& other) noexcept
      : m_ptr(::std::addressof(other))  { }

    template<typename otherT,
    ROCKET_ENABLE_IF(is_convertible<otherT*, valueT*>::value)>
    constexpr
    reference_wrapper(const reference_wrapper<otherT>& other) noexcept
      : m_ptr(other.m_ptr)  { }

  public:
    // access
    constexpr
    reference
    get() const noexcept
      { return *(this->m_ptr);  }

    constexpr
    pointer
    ptr() const noexcept
      { return this->m_ptr;  }

    constexpr operator
    reference() const noexcept
      { return this->get();  }
  };

template<typename valueT>
constexpr
reference_wrapper<const valueT>
cref(valueT& value) noexcept
  {
    return reference_wrapper<const valueT>(value);
  }

template<typename valueT>
constexpr
reference_wrapper<const valueT>
cref(reference_wrapper<valueT> value) noexcept
  {
    return reference_wrapper<const valueT>(value);
  }

template<typename valueT>
constexpr
reference_wrapper<const valueT>
cref(valueT&& value) = delete;

template<typename valueT>
constexpr
reference_wrapper<valueT>
ref(valueT& value) noexcept
  {
    return reference_wrapper<valueT>(value);
  }

template<typename valueT>
constexpr
reference_wrapper<valueT>
ref(reference_wrapper<valueT> value) noexcept
  {
    return reference_wrapper<valueT>(value);
  }

template<typename valueT>
constexpr
reference_wrapper<valueT>
ref(valueT&& value) = delete;

}  // namespace rocket
#endif
