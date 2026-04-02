// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../../rocket/tinyfmt_str.hpp"
namespace asteria {

template class basic_tinyfmt_str<char>;
template class basic_tinyfmt_str<wchar_t>;
template class basic_tinyfmt_str<char16_t>;
template class basic_tinyfmt_str<char32_t>;

}  // namespace asteria
