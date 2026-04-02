// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../../rocket/tinyfmt_file.hpp"
namespace asteria {

template class basic_tinyfmt_file<char>;
template class basic_tinyfmt_file<wchar_t>;
template class basic_tinyfmt_file<char16_t>;
template class basic_tinyfmt_file<char32_t>;

}  // namespace asteria
