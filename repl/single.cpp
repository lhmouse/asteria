// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../asteria/xprecompiled.hpp"
#include "fwd.hpp"
#include "../asteria/simple_script.hpp"
#include "../asteria/value.hpp"
namespace asteria {

void
load_and_execute_single_noreturn()
  {
    // Load and parse the script.
    try {
      if(repl_file == "-")
        repl_script.reload_stdin();
      else
        repl_script.reload_file(repl_file);
    }
    catch(exception& stdex) {
      // Print the error and exit.
      exit_printf(exit_compiler_error, "! exception: %s", stdex.what());
    }

    // Execute the script, passing all command-line arguments to it. If the
    // script exits without returning a value, success is assumed.
    Exit_Status status = exit_non_integer;
    auto ref = repl_script.execute(move(repl_args));
    if(ref.is_void())
      status = exit_success;
    else {
      const auto& val = ref.dereference_readonly();
      if(val.is_integer())
        status = static_cast<Exit_Status>(val.as_integer());
    }

    quick_exit(status);
  }

}  // namespace asteria
