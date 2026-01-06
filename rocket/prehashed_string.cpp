// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "prehashed_string.hpp"
namespace rocket {

template class basic_prehashed_string<cow_string, cow_string::hasher>;
template class basic_prehashed_string<cow_wstring, cow_wstring::hasher>;
template class basic_prehashed_string<cow_u16string, cow_u16string::hasher>;
template class basic_prehashed_string<cow_u32string, cow_u32string::hasher>;

}  // namespace rocket
