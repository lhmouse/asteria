// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern D_integer std_string_compare(const D_string& text_one, const D_string& text_two, D_integer length);
extern D_boolean std_string_starts_with(const D_string& text, const D_string& prefix);
extern D_boolean std_string_ends_with(const D_string& text, const D_string& suffix);

extern D_string std_string_substr(const D_string& text, D_integer from, D_integer length);
extern D_string std_string_reverse(const D_string& text);
extern D_string std_string_trim(const D_string& text, const D_string& reject);
extern D_string std_string_trim_left(const D_string& text, const D_string& reject);
extern D_string std_string_trim_right(const D_string& text, const D_string& reject);
extern D_string std_string_to_upper(const D_string& text);
extern D_string std_string_to_lower(const D_string& text);

extern D_array std_string_explode(const D_string& text, const D_string& delim, D_integer limit);
extern D_string std_string_implode(const D_array& segments, const D_string& delim);

// Create an object that is to be referenced as `std.string`.
extern D_object create_bindings_string();

}  // namespace Asteria

#endif
