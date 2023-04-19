// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#define ROCKET_LINEAR_BUFFER_NO_EXTERN_TEMPLATE_ 1
#include "linear_buffer.hpp"
namespace rocket {

template class basic_linear_buffer<char>;
template class basic_linear_buffer<wchar_t>;
template class basic_linear_buffer<char16_t>;
template class basic_linear_buffer<char32_t>;

}  // namespace rocket
