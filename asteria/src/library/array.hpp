// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_ARRAY_HPP_
#define ASTERIA_LIBRARY_ARRAY_HPP_

#include "../fwd.hpp"

namespace asteria {

// `std.array.slice`
V_array
std_array_slice(V_array data, V_integer from, Opt_integer length);

// `std.array.replace_slice`
V_array
std_array_replace_slice(V_array data, V_integer from, Opt_integer length, V_array replacement);

// `std.array.find`
Opt_integer
std_array_find(V_array data, V_integer from, Opt_integer length, Value target);

// `std.array.find_if`
Opt_integer
std_array_find_if(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                  V_function predictor);

// `std.array.find_if_not`
Opt_integer
std_array_find_if_not(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                      V_function predictor);

// `std.array.rfind`
Opt_integer
std_array_rfind(V_array data, V_integer from, Opt_integer length, Value target);

// `std.array.rfind_if`
Opt_integer
std_array_rfind_if(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                   V_function predictor);

// `std.array.rfind_if_not`
Opt_integer
std_array_rfind_if_not(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                       V_function predictor);

// `std.array.count`
V_integer
std_array_count(V_array data, V_integer from, Opt_integer length, Value target);

// `std.array.count_if`
V_integer
std_array_count_if(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                   V_function predictor);

// `std.array.count_if_not`
V_integer
std_array_count_if_not(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                       V_function predictor);

// `std.array.exclude`
V_array
std_array_exclude(V_array data, V_integer from, Opt_integer length, Value target);

// `std.array.exclude_if`
V_array
std_array_exclude_if(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                     V_function predictor);

// `std.array.exclude_if_not`
V_array
std_array_exclude_if_not(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                         V_function predictor);

// `std.array.is_sorted`
V_boolean
std_array_is_sorted(Global_Context& global, V_array data, Opt_function comparator);

// `std.array.binary_search`
Opt_integer
std_array_binary_search(Global_Context& global, V_array data, Value target, Opt_function comparator);

// `std.array.lower_bound`
V_integer
std_array_lower_bound(Global_Context& global, V_array data, Value target, Opt_function comparator);

// `std.array.upper_bound`
V_integer
std_array_upper_bound(Global_Context& global, V_array data, Value target, Opt_function comparator);

// `std.array.equal_range`
pair<V_integer, V_integer>
std_array_equal_range(Global_Context& global, V_array data, Value target, Opt_function comparator);

// `std.array.sort`
V_array
std_array_sort(Global_Context& global, V_array data, Opt_function comparator);

// `std.array.sortu`
V_array
std_array_sortu(Global_Context& global, V_array data, Opt_function comparator);

// `std.array.max_of`
Value
std_array_max_of(Global_Context& global, V_array data, Opt_function comparator);

// `std.array.min_of`
Value
std_array_min_of(Global_Context& global, V_array data, Opt_function comparator);

// `std.array.reverse`
V_array
std_array_reverse(V_array data);

// `std.array.generate`
V_array
std_array_generate(Global_Context& global, V_function generator, V_integer length);

// `std.array.shuffle`
V_array
std_array_shuffle(V_array data, Opt_integer seed);

// `std.array.rotate`
V_array
std_array_rotate(V_array data, V_integer shift);

// `std.array.copy_keys`
V_array
std_array_copy_keys(V_object source);

// `std.array.copy_values`
V_array
std_array_copy_values(V_object source);

// Create an object that is to be referenced as `std.array`.
void
create_bindings_array(V_object& result, API_Version version);

}  // namespace asteria

#endif
