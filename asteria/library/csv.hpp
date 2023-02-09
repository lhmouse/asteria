// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_CSV_
#define ASTERIA_LIBRARY_CSV_

#include "../fwd.hpp"
namespace asteria {

// `std.csv.format`
V_string
std_csv_format(V_array value);

// `std.csv.parse`
V_array
std_csv_parse(V_string text);

// `std.csv.parse_file`
V_array
std_csv_parse_file(V_string path);

// Create an object that is to be referenced as `std.csv`.
void
create_bindings_csv(V_object& result, API_Version version);

}  // namespace asteria
#endif
