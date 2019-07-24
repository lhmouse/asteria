// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_OPTIONAL_HPP_
#define ROCKET_OPTIONAL_HPP_

#include "compiler.h"
#include "static_vector.hpp"

namespace rocket {

template<typename valueT> class optional;

struct nullopt_t
  {
  }
constexpr nullopt;

template<typename valueT> class optional
  {
    static_assert(!is_array<valueT>::value, "`valueT` must not be an array type.");
    static_assert(!is_same<valueT, nullopt_t>::value, "`valueT` shall not be `nullopt_t`.");

  public:
    using value_type       = valueT;
    using const_reference  = const value_type&;
    using reference        = value_type&;

  private:
    static_vector<valueT, 1> m_stor;

  public:
    // 19.6.3.1, constructors
    constexpr optional(nullopt_t = nullopt) noexcept
      : m_stor()
      {
      }
    template<typename yvalueT, ROCKET_ENABLE_IF(is_convertible<yvalueT&&,
                                                               value_type>::value)>
        optional(yvalueT&& yvalue) noexcept(is_nothrow_constructible<value_type,
                                                                     yvalueT&&>::value)
      : m_stor()
      {
        this->m_stor.emplace_back(noadl::forward<yvalueT>(yvalue));
      }
    template<typename yvalueT, ROCKET_ENABLE_IF(is_convertible<const typename optional<yvalueT>::value_type&,
                                                               value_type>::value)>
        optional(const optional<yvalueT>& other) noexcept(is_nothrow_constructible<value_type,
                                                                                   const typename optional<yvalueT>::value_type&>::value)
      : m_stor()
      {
        if(other.m_stor.empty()) {
          this->m_stor.emplace_back(other.m_stor.front());
        }
      }
    template<typename yvalueT, ROCKET_ENABLE_IF(is_convertible<typename optional<yvalueT>::value_type&&,
                                                               value_type>::value)>
        optional(optional<yvalueT>&& other) noexcept(is_nothrow_constructible<value_type,
                                                                              typename optional<yvalueT>::value_type&&>::value)
      : m_stor()
      {
        if(!other.m_stor.empty()) {
          this->m_stor.emplace_back(noadl::move(other.m_stor.front()));
        }
      }
    // 19.6.3.3, assignment
    optional& operator=(nullopt_t) noexcept
      {
        this->m_stor.clear();
        return *this;
      }
    template<typename yvalueT, ROCKET_ENABLE_IF(is_assignable<value_type,
                                                              yvalueT&&>::value)>
        optional& operator=(yvalueT&& yvalue) noexcept(conjunction<is_nothrow_constructible<value_type,
                                                                                            yvalueT&&>,
                                                                   is_nothrow_assignable<value_type,
                                                                                         yvalueT&&>>::value)
      {
        if(!this->m_stor.empty()) {
          this->m_stor.mut_front() = noadl::forward<yvalueT>(yvalue);
        }
        else {
          this->m_stor.emplace_back(noadl::forward<yvalueT>(yvalue));
        }
        return *this;
      }
    template<typename yvalueT, ROCKET_ENABLE_IF(is_assignable<value_type,
                                                              const typename optional<yvalueT>::value_type&>::value)>
        optional& operator=(const optional<yvalueT>& other) noexcept(conjunction<is_nothrow_constructible<value_type,
                                                                                                          const typename optional<yvalueT>::value_type&>,
                                                                                 is_nothrow_assignable<value_type,
                                                                                                       const typename optional<yvalueT>::value_type&>>::value)
      {
        if(other.m_stor.empty()) {
          this->m_stor.clear();
        }
        else if(!this->m_stor.empty()) {
          this->m_stor.mut_front() = other.m_stor.front();
        }
        else {
          this->m_stor.emplace_back(other.m_stor.front());
        }
        return *this;
      }
    template<typename yvalueT, ROCKET_ENABLE_IF(is_assignable<value_type,
                                                              typename optional<yvalueT>::value_type&&>::value)>
        optional& operator=(optional<yvalueT>&& other) noexcept(conjunction<is_nothrow_constructible<value_type,
                                                                                                     typename optional<yvalueT>::value_type&&>,
                                                                            is_nothrow_assignable<value_type,
                                                                                                  typename optional<yvalueT>::value_type&&>>::value)
      {
        if(other.m_stor.empty()) {
          this->m_stor.clear();
        }
        else if(!this->m_stor.empty()) {
          this->m_stor.mut_front() = noadl::move(other.m_stor.front());
        }
        else {
          this->m_stor.emplace_back(noadl::move(other.m_stor.front()));
        }
        return *this;
      }

  private:
    [[noreturn]] ROCKET_NOINLINE reference do_throw_valueless() const
      {
        noadl::sprintf_and_throw<length_error>("variant: No value has been stored in this variant.");
      }

  public:
    // 19.6.3.5, observers
    constexpr bool has_value() const noexcept
      {
        return this->m_stor.size() != 0;
      }
    explicit constexpr operator bool () const noexcept
      {
        return this->m_stor.size() != 0;
      }

    const_reference value() const
      {
        return this->m_stor.empty() ? this->do_throw_valueless() : this->m_stor.front();
      }
    reference value()
      {
        return this->m_stor.empty() ? this->do_throw_valueless() : this->m_stor.mut_front();
      }
    // N.B. This is a non-standard extension.
    const value_type* value_ptr() const
      {
        return this->m_stor.empty() ? nullptr : ::std::addressof(this->m_stor.front());
      }
    // N.B. This is a non-standard extension.
    value_type* value_ptr()
      {
        return this->m_stor.empty() ? nullptr : ::std::addressof(this->m_stor.mut_front());
      }
    // N.B. The return type differs from `std::variant`.
    template<typename dvalueT> decltype(0 ? ::std::declval<const_reference>()
                                          : ::std::declval<dvalueT>()) value_or(dvalueT&& dvalue) const
      {
        return this->m_stor.empty() ? noadl::forward<dvalueT>(dvalue) : this->m_stor.front();
      }
    // N.B. The return type differs from `std::variant`.
    template<typename dvalueT> decltype(0 ? ::std::declval<reference>()
                                          : ::std::declval<dvalueT>()) value_or(dvalueT&& dvalue)
      {
        return this->m_stor.empty() ? noadl::forward<dvalueT>(dvalue) : this->m_stor.mut_front();
      }
    // N.B. This is a non-standard extension.
    template<typename dvalueT> decltype(0 ? ::std::declval<value_type&&>()
                                          : ::std::declval<dvalueT>()) move_value_or(dvalueT&& dvalue)
      {
        return this->m_stor.empty() ? noadl::forward<dvalueT>(dvalue) : noadl::move(this->m_stor.mut_front());
      }

    constexpr const_reference operator*() const
      {
        return this->m_stor.front();
      }
    constexpr reference operator*()
      {
        return this->m_stor.mut_front();
      }
    constexpr const value_type* operator->() const
      {
        return ::std::addressof(this->m_stor.front());
      }
    constexpr value_type* operator->()
      {
        return ::std::addressof(this->m_stor.mut_front());
      }

    // 19.6.3.6, modifiers
    void reset() noexcept
      {
        this->m_stor.clear();
      }
    template<typename... paramsT> reference emplace(paramsT&&... params)
      {
        return this->m_stor.clear(), this->m_stor.emplace_back(noadl::forward<paramsT>(params)...);
      }
    // N.B. This is a non-standard extension.
    template<typename... paramsT> reference value_or_emplace(paramsT&&... params)
      {
        return this->m_stor.empty() ? this->m_stor.emplace_back(noadl::forward<paramsT>(params)...) : this->m_stor.mut_front();
      }

    // 19.6.3.4, swap
    void swap(optional& other) noexcept(conjunction<is_nothrow_swappable<value_type>,
                                                    is_nothrow_move_constructible<value_type>,
                                                    is_nothrow_move_assignable<value_type>>::value)
      {
        this->m_stor.swap(other.m_stor);
      }
  };

template<typename valueT> void swap(optional<valueT>& lhs, optional<valueT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    return lhs.swap(rhs);
  }

template<typename valueT> constexpr bool operator==(const optional<valueT>& lhs,
                                                    const optional<valueT>& rhs) noexcept(noexcept(::std::declval<const valueT&>()
                                                                                                   == ::std::declval<const valueT&>()))
  {
    // `lhs` and `rhs` are equal if they contain values that are equal, or both contain no value.
    return (lhs && rhs) ? (*lhs == *rhs) : (!lhs == !rhs);
  }
template<typename valueT> constexpr bool operator!=(const optional<valueT>& lhs,
                                                    const optional<valueT>& rhs) noexcept(noexcept(::std::declval<const valueT&>()
                                                                                                   != ::std::declval<const valueT&>()))
  {
    // `lhs` and `rhs` are unequal if they contain values that are not equal, or one contains a value and the other does not.
    return (lhs && rhs) ? (*lhs != *rhs) : (!lhs != !rhs);
  }

template<typename valueT> constexpr bool operator==(const optional<valueT>& lhs, nullopt_t) noexcept
  {
    return +!lhs;
  }
template<typename valueT> constexpr bool operator!=(const optional<valueT>& lhs, nullopt_t) noexcept
  {
    return !!lhs;
  }

template<typename valueT> constexpr bool operator==(nullopt_t, const optional<valueT>& rhs) noexcept
  {
    return +!rhs;
  }
template<typename valueT> constexpr bool operator!=(nullopt_t, const optional<valueT>& rhs) noexcept
  {
    return !!rhs;
  }

template<typename valueT> constexpr bool operator==(const optional<valueT>& lhs,
                                                    const valueT& rhs) noexcept(noexcept(::std::declval<const valueT&>()
                                                                                         == ::std::declval<const valueT&>()))
  {
    return !!lhs && (*lhs == rhs);
  }
template<typename valueT> constexpr bool operator!=(const optional<valueT>& lhs,
                                                    const valueT& rhs) noexcept(noexcept(::std::declval<const valueT&>()
                                                                                         != ::std::declval<const valueT&>()))
  {
    return +!lhs || (*lhs != rhs);
  }

template<typename valueT> constexpr bool operator==(const valueT& lhs,
                                                    const optional<valueT>& rhs) noexcept(noexcept(::std::declval<const valueT&>()
                                                                                                   == ::std::declval<const valueT&>()))
  {
    return !!rhs && (lhs == *rhs);
  }
template<typename valueT> constexpr bool operator!=(const valueT& lhs,
                                                    const optional<valueT>& rhs) noexcept(noexcept(::std::declval<const valueT&>()
                                                                                                   != ::std::declval<const valueT&>()))
  {
    return +!rhs || (lhs != *rhs);
  }

template<typename charT, typename traitsT,
         typename valueT> basic_ostream<charT, traitsT>& operator<<(basic_ostream<charT, traitsT>& os,
                                                                    const optional<valueT>& rhs)
  {
    return rhs ? (os << *rhs) : (os << "<nullopt>");
  }

}  // namespace rocket

#endif
