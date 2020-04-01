// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "system.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/generational_collector.hpp"
#include "../utilities.hpp"
#include <spawn.h>  // ::posix_spawnp()
#include <sys/wait.h>  // ::waitpid()
#include <unistd.h>  // ::daemon()

namespace Asteria {

Iopt std_system_gc_count_variables(Global& global, Ival generation)
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

Iopt std_system_gc_get_threshold(Global& global, Ival generation)
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

Iopt std_system_gc_set_threshold(Global& global, Ival generation, Ival threshold)
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

Ival std_system_gc_collect(Global& global, Iopt generation_limit)
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

Ival std_system_execute(Sval cmd, Aopt argv, Aopt envp)
  {
    cow_vector<const char*> ptrs = { cmd.safe_c_str(), nullptr };
    size_t eoff = 1;  // beginning of environment variables

    // Append arguments. `eoff` will be the offset of the terminating null pointer.
    if(argv) {
      ptrs.clear();
      ::rocket::for_each(*argv, [&](const Value& arg) { ptrs.emplace_back(arg.as_string().safe_c_str());  });
      eoff = ptrs.size();
      ptrs.emplace_back(nullptr);
    }
    // Append environment variables.
    if(envp) {
      eoff = ptrs.size();
      ::rocket::for_each(*envp, [&](const Value& arg) { ptrs.emplace_back(arg.as_string().safe_c_str());  });
      ptrs.emplace_back(nullptr);
    }
    // Be `const`-friendly.
    auto pargv = const_cast<char**>(ptrs.data());
    auto penvp = const_cast<char**>(ptrs.data() + eoff);
    // Launch the program.
    ::pid_t pid;
    if(::posix_spawnp(&pid, cmd.c_str(), nullptr, nullptr, pargv, penvp) != 0)
      ASTERIA_THROW_SYSTEM_ERROR("posix_spawnp");

    // Await its termination.
    for(;;) {
      // Note: `waitpid()` may return if the child has been stopped or continued.
      int wstat;
      if(::waitpid(pid, &wstat, 0) == -1)
        ASTERIA_THROW_SYSTEM_ERROR("waitpid");
      // Check whether the process has terminated normally.
      if(WIFEXITED(wstat))
        return WEXITSTATUS(wstat);
      // Check whether the process has been terminated by a signal.
      if(WIFSIGNALED(wstat))
        return 128 + WTERMSIG(wstat);
    }
  }

void std_system_daemonize()
  {
    if(::daemon(1, 0) != 0)
      ASTERIA_THROW_SYSTEM_ERROR("daemon");
  }

void create_bindings_system(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.system.gc_count_variables()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_count_variables"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.gc_count_variables"));
    // Parse arguments.
    Ival generation;
    if(reader.I().v(generation).F()) {
      return std_system_gc_count_variables(global, ::std::move(generation));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_count_variables(generation)`

  * Gets the number of variables that are being tracked by the
    collector for `generation`. Valid values for `generation` are
    `0`, `1` and `2`.

  * Returns the number of variables being tracked. This value is
    only informative. If `generation` is not valid, `null` is
    returned.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.system.gc_get_threshold()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_get_threshold"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.gc_get_threshold"));
    // Parse arguments.
    Ival generation;
    if(reader.I().v(generation).F()) {
      return std_system_gc_get_threshold(global, ::std::move(generation));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_get_threshold(generation)`

  * Gets the threshold of the collector for `generation`. Valid
    values for `generation` are `0`, `1` and `2`.

  * Returns the threshold. If `generation` is not valid, `null` is
    returned.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.system.gc_set_threshold()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_set_threshold"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.gc_set_threshold"));
    // Parse arguments.
    Ival generation;
    Ival threshold;
    if(reader.I().v(generation).v(threshold).F()) {
      return std_system_gc_set_threshold(global, ::std::move(generation), ::std::move(threshold));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_set_threshold(generation, threshold)`

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
    // `std.system.gc_collect()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_collect"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.gc_collect"));
    // Parse arguments.
    Iopt generation_limit;
    if(reader.I().o(generation_limit).F()) {
      return std_system_gc_collect(global, ::std::move(generation_limit));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_collect([generation_limit])`

  * Performs garbage collection on all generations including and
    up to `generation_limit`. If it is absent, all generations are
    collected.

  * Returns the number of variables that have been collected in
    total.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.system.execute()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("execute"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.execute"));
    // Parse arguments.
    Sval cmd;
    Aopt argv;
    Aopt envp;
    if(reader.I().v(cmd).o(argv).o(envp).F()) {
      return std_system_execute(::std::move(cmd), ::std::move(argv), ::std::move(envp));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.execute(cmd, [argv], [envp])`

  * Launches the program denoted by `cmd`, awaits its termination,
    and returns its exit status. If `argv` is provided, it shall be
    an array of strings, which specify arguments passed to the
    program. If `envp` is provided, it shall also be an array of
    strings, which specify environment variables passed to the
    program.

  * Returns the exit status as an integer. If the process exits due
    to a signal, the exit status is `128+N` where `N` is the signal
    number.

  * Throws an exception if the program could not be launched or its
    exit status could not be retrieved.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.system.daemonize()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("daemonize"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.daemonize"));
    // Parse arguments.
    if(reader.I().F()) {
      std_system_daemonize();
      return true;
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.daemonize()`

  * Detaches the current process from its controlling terminal and
    continues in the background. The calling process terminates on
    success so this function never returns.

  * Throws an exception on failure.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // End of `std.system`
    //===================================================================
  }

}  // namespace Asteria
