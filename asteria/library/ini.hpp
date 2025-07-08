// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_INI_
#define ASTERIA_LIBRARY_INI_

#include "../fwd.hpp"
namespace asteria {

// `std.ini.format`
V_string
std_ini_format(V_object value);

// `std.ini.format_to_file`
void
std_ini_format_to_file(V_string path, V_object value);

// `std.ini.parse`
V_object
std_ini_parse(V_string text);

// `std.ini.parse_file`
V_object
std_ini_parse_file(V_string path);

// Create an object that is to be referenced as `std.ini`.
void
create_bindings_ini(V_object& result, API_Version version);

}  // namespace asteria
#endif
