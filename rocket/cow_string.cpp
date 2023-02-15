// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "cow_string.hpp"
namespace rocket {

template class basic_cow_string<char>;
template class basic_cow_string<wchar_t>;
template class basic_cow_string<char16_t>;
template class basic_cow_string<char32_t>;

}  // namespace rocket
