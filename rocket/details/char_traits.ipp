// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CHAR_TRAITS_
#  error Please include <rocket/char_traits.hpp> instead.
#endif
namespace details_char_traits {

template<typename charT>
constexpr
charT*
assign(charT* p, size_t n, charT c) noexcept
  {
    for(auto bp = p, ep = p + n;  bp != ep;  ++bp)
      *bp = c;
    return p;
  }

template<typename charT>
constexpr
const charT*
find(const charT* p, size_t n, charT c) noexcept
  {
    for(auto bp = p, ep = p + n;  bp != ep;  ++bp)
      if(*bp == c)
        return bp;
    return nullptr;
  }

template<typename charT>
constexpr
size_t
length(const charT* p) noexcept
  {
    for(auto bp = p;  true;  ++bp)
      if(*bp == charT())
        return static_cast<size_t>(bp - p);
  }

template<typename charT>
constexpr
typename make_unsigned<charT>::type
ucast(charT c) noexcept
  {
    return static_cast<typename make_unsigned<charT>::type>(c);
  }

template<typename charT>
constexpr
int
compare(const charT* p, const charT* s, size_t n) noexcept
  {
    for(size_t k = 0;  k != n;  ++k)
      if(p[k] != s[k])
        return ((ucast)(p[k]) < (ucast)(s[k])) ? -1 : +1;
    return 0;
  }

}  // namespace details_char_traits
