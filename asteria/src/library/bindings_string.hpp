// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern D_integer std_string_compare(const D_string &text_one, const D_string &text_two, D_integer length);

extern D_string std_string_reverse(const D_string &text);

extern D_string std_string_trim(const D_string &text, const D_string &reject);
extern D_string std_string_trim_left(const D_string &text, const D_string &reject);
extern D_string std_string_trim_right(const D_string &text, const D_string &reject);

// Create an object that is to be referenced as `std.string`.
extern D_object create_bindings_string();

}  // namespace Asteria

#endif
