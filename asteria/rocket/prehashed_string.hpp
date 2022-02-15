// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_PREHASHED_STRING_HPP_
#define ROCKET_PREHASHED_STRING_HPP_

#include "cow_string.hpp"

namespace rocket {

template<typename stringT, typename hashT = hash<stringT>, typename eqT = equal_to<void>>
class basic_prehashed_string;

#include "details/prehashed_string.ipp"

template<typename stringT, typename hashT, typename eqT>
class basic_prehashed_string
  {
    static_assert(noexcept(::std::declval<const hashT&>()(::std::declval<const stringT&>())),
                  "hash operations must not throw exceptions");

  public:
    // types
    using string_type  = stringT;
    using hasher       = hashT;
    using key_equal    = eqT;

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
    details_prehashed_string::string_storage<string_type, hasher, key_equal> m_sth;

  public:
    constexpr
    basic_prehashed_string()
      noexcept(conjunction<is_nothrow_constructible<string_type>,
                           is_nothrow_constructible<hasher>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_constructible<key_equal>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(hasher(), key_equal())
      { }

    template<typename... paramsT>
    explicit constexpr
    basic_prehashed_string(const hasher& hf, const key_equal& eq, paramsT&&... params)
      noexcept(conjunction<is_nothrow_constructible<string_type, paramsT&&...>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(hf, eq, ::std::forward<paramsT>(params)...)
      { }

    explicit constexpr
    basic_prehashed_string(const string_type& str, const hasher& hf = hasher(),
                           const key_equal& eq = key_equal())
      noexcept(conjunction<is_nothrow_copy_constructible<string_type>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(hf, eq, str)
      { }

    constexpr
    basic_prehashed_string(string_type&& str, const hasher& hf = hasher(),
                           const key_equal& eq = key_equal())
      noexcept(conjunction<is_nothrow_move_constructible<string_type>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(hf, eq, ::std::move(str))
      { }

    template<typename xstringT,
    ROCKET_ENABLE_IF(is_convertible<xstringT&, string_type>::value)>
    explicit constexpr
    basic_prehashed_string(xstringT& xstr, const hasher& hf = hasher(),
                           const key_equal& eq = key_equal())
      noexcept(conjunction<is_nothrow_constructible<string_type, xstringT&>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(hf, eq, xstr)
      { }

    template<typename xstringT,
    ROCKET_ENABLE_IF(is_convertible<xstringT&&, string_type>::value)>
    constexpr
    basic_prehashed_string(xstringT&& xstr, const hasher& hf = hasher(),
                           const key_equal& eq = key_equal())
      noexcept(conjunction<is_nothrow_constructible<string_type, xstringT&&>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(hf, eq, ::std::forward<xstringT>(xstr))
      { }

    constexpr
    basic_prehashed_string(initializer_list<value_type> init, const hasher& hf = hasher(),
                           const key_equal& eq = key_equal())
      : m_sth(hf, eq, init)
      { }

    basic_prehashed_string(const basic_prehashed_string& other)
      noexcept(conjunction<is_nothrow_constructible<string_type>,
                           is_nothrow_copy_assignable<string_type>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(other.m_sth.as_hasher(), other.m_sth.as_key_equal())
      { this->m_sth.share_with(other.m_sth);  }

    basic_prehashed_string(basic_prehashed_string&& other)
      noexcept(conjunction<is_nothrow_constructible<string_type>,
                           is_nothrow_move_assignable<string_type>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(other.m_sth.as_hasher(), other.m_sth.as_key_equal())
      { this->m_sth.exchange_with(other.m_sth);  }

    basic_prehashed_string&
    operator=(initializer_list<value_type> init)
      { this->m_sth.set_string(init);
        return *this;  }

    basic_prehashed_string&
    operator=(const basic_prehashed_string& other)
      noexcept(is_nothrow_copy_assignable<string_type>::value)
      { this->m_sth.share_with(other.m_sth);
        return *this;  }

    basic_prehashed_string&
    operator=(basic_prehashed_string&& other)
      noexcept(is_nothrow_move_assignable<string_type>::value)
      { this->m_sth.exchange_with(other.m_sth);
        return *this;  }

    basic_prehashed_string&
    swap(basic_prehashed_string& other) noexcept(is_nothrow_swappable<string_type>::value)
      { this->m_sth.exchange_with(other.m_sth);
        return *this;  }

  public:
    // getters
    constexpr const string_type&
    rdstr() const noexcept
      { return this->m_sth.str();  }

    operator
    const string_type&() const noexcept
      { return this->m_sth.str();  }

    constexpr size_t
    rdhash() const noexcept
      { return this->m_sth.hval();  }

    constexpr bool
    equals(const basic_prehashed_string& other) const
      noexcept(noexcept(::std::declval<const key_equal&>()(
            ::std::declval<const string_type&>(), ::std::declval<const string_type&>())))
      {
        return (this->m_sth.hval() == other.m_sth.hval())
               && this->m_sth.as_key_equal()(this->m_sth.str(), other.m_sth.str());
      }

    template<typename otherT>
    constexpr bool
    equals(const otherT& other) const
      noexcept(noexcept(::std::declval<const key_equal&>()(
            ::std::declval<const string_type&>(), ::std::declval<const otherT&>())))
      {
        return this->m_sth.as_key_equal()(this->m_sth.str(), other);
      }

    // 24.3.2.3, iterators
    const_iterator
    begin() const noexcept
      { return this->m_sth.str().begin();  }

    const_iterator
    end() const noexcept
      { return this->m_sth.str().end();  }

    const_reverse_iterator
    rbegin() const noexcept
      { return this->m_sth.str().rbegin();  }

    const_reverse_iterator
    rend() const noexcept
      { return this->m_sth.str().rend();  }

    // 24.3.2.4, capacity
    constexpr bool
    empty() const noexcept
      { return this->m_sth.str().empty();  }

    constexpr size_type
    size() const noexcept
      { return this->m_sth.str().size();  }

    constexpr size_type
    length() const noexcept
      { return this->m_sth.str().length();  }

    // N.B. This is a non-standard extension.
    constexpr difference_type
    ssize() const noexcept
      { return static_cast<difference_type>(this->m_sth.str().size());  }

    constexpr size_type
    max_size() const noexcept
      { return this->m_sth.str().max_size();  }

    basic_prehashed_string&
    clear() noexcept(noexcept(::std::declval<string_type&>().clear()))
      {
        this->m_sth.clear();
        return *this;
      }

    // 24.3.2.5, element access
    const_reference
    at(size_type pos) const
      { return this->m_sth.str().at(pos);  }

    const_reference
    operator[](size_type pos) const noexcept
      { return this->m_sth.str()[pos];  }

    const_reference
    front() const noexcept
      { return this->m_sth.str().front();  }

    const_reference
    back() const noexcept
      { return this->m_sth.str().back();  }

    template<typename... paramsT>
    basic_prehashed_string&
    assign(paramsT&&... params)
      {
        this->m_sth.assign(::std::forward<paramsT>(params)...);
        return *this;
      }

    size_type
    copy(size_type tpos, value_type* s, size_type tn) const
      { return this->m_sth.str().copy(tpos, s, tn);  }

    size_type
    copy(value_type* s, size_type tn) const
      { return this->m_sth.str().copy(s, tn);  }

    // 24.3.2.7, string operations
    constexpr const value_type*
    data() const noexcept
      { return this->m_sth.str().data();  }

    constexpr const value_type*
    c_str() const noexcept
      { return this->m_sth.str().c_str();  }
  };

template<typename stringT, typename hashT, typename eqT>
struct basic_prehashed_string<stringT, hashT, eqT>::hash
  {
    using result_type    = size_t;
    using argument_type  = basic_prehashed_string;

    constexpr result_type
    operator()(const argument_type& str) const noexcept
      { return str.rdhash();  }
  };

extern template
class basic_prehashed_string<cow_string, cow_string::hash>;

extern template
class basic_prehashed_string<cow_wstring, cow_wstring::hash>;

extern template
class basic_prehashed_string<cow_u16string, cow_u16string::hash>;

extern template
class basic_prehashed_string<cow_u32string, cow_u32string::hash>;

template<typename stringT, typename hashT, typename eqT>
constexpr bool
operator==(const basic_prehashed_string<stringT, hashT, eqT>& lhs,
           const basic_prehashed_string<stringT, hashT, eqT>& rhs)
  noexcept(noexcept(lhs.equals(rhs)))
  { return lhs.equals(rhs);  }

template<typename stringT, typename hashT, typename eqT, typename otherT>
constexpr bool
operator==(const basic_prehashed_string<stringT, hashT, eqT>& lhs, const otherT& rhs)
  noexcept(noexcept(lhs.equals(rhs)))
  { return lhs.equals(rhs);  }

template<typename stringT, typename hashT, typename eqT, typename otherT>
constexpr bool
operator==(const otherT& lhs, const basic_prehashed_string<stringT, hashT, eqT>& rhs)
  noexcept(noexcept(rhs.equals(lhs)))
  { return rhs.equals(lhs);  }

template<typename stringT, typename hashT, typename eqT>
constexpr bool
operator!=(const basic_prehashed_string<stringT, hashT, eqT>& lhs,
           const basic_prehashed_string<stringT, hashT, eqT>& rhs)
  noexcept(noexcept(lhs.equals(rhs)))
  { return !lhs.equals(rhs);  }

template<typename stringT, typename hashT, typename eqT, typename otherT>
constexpr bool
operator!=(const basic_prehashed_string<stringT, hashT, eqT>& lhs, const otherT& rhs)
  noexcept(noexcept(lhs.equals(rhs)))
  { return !lhs.equals(rhs);  }

template<typename stringT, typename hashT, typename eqT, typename otherT>
constexpr bool
operator!=(const otherT& lhs, const basic_prehashed_string<stringT, hashT, eqT>& rhs)
  noexcept(noexcept(rhs.equals(lhs)))
  { return !rhs.equals(lhs);  }

template<typename stringT, typename hashT, typename eqT>
inline void
swap(basic_prehashed_string<stringT, hashT, eqT>& lhs,
     basic_prehashed_string<stringT, hashT, eqT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

template<typename charT, typename traitsT, typename stringT, typename hashT, typename eqT>
inline basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt,
           const basic_prehashed_string<stringT, hashT, eqT>& str)
  { return fmt << str.rdstr();  }

using prehashed_string     = basic_prehashed_string<cow_string,    cow_string::hash>;
using prehashed_wstring    = basic_prehashed_string<cow_wstring,   cow_wstring::hash>;
using prehashed_u16string  = basic_prehashed_string<cow_u16string, cow_u16string::hash>;
using prehashed_u32string  = basic_prehashed_string<cow_u32string, cow_u32string::hash>;

}  // namespace rocket

#endif
