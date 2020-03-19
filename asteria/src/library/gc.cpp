// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "gc.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/generational_collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

Iopt std_gc_tracked_count(Global& global, Ival generation)
  {
    auto gc_gen = static_cast<GC_Generation>(::rocket::clamp(generation,
                        static_cast<Ival>(gc_generation_newest), static_cast<Ival>(gc_generation_oldest)));
    if(gc_gen != generation) {
      return nullopt;
    }
    // Get the current number of variables being tracked.
    auto gcoll = global.generational_collector();
    size_t count = gcoll->get_collector(gc_gen).count_tracked_variables();
    return static_cast<int64_t>(count);
  }

Iopt std_gc_get_threshold(Global& global, Ival generation)
  {
    auto gc_gen = static_cast<GC_Generation>(::rocket::clamp(generation,
                        static_cast<Ival>(gc_generation_newest), static_cast<Ival>(gc_generation_oldest)));
    if(gc_gen != generation) {
      return nullopt;
    }
    // Get the current threshold.
    auto gcoll = global.generational_collector();
    uint32_t thres = gcoll->get_collector(gc_gen).get_threshold();
    return static_cast<int64_t>(thres);
  }

Iopt std_gc_set_threshold(Global& global, Ival generation, Ival threshold)
  {
    auto gc_gen = static_cast<GC_Generation>(::rocket::clamp(generation,
                        static_cast<Ival>(gc_generation_newest), static_cast<Ival>(gc_generation_oldest)));
    if(gc_gen != generation) {
      return nullopt;
    }
    uint32_t thres_new = static_cast<uint32_t>(::rocket::clamp(threshold, 0, INT32_MAX));
    // Set the threshold and return its old value.
    auto gcoll = global.generational_collector();
    uint32_t thres = gcoll->get_collector(gc_gen).get_threshold();
    gcoll->open_collector(gc_gen).set_threshold(thres_new);
    return static_cast<int64_t>(thres);
  }

Ival std_gc_collect(Global& global, Iopt generation_limit)
  {
    auto gc_limit = gc_generation_oldest;
    // Unlike others, this function does not fail if `generation_limit` is out of range.
    if(generation_limit) {
      gc_limit = static_cast<GC_Generation>(::rocket::clamp(*generation_limit,
                       static_cast<Ival>(gc_generation_newest), static_cast<Ival>(gc_generation_oldest)));
    }
    // Perform garbage collection up to the generation specified.
    auto gcoll = global.generational_collector();
    size_t nvars = gcoll->collect_variables(gc_limit);
    return static_cast<int64_t>(nvars);
  }

void create_bindings_gc(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.gc.tracked_count()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("tracked_count"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.gc.tracked_count"));
    // Parse arguments.
    Ival generation;
    if(reader.I().v(generation).F()) {
      // Call the binding function.
      return std_gc_tracked_count(global, ::rocket::move(generation));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.gc.tracked_count(generation)`

  * Gets the number of variables that are being tracked by the
    collector for `generation`. Valid values for `generation` are
    `0`, `1` and `2`.

  * Returns the number of variables being tracked. This value is
    only informative. If `generation` is not valid, `null` is
    returned.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.gc.get_threshold()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("get_threshold"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.gc.get_threshold"));
    // Parse arguments.
    Ival generation;
    if(reader.I().v(generation).F()) {
      // Call the binding function.
      return std_gc_get_threshold(global, ::rocket::move(generation));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.gc.get_threshold(generation)`

  * Gets the threshold of the collector for `generation`. Valid
    values for `generation` are `0`, `1` and `2`.

  * Returns the threshold. If `generation` is not valid, `null` is
    returned.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.gc.set_threshold()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("set_threshold"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.gc.set_threshold"));
    // Parse arguments.
    Ival generation;
    Ival threshold;
    if(reader.I().v(generation).v(threshold).F()) {
      // Call the binding function.
      return std_gc_set_threshold(global, ::rocket::move(generation), ::rocket::move(threshold));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.gc.set_threshold(generation, threshold)`

  * Sets the threshold of the collector for `generation` to
    `threshold`. Valid values for `generation` are `0`, `1` and
    `2`. Valid values for `threshold` range from `0` to an
    unspecified positive integer; overlarge values are capped
    silently without failure. A larger `threshold` makes garbage
    collection run less often but slower. Setting `threshold` to
    `0` ensures all unreachable variables be collected immediately.

  * Returns the threshold before the call. If `generation` is not
    valid, `null` is returned.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.gc.collect()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("collect"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.gc.collect"));
    // Parse arguments.
    Iopt generation_limit;
    if(reader.I().o(generation_limit).F()) {
      // Call the binding function.
      return std_gc_collect(global, ::rocket::move(generation_limit));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.gc.collect([generation_limit])`

  * Performs garbage collection on all generations including and
    up to `generation_limit`. If it is absent, all generations are
    collected.

  * Returns the number of variables that have been collected in
    total.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // End of `std.gc`
    //===================================================================
  }

}  // namespace Asteria
