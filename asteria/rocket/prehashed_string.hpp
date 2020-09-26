// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_PREHASHED_STRING_HPP_
#define ROCKET_PREHASHED_STRING_HPP_

#include "cow_string.hpp"

namespace rocket {

template<typename stringT, typename hashT = hash<stringT>>
class basic_prehashed_string;

#include "details/prehashed_string.ipp"

template<typename stringT, typename hashT>
class basic_prehashed_string
  {
    static_assert(noexcept(::std::declval<const hashT&>()(::std::declval<const stringT&>())),
                  "hash operations must not throw exceptions");

  public:
    // types
    using string_type  = stringT;
    using hasher       = hashT;

    using value_type      = typename string_type::value_type;
    using traits_type     = typename string_type::traits_type;
    using allocator_type  = typename string_type::allocator_type;

    using size_type        = typename string_type::size_type;
    using difference_type  = typename string_type::difference_type;
    using const_reference  = typename string_type::const_reference;
    using reference        = typename string_type::const_reference;

    using const_iterator          = typename string_type::const_iterator;
    using iterator                = typename string_type::const_iterator;
    using const_reverse_iterator  = typename string_type::const_reverse_iterator;
    using reverse_iterator        = typename string_type::const_reverse_iterator;

    // hash support
    struct hash;

  private:
    details_prehashed_string::string_storage<string_type, hasher> m_sth;

  public:
    constexpr
    basic_prehashed_string()
    noexcept(conjunction<is_nothrow_constructible<string_type>,
                         is_nothrow_constructible<hasher>,
                         is_nothrow_copy_constructible<hasher>>::value)
      : m_sth(hasher())
      { }

    template<typename... paramsT>
    explicit constexpr
    basic_prehashed_string(const hasher& hf, paramsT&&... params)
    noexcept(conjunction<is_nothrow_constructible<string_type, paramsT&&...>,
                         is_nothrow_copy_constructible<hasher>>::value)
      : m_sth(hf, ::std::forward<paramsT>(params)...)
      { }

    explicit constexpr
    basic_prehashed_string(const string_type& str, const hasher& hf = hasher())
    noexcept(conjunction<is_nothrow_copy_constructible<string_type>,
                         is_nothrow_copy_constructible<hasher>>::value)
      : m_sth(hf, str)
      { }

    constexpr
    basic_prehashed_string(string_type&& str, const hasher& hf = hasher())
    noexcept(conjunction<is_nothrow_move_constructible<string_type>,
                         is_nothrow_copy_constructible<hasher>>::value)
      : m_sth(hf, ::std::move(str))
      { }

    template<typename xstringT,
    ROCKET_ENABLE_IF(is_convertible<xstringT&, string_type>::value)>
    explicit constexpr
    basic_prehashed_string(xstringT& xstr, const hasher& hf = hasher())
    noexcept(conjunction<is_nothrow_constructible<string_type, xstringT&>,
                         is_nothrow_copy_constructible<hasher>>::value)
      : m_sth(hf, xstr)
      { }

    template<typename xstringT,
    ROCKET_ENABLE_IF(is_convertible<xstringT&&, string_type>::value)>
    constexpr
    basic_prehashed_string(xstringT&& xstr, const hasher& hf = hasher())
    noexcept(conjunction<is_nothrow_constructible<string_type, xstringT&&>,
                         is_nothrow_copy_constructible<hasher>>::value)
      : m_sth(hf, ::std::forward<xstringT>(xstr))
      { }

    constexpr
    basic_prehashed_string(initializer_list<value_type> init, const hasher& hf = hasher())
      : m_sth(hf, init)
      { }

    basic_prehashed_string(const basic_prehashed_string& other)
    noexcept(conjunction<is_nothrow_constructible<string_type>,
                         is_nothrow_copy_assignable<string_type>,
                         is_nothrow_copy_constructible<hasher>>::value)
      : m_sth(other.m_sth.as_hasher())
      { this->m_sth.assign(other.m_sth);  }

