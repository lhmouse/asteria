// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "tinybuf_file.hpp"
namespace rocket {

template class basic_tinybuf_file<char>;
template class basic_tinybuf_file<wchar_t>;
template class basic_tinybuf_file<char16_t>;
template class basic_tinybuf_file<char32_t>;

}  // namespace rocket
