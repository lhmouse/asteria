// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "format.hpp"

namespace rocket {

template
tinyfmt&
vformat(tinyfmt&, const char*, size_t, const formatter*, size_t);

template
wtinyfmt&
vformat(wtinyfmt&, const wchar_t*, size_t, const wformatter*, size_t);

}  // namespace rocket
