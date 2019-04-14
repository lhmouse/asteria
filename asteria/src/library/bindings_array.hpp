// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_
#define ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_array std_array_slice(const G_array& data, const G_integer& from, const Opt<G_integer>& length = rocket::nullopt);
extern G_array std_array_replace_slice(const G_array& data, const G_integer& from, const G_array& replacement);
extern G_array std_array_replace_slice(const G_array& data, const G_integer& from, const Opt<G_integer>& length, const G_array& replacement);

extern Value std_array_max_of(const G_array& data);
extern Value std_array_min_of(const G_array& data);

extern Opt<G_integer> std_array_find(const G_array& data, const Value& target);
extern Opt<G_integer> std_array_find(const G_array& data, const G_integer& from, const Value& target);
extern Opt<G_integer> std_array_find(const G_array& data, const G_integer& from, const Opt<G_integer>& length, const Value& target);
extern Opt<G_integer> std_array_rfind(const G_array& data, const Value& target);
extern Opt<G_integer> std_array_rfind(const G_array& data, const G_integer& from, const Value& target);
extern Opt<G_integer> std_array_rfind(const G_array& data, const G_integer& from, const Opt<G_integer>& length, const Value& target);

extern Opt<G_integer> std_array_find_if(const Global_Context& global, const G_array& data, const G_function& predictor);
extern Opt<G_integer> std_array_find_if(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor);
extern Opt<G_integer> std_array_find_if(const Global_Context& global, const G_array& data, const G_integer& from, const Opt<G_integer>& length, const G_function& predictor);
extern Opt<G_integer> std_array_find_if_not(const Global_Context& global, const G_array& data, const G_function& predictor);
extern Opt<G_integer> std_array_find_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor);
extern Opt<G_integer> std_array_find_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const Opt<G_integer>& length, const G_function& predictor);
extern Opt<G_integer> std_array_rfind_if(const Global_Context& global, const G_array& data, const G_function& predictor);
extern Opt<G_integer> std_array_rfind_if(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor);
extern Opt<G_integer> std_array_rfind_if(const Global_Context& global, const G_array& data, const G_integer& from, const Opt<G_integer>& length, const G_function& predictor);
extern Opt<G_integer> std_array_rfind_if_not(const Global_Context& global, const G_array& data, const G_function& predictor);
extern Opt<G_integer> std_array_rfind_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor);
extern Opt<G_integer> std_array_rfind_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const Opt<G_integer>& length, const G_function& predictor);

extern G_array std_array_reverse(const G_array& data);

extern G_boolean std_array_is_sorted(const Global_Context& global, const G_array& data, const Opt<G_function>& comparator = rocket::nullopt);
extern Opt<G_integer> std_array_binary_search(const Global_Context& global, const G_array& data, const Value& target, const Opt<G_function>& comparator = rocket::nullopt);
extern G_integer std_array_lower_bound(const Global_Context& global, const G_array& data, const Value& target, const Opt<G_function>& comparator = rocket::nullopt);
extern G_integer std_array_upper_bound(const Global_Context& global, const G_array& data, const Value& target, const Opt<G_function>& comparator = rocket::nullopt);
extern std::pair<G_integer, G_integer> std_array_equal_range(const Global_Context& global, const G_array& data, const Value& target, const Opt<G_function>& comparator = rocket::nullopt);
extern G_array std_array_sort(const Global_Context& global, const G_array& data, const Opt<G_function>& comparator = rocket::nullopt);

extern G_array std_array_generate(const Global_Context& global, const G_function& generator, const G_integer& length);
extern G_array std_array_shuffle(const G_array& data, const Opt<G_integer>& seed);

// Create an object that is to be referenced as `std.array`.
extern void create_bindings_array(G_object& result, API_Version version);

}  // namespace Asteria

#endif
