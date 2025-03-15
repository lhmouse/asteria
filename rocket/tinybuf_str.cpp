// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "tinybuf_str.hpp"
namespace rocket {

template class basic_tinybuf_str<char>;
template class basic_tinybuf_str<wchar_t>;
template class basic_tinybuf_str<char16_t>;
template class basic_tinybuf_str<char32_t>;

}  // namespace rocket
