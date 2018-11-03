// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_PREHASHED_STRING_HPP_
#define ROCKET_PREHASHED_STRING_HPP_

#include <type_traits> // so many...
#include <initializer_list> // std::initializer_list<>
#include <functional> // std::hash<>
#include <iosfwd> // std::basic_ostream<>
#include <cstddef> // std::size_t
#include "compatibility.h"
#include "cow_string.hpp"

namespace rocket {

using ::std::is_nothrow_constructible;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_move_constructible;
using ::std::is_nothrow_copy_assignable;
using ::std::is_nothrow_move_assignable;
using ::std::is_convertible;
using ::std::remove_reference;
using ::std::initializer_list;
using ::std::hash;
using ::std::basic_ostream;
using ::std::size_t;

template<typename stringT, typename hashT = hash<stringT>>
  class basic_prehashed_string;

    namespace details_prehashed_string {

    template<typename stringT, typename hashT>
      class string_storage : private allocator_wrapper_base_for<hashT>::type
      {
      public:
        using string_type  = stringT;
        using hasher       = hashT;

      private:
        using hasher_base   = typename allocator_wrapper_base_for<hasher>::type;

      private:
        string_type m_str;
        size_t m_hval;

      public:
        template<typename ...paramsT>
          explicit string_storage(const hasher &hf, paramsT &&...params)
          : hasher_base(hf),
            m_str(::std::forward<paramsT>(params)...), m_hval(this->as_hasher()(this->m_str))
          {
          }

        string_storage(const string_storage &)
          = delete;
        string_storage & operator=(const string_storage &)
          = delete;

      public:
        const hasher & as_hasher() const noexcept
          {
            return static_cast<const hasher_base &>(*this);
          }
        hasher & as_hasher() noexcept
          {
            return static_cast<hasher_base &>(*this);
          }

        const string_type & str() const noexcept
          {
            return this->m_str;
          }
        size_t hval() const noexcept
          {
            return this->m_hval;
          }

        void clear()
          {
            this->m_str.clear();
            this->m_hval = this->as_hasher()(this->m_str);
          }

        template<typename ...paramsT>
          void assign(paramsT &&...params)
          {
            this->m_str.assign(::std::forward<paramsT>(params)...);
            this->m_hval = this->as_hasher()(this->m_str);
          }
        void assign(const string_storage &other)
          {
            this->m_str = other.m_str;
            this->m_hval = other.m_hval;
          }
        void assign(string_storage &&other)
          {
            this->m_str = ::std::move(other.m_str);
            this->m_hval = this->as_hasher()(this->m_str);
          }
        void exchange_with(string_storage &other)
          {
            noadl::adl_swap(this->m_str, other.m_str);
            ::std::swap(this->m_hval, other.m_hval);
          }
      };

    }

template<typename stringT, typename hashT>
  class basic_prehashed_string
  {
    static_assert(noexcept(::std::declval<const hashT &>()(::std::declval<const stringT &>())), "The hash operation shall not throw exceptions.");

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

    // associative container support
    struct equal_to;
    struct hash;

  private:
    details_prehashed_string::string_storage<string_type, hasher> m_sth;

