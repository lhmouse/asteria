// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_OPTIONAL_HPP_
#define ROCKET_OPTIONAL_HPP_

#include "static_vector.hpp"

namespace rocket {

template<typename valueT>
class optional;

template<typename charT, typename traitsT>
class basic_tinyfmt;

enum class nullopt_t
  : uintptr_t
  { }
  constexpr nullopt = { };

template<typename valueT>
class optional
  {
    static_assert(!is_array<valueT>::value, "invalid element type");
    static_assert(!is_same<valueT, nullopt_t>::value, "invalid element type");

    template<typename>
    friend class optional;

  public:
    using value_type       = valueT;
    using const_reference  = const value_type&;
    using reference        = value_type&;

  private:
    static_vector<valueT, 1> m_stor;

  public:
    // 19.6.3.1, constructors
    constexpr
    optional(nullopt_t = nullopt) noexcept
      { }

    optional(const value_type& value) noexcept(is_nothrow_copy_constructible<value_type>::value)
      { this->m_stor.emplace_back(value);  }

    optional(value_type&& value) noexcept(is_nothrow_move_constructible<value_type>::value)
      { this->m_stor.emplace_back(::std::move(value));  }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<yvalueT&&, value_type>::value)>
    optional(yvalueT&& yvalue) noexcept(is_nothrow_constructible<value_type, yvalueT&&>::value)
      { this->m_stor.emplace_back(::std::forward<yvalueT>(yvalue));  }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<const typename optional<yvalueT>::value_type&,
                                    value_type>::value)>
    optional(const optional<yvalueT>& other)
      noexcept(is_nothrow_constructible<value_type,
               const typename optional<yvalueT>::value_type&>::value)
      {
        if(other.m_stor.empty())
          this->m_stor.emplace_back(other.m_stor.front());
      }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<typename optional<yvalueT>::value_type&&,
                                    value_type>::value)>
    optional(optional<yvalueT>&& other)
      noexcept(is_nothrow_constructible<value_type,
               typename optional<yvalueT>::value_type&&>::value)
      {
        if(!other.m_stor.empty())
          this->m_stor.emplace_back(::std::move(other.m_stor.mut_front()));
      }

    // 19.6.3.3, assignment
    optional&
    operator=(nullopt_t) noexcept
      { this->m_stor.clear();
        return *this;  }

    optional&
    operator=(const value_type& value)
      noexcept(conjunction<is_nothrow_copy_constructible<value_type>,
                           is_nothrow_copy_assignable<value_type>>::value)
      {
        if(!this->m_stor.empty())
          this->m_stor.mut_front() = value;
        else
          this->m_stor.emplace_back(value);
        return *this;
      }

    optional&
    operator=(value_type&& value)
      noexcept(conjunction<is_nothrow_move_constructible<value_type>,
                           is_nothrow_move_assignable<value_type>>::value)
      {
        if(!this->m_stor.empty())
          this->m_stor.mut_front() = ::std::move(value);
        else
          this->m_stor.emplace_back(::std::move(value));
        return *this;
      }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_assignable<value_type&, yvalueT&&>::value)>
    optional&
    operator=(yvalueT&& yvalue)
      noexcept(conjunction<is_nothrow_constructible<value_type, yvalueT&&>,
                           is_nothrow_assignable<value_type&, yvalueT&&>>::value)
      {
        if(!this->m_stor.empty())
          this->m_stor.mut_front() = ::std::forward<yvalueT>(yvalue);
        else
          this->m_stor.emplace_back(::std::forward<yvalueT>(yvalue));
        return *this;
      }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_assignable<value_type&,
                          const typename optional<yvalueT>::value_type&>::value)>
    optional&
    operator=(const optional<yvalueT>& other)
      noexcept(conjunction<is_nothrow_constructible<value_type,
                                 const typename optional<yvalueT>::value_type&>,
                           is_nothrow_assignable<value_type&,
                                 const typename optional<yvalueT>::value_type&>>::value)
      {
        if(other.m_stor.empty())
          this->m_stor.clear();
        else if(!this->m_stor.empty())
          this->m_stor.mut_front() = other.m_stor.front();
        else
          this->m_stor.emplace_back(other.m_stor.front());
        return *this;
      }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_assignable<value_type&,
                         typename optional<yvalueT>::value_type&&>::value)>
    optional&
    operator=(optional<yvalueT>&& other)
      noexcept(conjunction<is_nothrow_constructible<value_type,
                                typename optional<yvalueT>::value_type&&>,
                           is_nothrow_assignable<value_type&,
                                typename optional<yvalueT>::value_type&&>>::value)
      {
        if(other.m_stor.empty())
          this->m_stor.clear();
        else if(!this->m_stor.empty())
          this->m_stor.mut_front() = ::std::move(other.m_stor.mut_front());
        else
          this->m_stor.emplace_back(::std::move(other.m_stor.mut_front()));
        return *this;
      }

    // 19.6.3.4, swap
    optional&
    swap(optional& other)
      noexcept(conjunction<is_nothrow_swappable<value_type>,
                           is_nothrow_move_constructible<value_type>,
                           is_nothrow_move_assignable<value_type>>::value)
      { this->m_stor.swap(other.m_stor);
        return *this;  }

  private:
    [[noreturn]] ROCKET_NEVER_INLINE reference
    do_throw_valueless() const
      {
        noadl::sprintf_and_throw<length_error>("variant: no value set");
      }

  public:
    // 19.6.3.5, observers
    constexpr bool
    has_value() const noexcept
      { return this->m_stor.size() != 0;  }

    explicit constexpr operator
    bool() const noexcept
      { return this->m_stor.size() != 0;  }

    const_reference
    value() const
      {
        return this->m_stor.empty()
                 ? this->do_throw_valueless()
                 : this->m_stor.front();
      }

    reference
    value()
      {
        return this->m_stor.empty()
                 ? this->do_throw_valueless()
                 : this->m_stor.mut_front();
      }

    // N.B. This is a non-standard extension.
    const value_type*
    value_ptr() const
      {
        return this->m_stor.empty()
                 ? nullptr
                 : this->m_stor.data();
      }

    // N.B. This is a non-standard extension.
    value_type*
    value_ptr()
      {
        return this->m_stor.empty()
                 ? nullptr
                 : this->m_stor.mut_data();
      }

    // N.B. The return type differs from `std::variant`.
    template<typename defvalT>
    typename select_type<const_reference, defvalT&&>::type
    value_or(defvalT&& defval) const
      {
        return this->m_stor.empty()
                 ? ::std::forward<defvalT>(defval)
                 : this->m_stor.front();
      }

    // N.B. The return type differs from `std::variant`.
    template<typename defvalT>
    typename select_type<reference, defvalT&&>::type
    value_or(defvalT&& defval)
      {
        return this->m_stor.empty()
                 ? ::std::forward<defvalT>(defval)
                 : this->m_stor.mut_front();
      }

    // N.B. This is a non-standard extension.
    template<typename defvalT>
    typename select_type<value_type&&, defvalT&&>::type
    move_value_or(defvalT&& defval)
      {
        return this->m_stor.empty()
                 ? ::std::forward<defvalT>(defval)
                 : ::std::move(this->m_stor.mut_front());
      }

    constexpr const_reference
    operator*() const
      { return this->m_stor.front();  }

    constexpr reference
    operator*()
      { return this->m_stor.mut_front();  }

    constexpr const value_type*
    operator->() const
      { return ::std::addressof(this->m_stor.front());  }

    constexpr value_type*
    operator->()
      { return ::std::addressof(this->m_stor.mut_front());  }

    // 19.6.3.6, modifiers
    optional&
    reset() noexcept
      {
        this->m_stor.clear();
        return *this;
      }

    template<typename... paramsT>
    reference
    emplace(paramsT&&... params)
      {
        this->m_stor.clear();
        return this->m_stor.emplace_back(::std::forward<paramsT>(params)...);
      }

    // N.B. This is a non-standard extension.
    template<typename... paramsT>
    reference
    value_or_emplace(paramsT&&... params)
      {
        return this->m_stor.empty()
                 ? this->m_stor.emplace_back(::std::forward<paramsT>(params)...)
                 : this->m_stor.mut_front();
      }
  };

