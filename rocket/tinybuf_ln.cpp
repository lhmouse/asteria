// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "tinybuf_ln.hpp"
namespace rocket {

template class basic_tinybuf_ln<char>;
template class basic_tinybuf_ln<wchar_t>;
template class basic_tinybuf_ln<char16_t>;
template class basic_tinybuf_ln<char32_t>;

}  // namespace rocket