    basic_prehashed_string(basic_prehashed_string&& other)
    noexcept(conjunction<is_nothrow_constructible<string_type>,
                         is_nothrow_move_assignable<string_type>,
                         is_nothrow_copy_constructible<hasher>>::value)
      : m_sth(other.m_sth.as_hasher())
      { this->m_sth.assign(::std::move(other.m_sth));  }

    basic_prehashed_string&
    operator=(initializer_list<value_type> init)
      {
        this->m_sth.assign(init);
        return *this;
      }

    basic_prehashed_string&
    operator=(const basic_prehashed_string& other)
    noexcept(is_nothrow_copy_assignable<string_type>::value)
      {
        this->m_sth.assign(other.m_sth);
        return *this;
      }

    basic_prehashed_string&
    operator=(basic_prehashed_string&& other)
    noexcept(is_nothrow_move_assignable<string_type>::value)
      {
        this->m_sth.assign(::std::move(other.m_sth));
        return *this;
      }

  public:
    // getters
    constexpr
    const string_type&
    rdstr()
    const noexcept
      { return this->m_sth.str();  }

    operator
    const string_type&()
    const noexcept
      { return this->m_sth.str();  }

    constexpr
    size_t
    rdhash()
    const noexcept
      { return this->m_sth.hval();  }

    // 24.3.2.3, iterators
    const_iterator
    begin()
    const noexcept
      { return this->m_sth.str().begin();  }

    const_iterator
    end()
    const noexcept
      { return this->m_sth.str().end();  }

    const_reverse_iterator
    rbegin()
    const noexcept
      { return this->m_sth.str().rbegin();  }

    const_reverse_iterator
    rend()
    const noexcept
      { return this->m_sth.str().rend();  }

    // 24.3.2.4, capacity
    bool
    empty()
    const noexcept
      { return this->m_sth.str().empty();  }

    size_type
    size()
    const noexcept
      { return this->m_sth.str().size();  }

    size_type
    length()
    const noexcept
      { return this->m_sth.str().length();  }

    // N.B. This is a non-standard extension.
    difference_type
    ssize()
    const noexcept
      { return static_cast<difference_type>(this->m_sth.str().size());  }

    size_type
    max_size()
    const noexcept
      { return this->m_sth.str().max_size();  }

    basic_prehashed_string&
    clear()
    noexcept(noexcept(::std::declval<string_type&>().clear()))
      {
        this->m_sth.clear();
        return *this;
      }

    // 24.3.2.5, element access
    const_reference
    at(size_type pos)
    const
      { return this->m_sth.str().at(pos);  }

    const_reference
    operator[](size_type pos)
    const noexcept
      { return this->m_sth.str()[pos];  }

    const_reference
    front()
    const noexcept
      { return this->m_sth.str().front();  }

    const_reference
    back()
    const noexcept
      { return this->m_sth.str().back();  }

    basic_prehashed_string&
    assign(const basic_prehashed_string& other)
    noexcept(is_nothrow_copy_constructible<string_type>::value)
      {
        this->m_sth.assign(other.m_sth);
        return *this;
      }

    basic_prehashed_string&
    assign(basic_prehashed_string&& other)
    noexcept(is_nothrow_move_constructible<string_type>::value)
      {
        this->m_sth.assign(::std::move(other.m_sth));
        return *this;
      }

    template<typename... paramsT>
    basic_prehashed_string&
    assign(paramsT&&... params)
      {
        this->m_sth.assign(::std::forward<paramsT>(params)...);
        return *this;
      }

    basic_prehashed_string&
    assign(initializer_list<value_type> init)
      {
        this->m_sth.assign(init);
        return *this;
      }

    size_type
    copy(value_type* s, size_type tn, size_type tpos = 0)
    const
      { return this->m_sth.str().copy(s, tn, tpos);  }

