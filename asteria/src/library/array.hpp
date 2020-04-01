// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_ARRAY_HPP_
#define ASTERIA_LIBRARY_ARRAY_HPP_

#include "../fwd.hpp"

namespace Asteria {

Aval std_array_slice(Aval data, Ival from, Iopt length);
Aval std_array_replace_slice(Aval data, Ival from, Aval replacement);
Aval std_array_replace_slice(Aval data, Ival from, Iopt length, Aval replacement);

Iopt std_array_find(Aval data, Value target);
Iopt std_array_find(Aval data, Ival from, Value target);
Iopt std_array_find(Aval data, Ival from, Iopt length, Value target);
Iopt std_array_find_if(Global& global, Aval data, Fval predictor);
Iopt std_array_find_if(Global& global, Aval data, Ival from, Fval predictor);
Iopt std_array_find_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
Iopt std_array_find_if_not(Global& global, Aval data, Fval predictor);
Iopt std_array_find_if_not(Global& global, Aval data, Ival from, Fval predictor);
Iopt std_array_find_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

Iopt std_array_rfind(Aval data, Value target);
Iopt std_array_rfind(Aval data, Ival from, Value target);
Iopt std_array_rfind(Aval data, Ival from, Iopt length, Value target);
Iopt std_array_rfind_if(Global& global, Aval data, Fval predictor);
Iopt std_array_rfind_if(Global& global, Aval data, Ival from, Fval predictor);
Iopt std_array_rfind_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
Iopt std_array_rfind_if_not(Global& global, Aval data, Fval predictor);
Iopt std_array_rfind_if_not(Global& global, Aval data, Ival from, Fval predictor);
Iopt std_array_rfind_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

Ival std_array_count(Aval data, Value target);
Ival std_array_count(Aval data, Ival from, Value target);
Ival std_array_count(Aval data, Ival from, Iopt length, Value target);
Ival std_array_count_if(Global& global, Aval data, Fval predictor);
Ival std_array_count_if(Global& global, Aval data, Ival from, Fval predictor);
Ival std_array_count_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
Ival std_array_count_if_not(Global& global, Aval data, Fval predictor);
Ival std_array_count_if_not(Global& global, Aval data, Ival from, Fval predictor);
Ival std_array_count_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

Aval std_array_exclude(Aval data, Value target);
Aval std_array_exclude(Aval data, Ival from, Value target);
Aval std_array_exclude(Aval data, Ival from, Iopt length, Value target);
Aval std_array_exclude_if(Global& global, Aval data, Fval predictor);
Aval std_array_exclude_if(Global& global, Aval data, Ival from, Fval predictor);
Aval std_array_exclude_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
Aval std_array_exclude_if_not(Global& global, Aval data, Fval predictor);
Aval std_array_exclude_if_not(Global& global, Aval data, Ival from, Fval predictor);
Aval std_array_exclude_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

Bval std_array_is_sorted(Global& global, Aval data, Fopt comparator);
Iopt std_array_binary_search(Global& global, Aval data, Value target, Fopt comparator);
Ival std_array_lower_bound(Global& global, Aval data, Value target, Fopt comparator);
Ival std_array_upper_bound(Global& global, Aval data, Value target, Fopt comparator);
pair<Ival, Ival> std_array_equal_range(Global& global, Aval data, Value target, Fopt comparator);

Aval std_array_sort(Global& global, Aval data, Fopt comparator);
Aval std_array_sortu(Global& global, Aval data, Fopt comparator);

Value std_array_max_of(Global& global, Aval data, Fopt comparator);
Value std_array_min_of(Global& global, Aval data, Fopt comparator);

Aval std_array_reverse(Aval data);
Aval std_array_generate(Global& global, Fval generator, Ival length);
Aval std_array_shuffle(Aval data, Iopt seed);
Aval std_array_rotate(Aval data, Ival shift);

Aval std_array_copy_keys(Oval source);
Aval std_array_copy_values(Oval source);

// Create an object that is to be referenced as `std.array`.
void create_bindings_array(V_object& result, API_Version version);

}  // namespace Asteria

#endif
