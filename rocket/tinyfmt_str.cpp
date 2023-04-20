// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "tinyfmt_str.hpp"
namespace rocket {

template class basic_tinyfmt_str<char>;
template class basic_tinyfmt_str<wchar_t>;
template class basic_tinyfmt_str<char16_t>;
template class basic_tinyfmt_str<char32_t>;

}  // namespace rocket
