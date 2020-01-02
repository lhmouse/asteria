// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_
#define ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_array std_array_slice(G_array data, G_integer from, opt<G_integer> length = ::rocket::clear);
extern G_array std_array_replace_slice(G_array data, G_integer from, G_array replacement);
extern G_array std_array_replace_slice(G_array data, G_integer from, opt<G_integer> length, G_array replacement);

extern opt<G_integer> std_array_find(G_array data, Value target);
extern opt<G_integer> std_array_find(G_array data, G_integer from, Value target);
extern opt<G_integer> std_array_find(G_array data, G_integer from, opt<G_integer> length, Value target);
extern opt<G_integer> std_array_find_if(Global_Context& global, G_array data, G_function predictor);
extern opt<G_integer> std_array_find_if(Global_Context& global, G_array data, G_integer from, G_function predictor);
extern opt<G_integer> std_array_find_if(Global_Context& global, G_array data, G_integer from, opt<G_integer> length, G_function predictor);
extern opt<G_integer> std_array_find_if_not(Global_Context& global, G_array data, G_function predictor);
extern opt<G_integer> std_array_find_if_not(Global_Context& global, G_array data, G_integer from, G_function predictor);
extern opt<G_integer> std_array_find_if_not(Global_Context& global, G_array data, G_integer from, opt<G_integer> length, G_function predictor);

extern opt<G_integer> std_array_rfind(G_array data, Value target);
extern opt<G_integer> std_array_rfind(G_array data, G_integer from, Value target);
extern opt<G_integer> std_array_rfind(G_array data, G_integer from, opt<G_integer> length, Value target);
extern opt<G_integer> std_array_rfind_if(Global_Context& global, G_array data, G_function predictor);
extern opt<G_integer> std_array_rfind_if(Global_Context& global, G_array data, G_integer from, G_function predictor);
extern opt<G_integer> std_array_rfind_if(Global_Context& global, G_array data, G_integer from, opt<G_integer> length, G_function predictor);
extern opt<G_integer> std_array_rfind_if_not(Global_Context& global, G_array data, G_function predictor);
extern opt<G_integer> std_array_rfind_if_not(Global_Context& global, G_array data, G_integer from, G_function predictor);
extern opt<G_integer> std_array_rfind_if_not(Global_Context& global, G_array data, G_integer from, opt<G_integer> length, G_function predictor);

extern G_integer std_array_count(G_array data, Value target);
extern G_integer std_array_count(G_array data, G_integer from, Value target);
extern G_integer std_array_count(G_array data, G_integer from, opt<G_integer> length, Value target);
extern G_integer std_array_count_if(Global_Context& global, G_array data, G_function predictor);
extern G_integer std_array_count_if(Global_Context& global, G_array data, G_integer from, G_function predictor);
extern G_integer std_array_count_if(Global_Context& global, G_array data, G_integer from, opt<G_integer> length, G_function predictor);
extern G_integer std_array_count_if_not(Global_Context& global, G_array data, G_function predictor);
extern G_integer std_array_count_if_not(Global_Context& global, G_array data, G_integer from, G_function predictor);
extern G_integer std_array_count_if_not(Global_Context& global, G_array data, G_integer from, opt<G_integer> length, G_function predictor);

extern G_boolean std_array_is_sorted(Global_Context& global, G_array data, opt<G_function> comparator = ::rocket::clear);
extern opt<G_integer> std_array_binary_search(Global_Context& global, G_array data, Value target, opt<G_function> comparator = ::rocket::clear);
extern G_integer std_array_lower_bound(Global_Context& global, G_array data, Value target, opt<G_function> comparator = ::rocket::clear);
extern G_integer std_array_upper_bound(Global_Context& global, G_array data, Value target, opt<G_function> comparator = ::rocket::clear);
extern pair<G_integer, G_integer> std_array_equal_range(Global_Context& global, G_array data, Value target, opt<G_function> comparator = ::rocket::clear);

extern G_array std_array_sort(Global_Context& global, G_array data, opt<G_function> comparator = ::rocket::clear);
extern G_array std_array_sortu(Global_Context& global, G_array data, opt<G_function> comparator = ::rocket::clear);

extern Value std_array_max_of(Global_Context& global, G_array data, opt<G_function> comparator = ::rocket::clear);
extern Value std_array_min_of(Global_Context& global, G_array data, opt<G_function> comparator = ::rocket::clear);

extern G_array std_array_reverse(G_array data);
extern G_array std_array_generate(Global_Context& global, G_function generator, G_integer length);
extern G_array std_array_shuffle(G_array data, opt<G_integer> seed = ::rocket::clear);

extern opt<G_array> std_array_copy_keys(opt<G_object> source);
extern opt<G_array> std_array_copy_values(opt<G_object> source);

// Create an object that is to be referenced as `std.array`.
extern void create_bindings_array(G_object& result, API_Version version);

}  // namespace Asteria

#endif
