// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "fwd.hpp"
#include "../simple_script.hpp"
#include "../value.hpp"
namespace asteria {

void
load_and_execute_single_noreturn()
  {
    // Load and parse the script.
    try {
      if(repl_file == "-")
        repl_script.reload_stdin();
      else
        repl_script.reload_file(repl_file.c_str());
    }
    catch(exception& stdex) {
      // Print the error and exit.
      exit_printf(exit_compiler_error, "! error: %s", stdex.what());
    }

    // Execute the script, passing all command-line arguments to it.
    auto ref = repl_script.execute(::std::move(repl_args));

    // If the script exits without returning a value, success is assumed.
    if(ref.is_void())
      ::quick_exit(exit_success);

    // Check whether the result is an integer.
    const auto& val = ref.dereference_readonly();
    if(val.is_integer())
      ::quick_exit((int) val.as_integer());

    // Exit with this code.
    ::quick_exit(exit_non_integer);
  }

}  // namespace asteria
