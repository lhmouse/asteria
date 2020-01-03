// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_
#define ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Aval std_array_slice(Aval data, Ival from, Iopt length = ::rocket::clear);
extern Aval std_array_replace_slice(Aval data, Ival from, Aval replacement);
extern Aval std_array_replace_slice(Aval data, Ival from, Iopt length, Aval replacement);

extern Iopt std_array_find(Aval data, Value target);
extern Iopt std_array_find(Aval data, Ival from, Value target);
extern Iopt std_array_find(Aval data, Ival from, Iopt length, Value target);
extern Iopt std_array_find_if(Global& global, Aval data, Fval predictor);
extern Iopt std_array_find_if(Global& global, Aval data, Ival from, Fval predictor);
extern Iopt std_array_find_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
extern Iopt std_array_find_if_not(Global& global, Aval data, Fval predictor);
extern Iopt std_array_find_if_not(Global& global, Aval data, Ival from, Fval predictor);
extern Iopt std_array_find_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

extern Iopt std_array_rfind(Aval data, Value target);
extern Iopt std_array_rfind(Aval data, Ival from, Value target);
extern Iopt std_array_rfind(Aval data, Ival from, Iopt length, Value target);
extern Iopt std_array_rfind_if(Global& global, Aval data, Fval predictor);
extern Iopt std_array_rfind_if(Global& global, Aval data, Ival from, Fval predictor);
extern Iopt std_array_rfind_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
extern Iopt std_array_rfind_if_not(Global& global, Aval data, Fval predictor);
extern Iopt std_array_rfind_if_not(Global& global, Aval data, Ival from, Fval predictor);
extern Iopt std_array_rfind_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

extern Ival std_array_count(Aval data, Value target);
extern Ival std_array_count(Aval data, Ival from, Value target);
extern Ival std_array_count(Aval data, Ival from, Iopt length, Value target);
extern Ival std_array_count_if(Global& global, Aval data, Fval predictor);
extern Ival std_array_count_if(Global& global, Aval data, Ival from, Fval predictor);
extern Ival std_array_count_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor);
extern Ival std_array_count_if_not(Global& global, Aval data, Fval predictor);
extern Ival std_array_count_if_not(Global& global, Aval data, Ival from, Fval predictor);
extern Ival std_array_count_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor);

extern Bval std_array_is_sorted(Global& global, Aval data, Fopt comparator = ::rocket::clear);
extern Iopt std_array_binary_search(Global& global, Aval data, Value target, Fopt comparator = ::rocket::clear);
extern Ival std_array_lower_bound(Global& global, Aval data, Value target, Fopt comparator = ::rocket::clear);
extern Ival std_array_upper_bound(Global& global, Aval data, Value target, Fopt comparator = ::rocket::clear);
extern pair<Ival, Ival> std_array_equal_range(Global& global, Aval data, Value target, Fopt comparator = ::rocket::clear);

extern Aval std_array_sort(Global& global, Aval data, Fopt comparator = ::rocket::clear);
extern Aval std_array_sortu(Global& global, Aval data, Fopt comparator = ::rocket::clear);

extern Value std_array_max_of(Global& global, Aval data, Fopt comparator = ::rocket::clear);
extern Value std_array_min_of(Global& global, Aval data, Fopt comparator = ::rocket::clear);

extern Aval std_array_reverse(Aval data);
extern Aval std_array_generate(Global& global, Fval generator, Ival length);
extern Aval std_array_shuffle(Aval data, Iopt seed = ::rocket::clear);

extern Aopt std_array_copy_keys(Oopt source);
extern Aopt std_array_copy_values(Oopt source);

// Create an object that is to be referenced as `std.array`.
extern void create_bindings_array(Oval& result, API_Version version);

}  // namespace Asteria

#endif
