// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#define ROCKET_TINYBUF_NO_EXTERN_TEMPLATE_ 1
#include "tinybuf.hpp"
namespace rocket {

template class basic_tinybuf<char>;
template class basic_tinybuf<wchar_t>;
template class basic_tinybuf<uint16_t>;
template class basic_tinybuf<uint32_t>;

}  // namespace rocket
