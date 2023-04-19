// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#define ROCKET_TINYFMT_FILE_NO_EXTERN_TEMPLATE_ 1
#include "tinyfmt_file.hpp"
namespace rocket {

template class basic_tinyfmt_file<char>;
template class basic_tinyfmt_file<wchar_t>;
template class basic_tinyfmt_file<char16_t>;
template class basic_tinyfmt_file<char32_t>;

}  // namespace rocket