template<typename valueT>
inline void
swap(optional<valueT>& lhs, optional<valueT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

template<typename valueT>
constexpr bool
operator==(const optional<valueT>& lhs, const optional<valueT>& rhs)
  noexcept(noexcept(::std::declval<const valueT&>() == ::std::declval<const valueT&>()))
  { return (lhs && rhs) ? (*lhs == *rhs) : (!lhs == !rhs);  }

template<typename valueT>
constexpr bool
operator!=(const optional<valueT>& lhs, const optional<valueT>& rhs)
  noexcept(noexcept(::std::declval<const valueT&>() != ::std::declval<const valueT&>()))
  { return (lhs && rhs) ? (*lhs != *rhs) : (!lhs != !rhs);  }

template<typename valueT>
constexpr bool
operator==(const optional<valueT>& lhs, nullopt_t) noexcept
  { return !lhs;  }

template<typename valueT>
constexpr bool
operator!=(const optional<valueT>& lhs, nullopt_t) noexcept
  { return !!lhs;  }

template<typename valueT>
constexpr bool
operator==(nullopt_t, const optional<valueT>& rhs) noexcept
  { return !rhs;  }

template<typename valueT>
constexpr bool
operator!=(nullopt_t, const optional<valueT>& rhs) noexcept
  { return !!rhs;  }

template<typename valueT>
constexpr bool
operator==(const optional<valueT>& lhs, const valueT& rhs)
  noexcept(noexcept(::std::declval<const valueT&>() == ::std::declval<const valueT&>()))
  { return !!lhs && (*lhs == rhs);  }

template<typename valueT>
constexpr bool
operator!=(const optional<valueT>& lhs, const valueT& rhs)
  noexcept(noexcept(::std::declval<const valueT&>() != ::std::declval<const valueT&>()))
  { return !lhs || (*lhs != rhs);  }

template<typename valueT>
constexpr bool
operator==(const valueT& lhs, const optional<valueT>& rhs)
  noexcept(noexcept(::std::declval<const valueT&>() == ::std::declval<const valueT&>()))
  { return !!rhs && (lhs == *rhs);  }

template<typename valueT>
constexpr bool
operator!=(const valueT& lhs, const optional<valueT>& rhs)
  noexcept(noexcept(::std::declval<const valueT&>() != ::std::declval<const valueT&>()))
  { return !rhs || (lhs != *rhs);  }

template<typename charT, typename traitsT, typename valueT>
inline basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const optional<valueT>& rhs)
  { return rhs ? (fmt << *rhs) : (fmt << "[no value]");  }

}  // namespace rocket

#endif
