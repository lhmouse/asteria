// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "format.hpp"
#include "cow_string.hpp"
namespace rocket {

template tinyfmt& vformat(tinyfmt&, const char*, size_t, const formatter*, size_t);
template tinyfmt& vformat(tinyfmt&, const char*, const formatter*, size_t);
template tinyfmt& vformat(tinyfmt&, const cow_string&, const formatter*, size_t);

}  // namespace rocket
