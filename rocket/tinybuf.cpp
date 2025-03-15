// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "tinybuf.hpp"
namespace rocket {

template class basic_tinybuf<char>;
template class basic_tinybuf<wchar_t>;
template class basic_tinybuf<uint16_t>;
template class basic_tinybuf<uint32_t>;

}  // namespace rocket
