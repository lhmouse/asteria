// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#define ROCKET_TINYBUF_STR_NO_EXTERN_TEMPLATE_ 1
#include "tinybuf_str.hpp"
namespace rocket {

template class basic_tinybuf_str<char>;
template class basic_tinybuf_str<wchar_t>;
template class basic_tinybuf_str<char16_t>;
template class basic_tinybuf_str<char32_t>;

}  // namespace rocket
