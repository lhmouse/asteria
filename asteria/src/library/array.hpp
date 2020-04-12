// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_ARRAY_HPP_
#define ASTERIA_LIBRARY_ARRAY_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.array.slice`
Aval
std_array_slice(Aval data, Ival from, Iopt length);

// `std.array.replace_slice`
Aval
std_array_replace_slice(Aval data, Ival from, Iopt length, Aval replacement);

// `std.array.find`
Iopt
std_array_find(Aval data, Ival from, Iopt length, Value target);

// `std.array.find_if`
Iopt
std_array_find_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

// `std.array.find_if_not`
Iopt
std_array_find_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

// `std.array.rfind`
Iopt
std_array_rfind(Aval data, Ival from, Iopt length, Value target);

// `std.array.rfind_if`
Iopt
std_array_rfind_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

// `std.array.rfind_if_not`
Iopt
std_array_rfind_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

// `std.array.count`
Ival
std_array_count(Aval data, Ival from, Iopt length, Value target);

// `std.array.count_if`
Ival
std_array_count_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

// `std.array.count_if_not`
Ival
std_array_count_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

// `std.array.exclude`
Aval
std_array_exclude(Aval data, Ival from, Iopt length, Value target);

// `std.array.exclude_if`
Aval
std_array_exclude_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

// `std.array.exclude_if_not`
Aval
std_array_exclude_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

// `std.array.is_sorted`
Bval
std_array_is_sorted(Global& global, Aval data, Fopt comparator);

// `std.array.binary_search`
Iopt
std_array_binary_search(Global& global, Aval data, Value target, Fopt comparator);

// `std.array.lower_bound`
Ival
std_array_lower_bound(Global& global, Aval data, Value target, Fopt comparator);

// `std.array.upper_bound`
Ival
std_array_upper_bound(Global& global, Aval data, Value target, Fopt comparator);

// `std.array.equal_range`
pair<Ival, Ival>
std_array_equal_range(Global& global, Aval data, Value target, Fopt comparator);

// `std.array.sort`
Aval
std_array_sort(Global& global, Aval data, Fopt comparator);

// `std.array.sortu`
Aval
std_array_sortu(Global& global, Aval data, Fopt comparator);

// `std.array.max_of`
Value
std_array_max_of(Global& global, Aval data, Fopt comparator);

// `std.array.min_of`
Value
std_array_min_of(Global& global, Aval data, Fopt comparator);

// `std.array.reverse`
Aval
std_array_reverse(Aval data);

// `std.array.generate`
Aval
std_array_generate(Global& global, Fval generator, Ival length);

// `std.array.shuffle`
Aval
std_array_shuffle(Aval data, Iopt seed);

// `std.array.rotate`
Aval
std_array_rotate(Aval data, Ival shift);

// `std.array.copy_keys`
Aval
std_array_copy_keys(Oval source);

// `std.array.copy_values`
Aval
std_array_copy_values(Oval source);

// Create an object that is to be referenced as `std.array`.
void
create_bindings_array(V_object& result, API_Version version);

}  // namespace Asteria

#endif
