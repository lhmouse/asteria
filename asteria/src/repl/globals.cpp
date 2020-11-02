// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../runtime/global_context.hpp"
#include "../simple_script.hpp"
#include <stdarg.h>  // va_list, va_start(), va_end()
#include <stdlib.h>  // exit(), quick_exit()
#include <stdio.h>  // fflush(), fprintf(), stderr

namespace asteria {

const char repl_version[] = PACKAGE_STRING;
::rocket::atomic<bool> repl_interrupted;
Command_Line repl_cmdline;

Global_Context repl_global;
Simple_Script repl_script;

unsigned long repl_index;  // snippet index
cow_string repl_heredoc;  // heredoc terminator

void
exit_printf(Exit_Status stat, const char* fmt, ...)
  noexcept
  {
    // Output the string to standard error.
    ::va_list ap;
    va_start(ap, fmt);
    ::vfprintf(stderr, fmt, ap);
    va_end(ap);

    // Perform normal exit if verbose mode is on.
    // This helps catching memory leaks upon exit.
    if(repl_cmdline.verbose)
      ::exit(static_cast<int>(stat));

    // Perform fast exit by default.
    ::fflush(nullptr);
    ::quick_exit(static_cast<int>(stat));
  }

void
initialize_global_context(const void* stack_base)
  {
    repl_global.set_recursion_base(stack_base);
  }

}  // namespace asteria
