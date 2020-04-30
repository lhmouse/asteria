// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "system.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/genius_collector.hpp"
#include "../runtime/random_engine.hpp"
#include "../utilities.hpp"
#include <spawn.h>  // ::posix_spawnp()
#include <sys/wait.h>  // ::waitpid()
#include <unistd.h>  // ::daemon()

namespace Asteria {
namespace {

constexpr int64_t xgcgen_newest = static_cast<int64_t>(gc_generation_newest);
constexpr int64_t xgcgen_oldest = static_cast<int64_t>(gc_generation_oldest);
constexpr char s_hex_digits[] = "00112233445566778899AaBbCcDdEeFf";

}  // namespace

optV_integer
std_system_gc_count_variables(Global_Context& global, V_integer generation)
  {
    auto gc_gen = static_cast<GC_Generation>(::rocket::clamp(generation, xgcgen_newest, xgcgen_oldest));
    if(gc_gen != generation)
      return nullopt;

    // Get the current number of variables being tracked.
    auto gcoll = global.genius_collector();
    size_t count = gcoll->get_collector(gc_gen).count_tracked_variables();
    return static_cast<int64_t>(count);
  }

optV_integer
std_system_gc_get_threshold(Global_Context& global, V_integer generation)
  {
    auto gc_gen = static_cast<GC_Generation>(::rocket::clamp(generation, xgcgen_newest, xgcgen_oldest));
    if(gc_gen != generation)
      return nullopt;

    // Get the current threshold.
    auto gcoll = global.genius_collector();
    uint32_t thres = gcoll->get_collector(gc_gen).get_threshold();
    return static_cast<int64_t>(thres);
  }

optV_integer
std_system_gc_set_threshold(Global_Context& global, V_integer generation, V_integer threshold)
  {
    auto gc_gen = static_cast<GC_Generation>(::rocket::clamp(generation, xgcgen_newest, xgcgen_oldest));
    if(gc_gen != generation)
      return nullopt;

    // Set the threshold and return its old value.
    auto gcoll = global.genius_collector();
    uint32_t thres = gcoll->get_collector(gc_gen).get_threshold();
    gcoll->open_collector(gc_gen).set_threshold(static_cast<uint32_t>(::rocket::clamp(threshold, 0, INT32_MAX)));
    return static_cast<int64_t>(thres);
  }

V_integer
std_system_gc_collect(Global_Context& global, optV_integer generation_limit)
  {
    auto gc_limit = gc_generation_oldest;

    // Unlike others, this function does not fail if `generation_limit` is out of range.
    if(generation_limit)
      gc_limit = static_cast<GC_Generation>(::rocket::clamp(*generation_limit, xgcgen_newest, xgcgen_oldest));

    // Perform garbage collection up to the generation specified.
    auto gcoll = global.genius_collector();
    size_t nvars = gcoll->collect_variables(gc_limit);
    return static_cast<int64_t>(nvars);
  }

V_integer
std_system_execute(V_string cmd, optV_array argv, optV_array envp)
  {
    // Append arguments.
    cow_vector<const char*> ptrs = { cmd.safe_c_str() };
    if(argv)
      ::rocket::for_each(*argv, [&](const Value& arg) { ptrs.emplace_back(arg.as_string().safe_c_str());  });
    ptrs.emplace_back(nullptr);
    size_t eoff = 1;  // beginning of environment variables

    // Append environment variables.
    if(envp) {
      eoff = ptrs.size();
      ::rocket::for_each(*envp, [&](const Value& env) { ptrs.emplace_back(env.as_string().safe_c_str());  });
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

void
std_system_daemonize()
  {
    if(::daemon(1, 0) != 0)
      ASTERIA_THROW_SYSTEM_ERROR("daemon");
  }

optV_string
std_system_env_get_variable(V_string name)
  {
    const char* val = ::getenv(name.safe_c_str());
    if(!val)
      return nullopt;
    return cow_string(val);
  }

V_object
std_system_env_get_variables()
  {
    V_object vars;
    const char* const* vpos = ::environ;
    while(const char* str = *(vpos++)) {
      // The key is terminated by an equals sign.
      const char* equ = ::std::strchr(str, '=');
      if(ROCKET_UNEXPECT(!equ))
        vars.insert_or_assign(cow_string(str), ::rocket::sref(""));  // no equals sign?
      else
        vars.insert_or_assign(cow_string(str, equ), cow_string(equ + 1));
    }
    return vars;
  }

V_string
std_system_uuid(Global_Context& global, optV_boolean lowercase)
  {
    // Canonical form: `xxxxxxxx-xxxx-Myyy-Nzzz-wwwwwwwwwwww`
    //  * x: number of 1/10,000 seconds since UNIX Epoch
    //  * M: always `4` (UUID version)
    //  * y: process ID
    //  * N: any of `0`-`7` (UUID variant)
    //  * z: context ID
    //  * w: random bytes
    bool rlowerc = lowercase.value_or(false);
    const auto prng = global.random_engine();
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t x = static_cast<uint64_t>(ts.tv_sec) * 30518 + static_cast<uint64_t>(ts.tv_nsec) / 32768;
    uint64_t y = static_cast<uint32_t>(::getpid());
    uint64_t z = reinterpret_cast<uintptr_t>(::std::addressof(global)) >> 12;
    uint64_t w = static_cast<uint64_t>(prng->bump()) << 32 | static_cast<uint64_t>(prng->bump());

    // Set version and variant.
    y &= 0x0FFF;
    y |= 0x4000;
    z &= 0x7FFF;

    // Compose the string backwards.
    char sbuf[36];
    size_t bp = 36;

    auto put_field_bkwd = [&](uint64_t& value, size_t n)
      {
        for(size_t k = 0;  k != n;  ++k)
          sbuf[--bp] = s_hex_digits[((value << 1) & 0x1E) + rlowerc],
          value >>= 4;
      };

    put_field_bkwd(w, 12);
    sbuf[--bp] = '-';
    put_field_bkwd(z,  4);
    sbuf[--bp] = '-';
    put_field_bkwd(y,  4);
    sbuf[--bp] = '-';
    put_field_bkwd(x,  4);
    sbuf[--bp] = '-';
    put_field_bkwd(x,  8);

    // Return the string. Note there is no null terminator in `sbuf`.
    return cow_string(sbuf, sizeof(sbuf));
  }

void
create_bindings_system(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.system.gc_count_variables()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_count_variables"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_count_variables(generation)`

  * Gets the number of variables that are being tracked by the
    collector for `generation`. Valid values for `generation` are
    `0`, `1` and `2`.

  * Returns the number of variables being tracked. This value is
    only informative. If `generation` is not valid, `null` is
    returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.gc_count_variables"));
    // Parse arguments.
    V_integer generation;
    if(reader.I().v(generation).F()) {
      Reference_root::S_temporary xref = { std_system_gc_count_variables(global, ::std::move(generation)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.gc_get_threshold()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_get_threshold"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_get_threshold(generation)`

  * Gets the threshold of the collector for `generation`. Valid
    values for `generation` are `0`, `1` and `2`.

  * Returns the threshold. If `generation` is not valid, `null` is
    returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.gc_get_threshold"));
    // Parse arguments.
    V_integer generation;
    if(reader.I().v(generation).F()) {
      Reference_root::S_temporary xref = { std_system_gc_get_threshold(global, ::std::move(generation)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.gc_set_threshold()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_set_threshold"),
      V_function(
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
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.gc_set_threshold"));
    // Parse arguments.
    V_integer generation;
    V_integer threshold;
    if(reader.I().v(generation).v(threshold).F()) {
      Reference_root::S_temporary xref = { std_system_gc_set_threshold(global, ::std::move(generation),
                                                                       ::std::move(threshold)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.gc_collect()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_collect"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_collect([generation_limit])`

  * Performs garbage collection on all generations including and
    up to `generation_limit`. If it is absent, all generations are
    collected.

  * Returns the number of variables that have been collected in
    total.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.gc_collect"));
    // Parse arguments.
    optV_integer generation_limit;
    if(reader.I().o(generation_limit).F()) {
      Reference_root::S_temporary xref = { std_system_gc_collect(global, ::std::move(generation_limit)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.execute()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("execute"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.execute(cmd, [argv], [envp])`

  * Launches the program denoted by `cmd`, awaits its termination,
    and returns its exit status. If `argv` is provided, it shall be
    an array of strings, which specify additional arguments to pass
    to the program along with `cmd`. If `envp` is specified, it
    shall also be an array of strings, which specify environment
    variables to pass to the program.

  * Returns the exit status as an integer. If the process exits due
    to a signal, the exit status is `128+N` where `N` is the signal
    number.

  * Throws an exception if the program could not be launched or its
    exit status could not be retrieved.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.execute"));
    // Parse arguments.
    V_string cmd;
    optV_array argv;
    optV_array envp;
    if(reader.I().v(cmd).o(argv).o(envp).F()) {
      Reference_root::S_temporary xref = { std_system_execute(::std::move(cmd), ::std::move(argv),
                                                              ::std::move(envp)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.daemonize()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("daemonize"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.daemonize()`

  * Detaches the current process from its controlling terminal and
    continues in the background. The calling process terminates on
    success so this function never returns.

  * Throws an exception on failure.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.daemonize"));
    // Parse arguments.
    if(reader.I().F()) {
      std_system_daemonize();
      return self = Reference_root::S_void();
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.env_get_variable()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("env_get_variable"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.env_get_variable(name)`

  * Retrieves an environment variable with `name`.

  * Returns the environment variable's value if a match is found,
    or `null` if no such variable exists.

  * Throws an exception if `name` is not valid.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.env_get_variable"));
    // Parse arguments.
    V_string name;
    if(reader.I().v(name).F()) {
      Reference_root::S_temporary xref = { std_system_env_get_variable(::std::move(name)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.env_get_variables()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("env_get_variables"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.env_get_variables()`

  * Retrieves all environment variables.

  * Returns an object of strings which consists of copies of all
    environment variables.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.env_get_variables"));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_system_env_get_variables() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.uuid()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("uuid"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  * Generates a UUID according to the following specification:

    Canonical form: `xxxxxxxx-xxxx-Myyy-Nzzz-wwwwwwwwwwww`

     * x: number of 1/30,518 seconds since UNIX Epoch
     * M: always `4` (UUID version)
     * y: process ID
     * N: any of `0`-`7` (UUID variant)
     * z: context ID
     * w: random bytes

     Unlike version-1 UUIDs in RFC 4122, the timestamp is written
     in pure big-endian order. This ensures the case-insensitive
     lexicographical order of such UUIDs will match their order of
     creation. If `lowercase` is set to `true`, hexadecimal digits
     above `9` are encoded as `abcdef`; otherwise they are encoded
     as `ABCDEF`.

  * Returns a UUID as a string of 36 characters without braces.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.system.uuid"));
    // Parse arguments.
    optV_boolean lowercase;
    if(reader.I().o(lowercase).F()) {
      Reference_root::S_temporary xref = { std_system_uuid(global, lowercase) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
  }

}  // namespace Asteria
