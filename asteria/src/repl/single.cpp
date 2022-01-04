// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../simple_script.hpp"
#include "../value.hpp"

namespace asteria {

void
load_and_execute_single_noreturn()
  try {
    // Load and parse the script.
    try {
      if(repl_file == "-")
        repl_script.reload_stdin();
      else
        repl_script.reload_file(repl_file.c_str());
    }
    catch(exception& stdex) {
      exit_printf(exit_compiler_error, "! error: %s\n", stdex.what());
    }

    // Execute the script, passing all command-line arguments to it.
    auto ref = repl_script.execute(::std::move(repl_args));

    // If the script exits without returning a value, success is assumed.
    if(ref.is_void())
      exit_printf(exit_success);

    // Check whether the result is an integer.
    const auto& val = ref.dereference_readonly();
    if(!val.is_integer())
      exit_printf(exit_non_integer);

    // Exit with this code.
    exit_printf(static_cast<Exit_Status>(val.as_integer()));
  }
  catch(exception& stdex) {
    exit_printf(exit_runtime_error, "! error: %s\n", stdex.what());
  }

}  // namespace asteria