  public:
    basic_prehashed_string() noexcept(is_nothrow_constructible<string_type>::value &&
                                      is_nothrow_constructible<hasher>::value && is_nothrow_copy_constructible<hasher>::value)
      : m_sth(hasher())
      {
      }
    template<typename ...paramsT>
      explicit basic_prehashed_string(const hasher &hf, paramsT &&...params) noexcept(is_nothrow_constructible<string_type, paramsT &&...>::value &&
                                                                                      is_nothrow_copy_constructible<hasher>::value)
      : m_sth(hf, ::std::forward<paramsT>(params)...)
      {
      }
    explicit basic_prehashed_string(const string_type &str, const hasher &hf = hasher())
      : m_sth(hf, str)
      {
      }
    basic_prehashed_string(string_type &&str, const hasher &hf = hasher())
      : m_sth(hf, ::std::move(str))
      {
      }
    template<typename paramT, typename enable_if<is_convertible<typename remove_reference<paramT>::type &&, string_type>::value>::type * = nullptr>
      basic_prehashed_string(paramT &&param, const hasher &hf = hasher())
      : m_sth(hf, ::std::forward<paramT>(param))
      {
      }
    basic_prehashed_string(initializer_list<value_type> init, const hasher &hf = hasher())
      : m_sth(hf, init)
      {
      }
    basic_prehashed_string(const basic_prehashed_string &other) noexcept(is_nothrow_copy_constructible<string_type>::value &&
                                                                         is_nothrow_copy_constructible<hasher>::value)
      : m_sth(other.m_sth.as_hasher(), other.m_sth.str())
      {
      }
    basic_prehashed_string(basic_prehashed_string &&other) noexcept(is_nothrow_move_constructible<string_type>::value &&
                                                                    is_nothrow_copy_constructible<hasher>::value)
      : m_sth(other.m_sth.as_hasher(), ::std::move(other.m_sth.str()))
      {
      }
    basic_prehashed_string & operator=(const basic_prehashed_string &other) noexcept(is_nothrow_copy_assignable<string_type>::value)
      {
        this->m_sth.assign(other.m_sth);
        return *this;
      }
    basic_prehashed_string & operator=(basic_prehashed_string &&other) noexcept(is_nothrow_move_assignable<string_type>::value)
      {
        this->m_sth.assign(::std::move(other.m_sth));
        return *this;
      }
    basic_prehashed_string & operator=(initializer_list<value_type> init)
      {
        this->m_sth.assign(init);
        return *this;
      }

  public:
    // getters
    const string_type & rdstr() const noexcept
      {
        return this->m_sth.str();
      }
    operator const string_type & () const noexcept
      {
        return this->m_sth.str();
      }
    size_t rdhash() const noexcept
      {
        return this->m_sth.hval();
      }

    // 24.3.2.3, iterators
    const_iterator begin() const noexcept
      {
        return this->m_sth.str().begin();
      }
    const_iterator end() const noexcept
      {
        return this->m_sth.str().end();
      }
    const_reverse_iterator rbegin() const noexcept
      {
        return this->m_sth.str().rbegin();
      }
    const_reverse_iterator rend() const noexcept
      {
        return this->m_sth.str().rend();
      }

    const_iterator cbegin() const noexcept
      {
        return this->begin();
      }
    const_iterator cend() const noexcept
      {
        return this->end();
      }
    const_reverse_iterator crbegin() const noexcept
      {
        return this->rbegin();
      }
    const_reverse_iterator crend() const noexcept
      {
        return this->rend();
      }

    // 24.3.2.4, capacity
    bool empty() const noexcept
      {
        return this->m_sth.str().empty();
      }
    size_type size() const noexcept
      {
        return this->m_sth.str().size();
      }
    size_type length() const noexcept
      {
        return this->m_sth.str().length();
      }
    size_type max_size() const noexcept
      {
        return this->m_sth.str().max_size();
      }
    void clear() noexcept(noexcept(::std::declval<string_type &>().clear()))
      {
        this->m_sth.clear();
      }

    // 24.3.2.5, element access
    const_reference at(size_type pos) const
      {
        return this->m_sth.str().at(pos);
      }
    const_reference operator[](size_type pos) const noexcept
      {
        return this->m_sth.str()[pos];
      }
    const_reference front() const noexcept
      {
        return this->m_sth.str().front();
      }
    const_reference back() const noexcept
      {
        return this->m_sth.str().back();
      }

    template<typename ...paramsT>
      basic_prehashed_string & assign(paramsT &&...params)
      {
        this->m_sth.assign(::std::forward<paramsT>(params)...);
        return *this;
      }
    basic_prehashed_string & assign(initializer_list<value_type> init)
      {
        this->m_sth.assign(init);
        return *this;
      }

    size_type copy(value_type *s, size_type tn, size_type tpos = 0) const
      {
        return this->m_sth.str().copy(s, tn, tpos);
      }

    void swap(basic_prehashed_string &other) noexcept(is_nothrow_swappable<string_type>::value)
      {
        this->m_sth.exchange_with(other.m_sth);
      }

