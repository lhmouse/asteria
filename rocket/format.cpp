// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#define ROCKET_FORMAT_NO_EXTERN_TEMPLATE_  1
#include "format.hpp"
namespace rocket {

template tinyfmt& vformat(tinyfmt&, const char*, const formatter*, size_t);
template wtinyfmt& vformat(wtinyfmt&, const wchar_t*, const wformatter*, size_t);
template u16tinyfmt& vformat(u16tinyfmt&, const char16_t*, const u16formatter*, size_t);
template u32tinyfmt& vformat(u32tinyfmt&, const char32_t*, const u32formatter*, size_t);

}  // namespace rocket
