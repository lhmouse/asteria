// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_
#define ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_

#include "../fwd.hpp"

namespace Asteria {

Aval array_slice(Aval data, Ival from, Iopt length = { });
Aval array_replace_slice(Aval data, Ival from, Aval replacement);
Aval array_replace_slice(Aval data, Ival from, Iopt length, Aval replacement);

Iopt array_find(Aval data, Value target);
Iopt array_find(Aval data, Ival from, Value target);
Iopt array_find(Aval data, Ival from, Iopt length, Value target);
Iopt array_find_if(Global& global, Aval data, Fval predictor);
Iopt array_find_if(Global& global, Aval data, Ival from, Fval predictor);
Iopt array_find_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
Iopt array_find_if_not(Global& global, Aval data, Fval predictor);
Iopt array_find_if_not(Global& global, Aval data, Ival from, Fval predictor);
Iopt array_find_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

Iopt array_rfind(Aval data, Value target);
Iopt array_rfind(Aval data, Ival from, Value target);
Iopt array_rfind(Aval data, Ival from, Iopt length, Value target);
Iopt array_rfind_if(Global& global, Aval data, Fval predictor);
Iopt array_rfind_if(Global& global, Aval data, Ival from, Fval predictor);
Iopt array_rfind_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
Iopt array_rfind_if_not(Global& global, Aval data, Fval predictor);
Iopt array_rfind_if_not(Global& global, Aval data, Ival from, Fval predictor);
Iopt array_rfind_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

Ival array_count(Aval data, Value target);
Ival array_count(Aval data, Ival from, Value target);
Ival array_count(Aval data, Ival from, Iopt length, Value target);
Ival array_count_if(Global& global, Aval data, Fval predictor);
Ival array_count_if(Global& global, Aval data, Ival from, Fval predictor);
Ival array_count_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
Ival array_count_if_not(Global& global, Aval data, Fval predictor);
Ival array_count_if_not(Global& global, Aval data, Ival from, Fval predictor);
Ival array_count_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

Aval array_exclude(Aval data, Value target);
Aval array_exclude(Aval data, Ival from, Value target);
Aval array_exclude(Aval data, Ival from, Iopt length, Value target);
Aval array_exclude_if(Global& global, Aval data, Fval predictor);
Aval array_exclude_if(Global& global, Aval data, Ival from, Fval predictor);
Aval array_exclude_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
Aval array_exclude_if_not(Global& global, Aval data, Fval predictor);
Aval array_exclude_if_not(Global& global, Aval data, Ival from, Fval predictor);
Aval array_exclude_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

Bval array_is_sorted(Global& global, Aval data, Fopt comparator = { });
Iopt array_binary_search(Global& global, Aval data, Value target, Fopt comparator = { });
Ival array_lower_bound(Global& global, Aval data, Value target, Fopt comparator = { });
Ival array_upper_bound(Global& global, Aval data, Value target, Fopt comparator = { });
pair<Ival, Ival> array_equal_range(Global& global, Aval data, Value target, Fopt comparator = { });

Aval array_sort(Global& global, Aval data, Fopt comparator = { });
Aval array_sortu(Global& global, Aval data, Fopt comparator = { });

Value array_max_of(Global& global, Aval data, Fopt comparator = { });
Value array_min_of(Global& global, Aval data, Fopt comparator = { });

Aval array_reverse(Aval data);
Aval array_generate(Global& global, Fval generator, Ival length);
Aval array_shuffle(Aval data, Iopt seed = { });
Aval array_rotate(Aval data, Ival shift);

Aval array_copy_keys(Oval source);
Aval array_copy_values(Oval source);

// Create an object that is to be referenced as `std.array`.
void create_bindings_array(Oval& result, API_Version version);

}  // namespace Asteria

#endif
