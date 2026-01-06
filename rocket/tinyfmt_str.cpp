// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "tinyfmt_str.hpp"
namespace rocket {

template class basic_tinyfmt_str<char>;
template class basic_tinyfmt_str<wchar_t>;
template class basic_tinyfmt_str<char16_t>;
template class basic_tinyfmt_str<char32_t>;

}  // namespace rocket
