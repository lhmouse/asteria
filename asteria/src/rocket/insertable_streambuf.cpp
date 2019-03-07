// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "insertable_streambuf.hpp"

namespace rocket {

template class basic_insertable_streambuf<char>;
template class basic_insertable_streambuf<wchar_t>;

}  // namespace rocket
