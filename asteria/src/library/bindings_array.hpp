// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_
#define ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Value std_array_max_of(const D_array& data);
extern Value std_array_min_of(const D_array& data);

extern Opt<D_integer> std_array_find(const D_array& data, const Value& target);
extern Opt<D_integer> std_array_rfind(const D_array& data, const Value& target);

extern Opt<D_integer> std_array_find_if(const Global_Context& global, const D_array& data, const D_function& predictor);
extern Opt<D_integer> std_array_find_if_not(const Global_Context& global, const D_array& data, const D_function& predictor);
extern Opt<D_integer> std_array_rfind_if(const Global_Context& global, const D_array& data, const D_function& predictor);
extern Opt<D_integer> std_array_rfind_if_not(const Global_Context& global, const D_array& data, const D_function& predictor);

extern D_boolean std_array_is_sorted(const Global_Context& global, const D_array& data, const Opt<D_function>& comparator = rocket::nullopt);
extern Opt<D_integer> std_array_binary_search(const Global_Context& global, const D_array& data, const Value& target, const Opt<D_function>& comparator = rocket::nullopt);
extern D_integer std_array_lower_bound(const Global_Context& global, const D_array& data, const Value& target, const Opt<D_function>& comparator = rocket::nullopt);
extern D_integer std_array_upper_bound(const Global_Context& global, const D_array& data, const Value& target, const Opt<D_function>& comparator = rocket::nullopt);
extern std::pair<D_integer, D_integer> std_array_equal_range(const Global_Context& global, const D_array& data, const Value& target, const Opt<D_function>& comparator = rocket::nullopt);
extern D_array std_array_sort(const Global_Context& global, const D_array& data, const Opt<D_function>& comparator = rocket::nullopt);

// Create an object that is to be referenced as `std.array`.
extern void create_bindings_array(D_object& result, API_Version version);

}  // namespace Asteria

#endif
