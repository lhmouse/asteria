// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "tinyfmt.hpp"

namespace rocket {

template
class basic_tinyfmt<char>;

template
class basic_tinyfmt<wchar_t>;

template
tinyfmt&
operator<<(tinyfmt&, char);

template
tinyfmt&
operator<<(tinyfmt&, const char*);

template
tinyfmt&
operator<<(tinyfmt&, const ascii_numput&);

template
tinyfmt&
operator<<(tinyfmt&, signed char);

template
tinyfmt&
operator<<(tinyfmt&, signed short);

template
tinyfmt&
operator<<(tinyfmt&, signed);

template
tinyfmt&
operator<<(tinyfmt&, signed long);

template
tinyfmt&
operator<<(tinyfmt&, signed long long);

template
tinyfmt&
operator<<(tinyfmt&, unsigned char);

template
tinyfmt&
operator<<(tinyfmt&, unsigned short);

template
tinyfmt&
operator<<(tinyfmt&, unsigned);

template
tinyfmt&
operator<<(tinyfmt&, unsigned long);

template
tinyfmt&
operator<<(tinyfmt&, unsigned long long);

template
tinyfmt&
operator<<(tinyfmt&, float);

template
tinyfmt&
operator<<(tinyfmt&, double);

template
tinyfmt&
operator<<(tinyfmt&, const void*);

template
tinyfmt&
operator<<(tinyfmt&, void*);

template
wtinyfmt&
operator<<(wtinyfmt&, wchar_t);

template
wtinyfmt&
operator<<(wtinyfmt&, const wchar_t*);

template
wtinyfmt&
operator<<(wtinyfmt&, const ascii_numput&);

template
wtinyfmt&
operator<<(wtinyfmt&, signed char);

template
wtinyfmt&
operator<<(wtinyfmt&, signed short);

template
wtinyfmt&
operator<<(wtinyfmt&, signed);

template
wtinyfmt&
operator<<(wtinyfmt&, signed long);

template
wtinyfmt&
operator<<(wtinyfmt&, signed long long);

template
wtinyfmt&
operator<<(wtinyfmt&, unsigned char);

template
wtinyfmt&
operator<<(wtinyfmt&, unsigned short);

template
wtinyfmt&
operator<<(wtinyfmt&, unsigned);

template
wtinyfmt&
operator<<(wtinyfmt&, unsigned long);

template
wtinyfmt&
operator<<(wtinyfmt&, unsigned long long);

template
wtinyfmt&
operator<<(wtinyfmt&, float);

template
wtinyfmt&
operator<<(wtinyfmt&, double);

template
wtinyfmt&
operator<<(wtinyfmt&, const void*);

template
wtinyfmt&
operator<<(wtinyfmt&, void*);

}  // namespace rocket
