// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "linear_buffer.hpp"
namespace rocket {

template class basic_linear_buffer<char>;
template class basic_linear_buffer<wchar_t>;
template class basic_linear_buffer<char16_t>;
template class basic_linear_buffer<char32_t>;

}  // namespace rocket
