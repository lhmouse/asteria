// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#define ROCKET_COW_STRING_NO_EXTERN_TEMPLATE_ 1
#include "cow_string.hpp"
namespace rocket {

template class basic_shallow_string<char>;
template class basic_shallow_string<wchar_t>;
template class basic_shallow_string<char16_t>;
template class basic_shallow_string<char32_t>;

template class basic_cow_string<char>;
template class basic_cow_string<wchar_t>;
template class basic_cow_string<char16_t>;
template class basic_cow_string<char32_t>;

}  // namespace rocket
