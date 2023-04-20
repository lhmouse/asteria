// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "prehashed_string.hpp"
namespace rocket {

template class basic_prehashed_string<cow_string, cow_string::hash>;
template class basic_prehashed_string<cow_wstring, cow_wstring::hash>;
template class basic_prehashed_string<cow_u16string, cow_u16string::hash>;
template class basic_prehashed_string<cow_u32string, cow_u32string::hash>;

}  // namespace rocket
