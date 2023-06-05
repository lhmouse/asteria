// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "tinybuf_file.hpp"
namespace rocket {

template class basic_tinybuf_file<char>;
template class basic_tinybuf_file<wchar_t>;
template class basic_tinybuf_file<char16_t>;
template class basic_tinybuf_file<char32_t>;

}  // namespace rocket
