// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ROCKET_XSTRING_
#define ASTERIA_ROCKET_XSTRING_

#include "fwd.hpp"
namespace asteria {

#include "details/xstring.ipp"

template<typename charT>
ASTERIA_PURE constexpr
size_t
xstrlen(charT* str)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ystrlen(str))
                           ? details_xstring::maybe_constexpr::ystrlen(str)
                           : details_xstring::ystrlen(str);
  }

template<typename charT>
ASTERIA_PURE constexpr
charT*
xstrchr(charT* str, ASTERIA_UNDEDUCED(charT) target)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ystrchr(str, target) == str)
                           ? details_xstring::maybe_constexpr::ystrchr(str, target)
                           : details_xstring::ystrchr(str, target);
  }

template<typename charT>
ASTERIA_PURE constexpr
charT*
xmemchr(charT* str, ASTERIA_UNDEDUCED(charT) target, size_t len)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ymemchr(str, target, len) == str)
                           ? details_xstring::maybe_constexpr::ymemchr(str, target, len)
                           : details_xstring::ymemchr(str, target, len);
  }

template<typename charT>
ASTERIA_PURE constexpr
int
xstrcmp(const charT* lhs, const charT* rhs)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ystrcmp(lhs, rhs))
                           ? details_xstring::maybe_constexpr::ystrcmp(lhs, rhs)
                           : details_xstring::ystrcmp(lhs, rhs);
  }

template<typename charT>
ASTERIA_PURE constexpr
bool
xstreq(const charT* lhs, const charT* rhs)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ystrcmp(lhs, rhs))
                          ? (details_xstring::maybe_constexpr::ystrcmp(lhs, rhs) == 0)
                          : (details_xstring::ystrcmp(lhs, rhs) == 0);
  }

template<typename charT>
ASTERIA_PURE constexpr
int
xmemcmp(const charT* lhs, const charT* rhs, size_t len)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ymemcmp(lhs, rhs, len))
                           ? details_xstring::maybe_constexpr::ymemcmp(lhs, rhs, len)
                           : details_xstring::ymemcmp(lhs, rhs, len);
  }

template<typename charT>
ASTERIA_PURE constexpr
int
xmemcmp(const charT* lhs, size_t llen, const charT* rhs, size_t rlen)
  noexcept
  {
    return (llen < rlen) ? ((noadl::xmemcmp(rhs, lhs, llen) < 0) ? 1 : -1)
         : (llen > rlen) ? ((noadl::xmemcmp(lhs, rhs, rlen) < 0) ? -1 : 1)
                         :   noadl::xmemcmp(lhs, rhs, llen);
  }

template<typename charT>
ASTERIA_PURE constexpr
bool
xmemeq(const charT* lhs, const charT* rhs, size_t len)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ymemcmp(lhs, rhs, len))
                          ? (details_xstring::maybe_constexpr::ymemcmp(lhs, rhs, len) == 0)
                          : ((lhs == rhs) || (::memcmp(lhs, rhs, sizeof(charT) * len) == 0));
  }

template<typename charT>
ASTERIA_PURE constexpr
bool
xmemeq(const charT* lhs, size_t llen, const charT* rhs, size_t rlen)
  noexcept
  {
    return (llen == rlen) && noadl::xmemeq(lhs, rhs, llen);
  }

template<typename charT>
constexpr
charT*
xmempset(charT* out, ASTERIA_UNDEDUCED(charT) elem, size_t len)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ymempset(out, elem, len) == out)
                           ? details_xstring::maybe_constexpr::ymempset(out, elem, len)
                           : details_xstring::ymempset(out, elem, len);
  }

template<typename charT>
inline
charT*
xmemrpset(charT*& out, ASTERIA_UNDEDUCED(charT) elem, size_t len)
  noexcept
  {
    return out = noadl::xmempset(out, elem, len);
  }

template<typename charT>
constexpr
charT*
xstrpcpy(charT* out, const ASTERIA_UNDEDUCED(charT)* str)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ystrpcpy(out, str) == out)
                           ? details_xstring::maybe_constexpr::ystrpcpy(out, str)
                           : details_xstring::ystrpcpy(out, str);
  }

template<typename charT>
inline
charT*
xstrrpcpy(charT*& out, const ASTERIA_UNDEDUCED(charT)* str)
  noexcept
  {
    return out = noadl::xstrpcpy(out, str);
  }

template<typename charT>
constexpr
charT*
xmempcpy(charT* out, const ASTERIA_UNDEDUCED(charT)* str, size_t len)
  noexcept
  {
    return ASTERIA_CONSTANT_P(details_xstring::maybe_constexpr::ymempcpy(out, str, len) == out)
                           ? details_xstring::maybe_constexpr::ymempcpy(out, str, len)
                           : details_xstring::ymempcpy(out, str, len);
  }

template<typename charT>
inline
charT*
xmemrpcpy(charT*& out, const ASTERIA_UNDEDUCED(charT)* str, size_t len)
  noexcept
  {
    return out = noadl::xmempcpy(out, str, len);
  }

}  // namespace asteria
#endif
