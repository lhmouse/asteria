// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "format.hpp"

namespace rocket {

template tinyfmt&  vformat(tinyfmt&  fmt, const char*    templ, const formatter*  pinsts, size_t ninsts);
template wtinyfmt& vformat(wtinyfmt& fmt, const wchar_t* templ, const wformatter* pinsts, size_t ninsts);

}  // namespace rocket
