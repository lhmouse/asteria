// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_JSON_HPP_
#define ASTERIA_LIBRARY_BINDINGS_JSON_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_string std_json_format(Value value, opt<G_string> indent = ::rocket::clear);
extern G_string std_json_format(Value value, G_integer indent);
extern Value std_json_parse(G_string text);

// Create an object that is to be referenced as `std.json`.
extern void create_bindings_json(G_object& result, API_Version version);

}  // namespace Asteria

#endif
