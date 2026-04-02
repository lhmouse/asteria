// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../../rocket/cow_string.hpp"
namespace asteria {

template class basic_shallow_string<char>;
template class basic_shallow_string<unsigned char>;
template class basic_shallow_string<wchar_t>;
template class basic_shallow_string<char16_t>;
template class basic_shallow_string<char32_t>;

template class basic_cow_string<char>;
template class basic_cow_string<unsigned char>;
template class basic_cow_string<wchar_t>;
template class basic_cow_string<char16_t>;
template class basic_cow_string<char32_t>;

}  // namespace asteria
