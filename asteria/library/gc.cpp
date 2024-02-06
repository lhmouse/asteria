// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "gc.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/garbage_collector.hpp"
#include "../utils.hpp"
namespace asteria {

V_integer
std_gc_count_variables(Global_Context& global, V_integer generation)
  {
    auto rgen = ::rocket::clamp_cast<GC_Generation>(generation, 0, 2);
    if(rgen != generation)
      ASTERIA_THROW((
          "Invalid generation `$1`"),
          generation);

    // Get the current number of variables being tracked.
    const auto gcoll = global.garbage_collector();
    size_t nvars = gcoll->count_tracked_variables(rgen);
    return static_cast<int64_t>(nvars);
  }

V_integer
std_gc_get_threshold(Global_Context& global, V_integer generation)
  {
    auto rgen = ::rocket::clamp_cast<GC_Generation>(generation, 0, 2);
    if(rgen != generation)
      ASTERIA_THROW((
          "Invalid generation `$1`"),
          generation);

    // Get the current number of variables being tracked.
    const auto gcoll = global.garbage_collector();
    size_t thres = gcoll->get_threshold(rgen);
    return static_cast<int64_t>(thres);
  }

V_integer
std_gc_set_threshold(Global_Context& global, V_integer generation, V_integer threshold)
  {
    auto rgen = ::rocket::clamp_cast<GC_Generation>(generation, 0, 2);
    if(rgen != generation)
      ASTERIA_THROW((
          "Invalid generation `$1`"),
          generation);

    // Set the threshold and return its old value.
    const auto gcoll = global.garbage_collector();
    size_t oldval = gcoll->get_threshold(rgen);
    gcoll->set_threshold(rgen, ::rocket::clamp_cast<size_t>(threshold, 0, PTRDIFF_MAX));
    return static_cast<int64_t>(oldval);
  }

V_integer
std_gc_collect(Global_Context& global, optV_integer generation_limit)
  {
    auto rglimit = gc_generation_oldest;
    if(generation_limit) {
      rglimit = ::rocket::clamp_cast<GC_Generation>(*generation_limit, 0, 2);
      if(rglimit != *generation_limit)
        ASTERIA_THROW((
            "Invalid generation limit `$1`"),
            *generation_limit);
    }

    // Perform garbage collection up to the generation specified.
    const auto gcoll = global.garbage_collector();
    size_t nvars = gcoll->collect_variables(rglimit);
    return static_cast<int64_t>(nvars);
  }

void
create_bindings_gc(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("count_variables"),
      ASTERIA_BINDING(
        "std.gc.count_variables", "generation",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_integer gen;

        reader.start_overload();
        reader.required(gen);
        if(reader.end_overload())
          return (Value) std_gc_count_variables(global, gen);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("get_threshold"),
      ASTERIA_BINDING(
        "std.gc.get_threshold", "generation",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_integer gen;

        reader.start_overload();
        reader.required(gen);
        if(reader.end_overload())
          return (Value) std_gc_get_threshold(global, gen);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("set_threshold"),
      ASTERIA_BINDING(
        "std.gc.set_threshold", "generation, threshold",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_integer gen, thrs;

        reader.start_overload();
        reader.required(gen);
        reader.required(thrs);
        if(reader.end_overload())
          return (Value) std_gc_set_threshold(global, gen, thrs);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("collect"),
      ASTERIA_BINDING(
        "std.gc.collect", "[generation_limit]",
        Global_Context& global, Argument_Reader&& reader)
      {
        optV_integer glim;

        reader.start_overload();
        reader.optional(glim);
        if(reader.end_overload())
          return (Value) std_gc_collect(global, glim);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
