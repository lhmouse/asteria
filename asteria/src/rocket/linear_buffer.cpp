// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../../rocket/linear_buffer.hpp"
namespace asteria {

template class basic_linear_buffer<char>;
template class basic_linear_buffer<wchar_t>;
template class basic_linear_buffer<char16_t>;
template class basic_linear_buffer<char32_t>;

}  // namespace asteria
