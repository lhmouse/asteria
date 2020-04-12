// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_JSON_HPP_
#define ASTERIA_LIBRARY_JSON_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.json.format`
Sval
std_json_format(Value value, Sopt indent);

Sval
std_json_format(Value value, Ival indent);

// `std.json.format5`
Sval
std_json_format5(Value value, Sopt indent);

Sval
std_json_format5(Value value, Ival indent);

// `std.json.parse`
Value
std_json_parse(Sval text);

// Create an object that is to be referenced as `std.json`.
void
create_bindings_json(V_object& result, API_Version version);

}  // namespace Asteria

#endif