    // 24.3.2.7, string operations
    const value_type * data() const noexcept
      {
        return this->m_sth.str().data();
      }
    const value_type * c_str() const noexcept
      {
        return this->m_sth.str().c_str();
      }
  };

template<typename stringT, typename hashT>
  struct basic_prehashed_string<stringT, hashT>::equal_to
  {
    using result_type           = bool;
    using first_argument_type   = basic_prehashed_string;
    using second_argument_type  = basic_prehashed_string;

    result_type operator()(const first_argument_type &lhs, const second_argument_type &rhs) const noexcept
      {
        return (lhs.rdhash() == rhs.rdhash()) && (lhs.rdstr() == rhs.rdstr());
      }
  };

template<typename stringT, typename hashT>
  struct basic_prehashed_string<stringT, hashT>::hash
  {
    using result_type    = size_t;
    using argument_type  = basic_prehashed_string;

    result_type operator()(const argument_type &str) const noexcept
      {
        return str.rdhash();
      }
  };

using prehashed_string     = basic_prehashed_string<cow_string,    cow_string::hash>;
using prehashed_wstring    = basic_prehashed_string<cow_wstring,   cow_wstring::hash>;
using prehashed_u16string  = basic_prehashed_string<cow_u16string, cow_u16string::hash>;
using prehashed_u32string  = basic_prehashed_string<cow_u32string, cow_u32string::hash>;

template<typename stringT, typename hashT>
  inline bool operator==(const basic_prehashed_string<stringT, hashT> &lhs, const basic_prehashed_string<stringT, hashT> &rhs) noexcept
  {
    return (lhs.rdhash() == rhs.rdhash()) && (lhs.rdstr() == rhs.rdstr());
  }
template<typename stringT, typename hashT>
  inline bool operator!=(const basic_prehashed_string<stringT, hashT> &lhs, const basic_prehashed_string<stringT, hashT> &rhs) noexcept
  {
    return (lhs.rdhash() != rhs.rdhash()) || (lhs.rdstr() != rhs.rdstr());
  }

template<typename stringT, typename hashT>
  inline bool operator==(const basic_prehashed_string<stringT, hashT> &lhs, const stringT &rhs) noexcept
  {
    return lhs.rdstr() == rhs;
  }
template<typename stringT, typename hashT>
  inline bool operator!=(const basic_prehashed_string<stringT, hashT> &lhs, const stringT &rhs) noexcept
  {
    return lhs.rdstr() != rhs;
  }

template<typename stringT, typename hashT>
  inline bool operator==(const stringT &lhs, const basic_prehashed_string<stringT, hashT> &rhs) noexcept
  {
    return lhs == rhs.rdstr();
  }
template<typename stringT, typename hashT>
  inline bool operator!=(const stringT &lhs, const basic_prehashed_string<stringT, hashT> &rhs) noexcept
  {
    return lhs != rhs.rdstr();
  }

template<typename stringT, typename hashT>
  inline bool operator==(const basic_prehashed_string<stringT, hashT> &lhs, const typename stringT::value_type *rhs) noexcept
  {
    return lhs.rdstr() == rhs;
  }
template<typename stringT, typename hashT>
  inline bool operator!=(const basic_prehashed_string<stringT, hashT> &lhs, const typename stringT::value_type *rhs) noexcept
  {
    return lhs.rdstr() != rhs;
  }

template<typename stringT, typename hashT>
  inline bool operator==(const typename stringT::value_type *lhs, const basic_prehashed_string<stringT, hashT> &rhs) noexcept
  {
    return lhs == rhs.rdstr();
  }
template<typename stringT, typename hashT>
  inline bool operator!=(const typename stringT::value_type *lhs, const basic_prehashed_string<stringT, hashT> &rhs) noexcept
  {
    return lhs != rhs.rdstr();
  }

template<typename stringT, typename hashT>
  inline void swap(basic_prehashed_string<stringT, hashT> &lhs, basic_prehashed_string<stringT, hashT> &rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

template<typename charT, typename traitsT, typename stringT, typename hashT>
  inline basic_ostream<charT, traitsT> & operator<<(basic_ostream<charT, traitsT> &os, const basic_prehashed_string<stringT, hashT> &str)
  {
    return os << str.rdstr();
  }

}

#endif
