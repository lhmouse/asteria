// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_process.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"
#include <spawn.h>  // ::posix_spawn()
#include <sys/wait.h>  // ::waitpid()
#include <unistd.h>  // ::daemon()

namespace Asteria {
namespace {

[[noreturn]] ROCKET_NOINLINE void throw_system_error(const char* name, int err = errno)
  {
    char sbuf[256];
    const char* msg = ::strerror_r(err, sbuf, sizeof(sbuf));
    ASTERIA_THROW("`$1()` failed (errno was `$2`: $3)", name, err, msg);
  }

}  // namespace

Ival std_process_execute(Sval path, Aopt argv, Aopt envp)
  {
    cow_vector<const char*> ptrs = { path.c_str(), nullptr };
    size_t eoff = 1;  // beginning of environment variables

    // Append arguments. `eoff` will be the offset of the terminating null pointer.
    if(argv) {
      ptrs.clear();
      ::rocket::for_each(*argv, [&](const Value& arg) { ptrs.emplace_back(arg.as_string().c_str());  });
      eoff = ptrs.size();
      ptrs.emplace_back(nullptr);
    }
    // Append environment variables.
    if(envp) {
      eoff = ptrs.size();
      ::rocket::for_each(*envp, [&](const Value& arg) { ptrs.emplace_back(arg.as_string().c_str());  });
      ptrs.emplace_back(nullptr);
    }
    // Be `const`-friendly.
    auto pargv = const_cast<char**>(ptrs.data());
    auto penvp = const_cast<char**>(ptrs.data() + eoff);
    // Launch the program.
    ::pid_t pid;
    if(::posix_spawn(&pid, path.c_str(), nullptr, nullptr, pargv, penvp) != 0)
      throw_system_error("posix_spawn");

    // Await its termination.
    for(;;) {
      // Note: `waitpid()` may return if the child has been stopped or continued.
      int wstat;
      if(::waitpid(pid, &wstat, 0) == -1)
        throw_system_error("waitpid");
      // Check whether the process has terminated normally.
      if(WIFEXITED(wstat))
        return WEXITSTATUS(wstat);
      // Check whether the process has been terminated by a signal.
      if(WIFSIGNALED(wstat))
        return 128 + WTERMSIG(wstat);
    }
  }

void std_process_daemonize()
  {
    if(::daemon(1, 0) != 0)
      throw_system_error("daemon");
  }

void create_bindings_process(Oval& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.process.execute()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("execute"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.process.execute(path, [argv], [envp])`\n"
          "\n"
          "  * Launches the program denoted by `path`, awaits its termination,\n"
          "    and returns its exit status. If `argv` is provided, it shall be\n"
          "    an `array` of `string`s, which specify arguments passed to the\n"
          "    program. If `envp` is provided, it shall also be an `array` of\n"
          "    `string`s, which specify environment variables passed to the\n"
          "    program.\n"
          "\n"
          "  * Returns the exit status as an `integer` using sign-extension.\n"
          "\n"
          "  * Throws an exception if the program could not be launched or its\n"
          "    exit status could not be retrieved.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.process.execute"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          Aopt argv;
          Aopt envp;
          if(reader.I().g(path).g(argv).g(envp).F()) {
            // Call the binding function.
            return std_process_execute(::rocket::move(path), ::rocket::move(argv),
                                                             ::rocket::move(envp));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.process.daemonize()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("daemonize"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.process.daemonize()`\n"
          "\n"
          "  * Detaches the current process from its controlling terminal and\n"
          "    makes it run in the background.\n"
          "\n"
          "  * Detaches the current process from its controlling terminal and\n"
          "    continues in the background. The calling process terminates on\n"
          "    success so this function never returns.\n"
          "\n"
          "  * Throws an exception on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.process.daemonize"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            std_process_daemonize();
            return true;
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // End of `std.process`
    //===================================================================
  }

}  // namespace Asteria
