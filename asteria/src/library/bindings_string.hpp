// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern int std_string_compare(const Cow_String &text_one, const Cow_String &text_two, std::size_t length);

extern Cow_String std_string_reverse(const Cow_String &text);

extern Cow_String std_string_trim(const Cow_String &text, const Cow_String &reject);
extern Cow_String std_string_trim_left(const Cow_String &text, const Cow_String &reject);
extern Cow_String std_string_trim_right(const Cow_String &text, const Cow_String &reject);

// Create an object that is to be referenced as `std.string`.
extern D_object create_bindings_string();

}  // namespace Asteria

#endif