    basic_prehashed_string&
    swap(basic_prehashed_string& other)
    noexcept(is_nothrow_swappable<string_type>::value)
      {
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }

    // 24.3.2.7, string operations
    constexpr const value_type*
    data()
    const noexcept
      { return this->m_sth.str().data();  }

    const value_type*
    c_str()
    const noexcept
      { return this->m_sth.str().c_str();  }
  };

template<typename stringT, typename hashT>
struct basic_prehashed_string<stringT, hashT>::hash
  {
    using result_type    = size_t;
    using argument_type  = basic_prehashed_string;

    constexpr
    result_type
    operator()(const argument_type& str)
    const noexcept
      { return str.rdhash();  }
  };

template class
basic_prehashed_string<cow_string, cow_string::hash>;

template class
basic_prehashed_string<cow_wstring, cow_wstring::hash>;

template class
basic_prehashed_string<cow_u16string, cow_u16string::hash>;

template class
basic_prehashed_string<cow_u32string, cow_u32string::hash>;

template<typename stringT, typename hashT>
constexpr
bool
operator==(const basic_prehashed_string<stringT, hashT>& lhs, const basic_prehashed_string<stringT, hashT>& rhs)
  { return (lhs.rdhash() == rhs.rdhash()) && (lhs.rdstr() == rhs.rdstr());  }

template<typename stringT, typename hashT>
constexpr
bool
operator!=(const basic_prehashed_string<stringT, hashT>& lhs, const basic_prehashed_string<stringT, hashT>& rhs)
  { return (lhs.rdhash() != rhs.rdhash()) || (lhs.rdstr() != rhs.rdstr());  }

template<typename stringT, typename hashT, typename otherT,
ROCKET_ENABLE_IF_HAS_TYPE(decltype(::std::declval<const stringT&>() == ::std::declval<const otherT&>()))>
constexpr
bool
operator==(const basic_prehashed_string<stringT, hashT>& lhs, const otherT& rhs)
  { return lhs.rdstr() == rhs;  }

template<typename stringT, typename hashT, typename otherT,
ROCKET_ENABLE_IF_HAS_TYPE(decltype(::std::declval<const stringT&>() != ::std::declval<const otherT&>()))>
constexpr
bool
operator!=(const basic_prehashed_string<stringT, hashT>& lhs, const otherT& rhs)
  { return lhs.rdstr() != rhs;  }

template<typename stringT, typename hashT, typename otherT,
ROCKET_ENABLE_IF_HAS_TYPE(decltype(::std::declval<const otherT&>() == ::std::declval<const stringT&>()))>
constexpr
bool
operator==(const otherT& lhs, const basic_prehashed_string<stringT, hashT>& rhs)
  { return lhs == rhs.rdstr();  }

template<typename stringT, typename hashT, typename otherT,
ROCKET_ENABLE_IF_HAS_TYPE(decltype(::std::declval<const otherT&>() != ::std::declval<const stringT&>()))>
constexpr
bool
operator!=(const otherT& lhs, const basic_prehashed_string<stringT, hashT>& rhs)
  { return lhs != rhs.rdstr();  }

template<typename stringT, typename hashT>
inline
void
swap(basic_prehashed_string<stringT, hashT>& lhs, basic_prehashed_string<stringT, hashT>& rhs)
noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

template<typename charT, typename traitsT, typename stringT, typename hashT>
inline
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const basic_prehashed_string<stringT, hashT>& str)
  { return fmt << str.rdstr();  }

using prehashed_string     = basic_prehashed_string<cow_string,    cow_string::hash>;
using prehashed_wstring    = basic_prehashed_string<cow_wstring,   cow_wstring::hash>;
using prehashed_u16string  = basic_prehashed_string<cow_u16string, cow_u16string::hash>;
using prehashed_u32string  = basic_prehashed_string<cow_u32string, cow_u32string::hash>;

}  // namespace rocket

#endif
