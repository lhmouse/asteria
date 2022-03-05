// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../simple_script.hpp"
#include <stdarg.h>  // va_list, va_start(), va_end()
#include <stdlib.h>  // exit(), quick_exit()
#include <stdio.h>  // fflush(), fprintf(), stderr

namespace asteria {

const char repl_name[] = PACKAGE_NAME;
const char repl_version[] = VERSION;
const char repl_url[] = PACKAGE_URL;
const char repl_bug_report[] = PACKAGE_BUGREPORT;
const char repl_tar_name[] = PACKAGE_TARNAME;

bool repl_verbose;
bool repl_interactive;
Simple_Script repl_script;

unsigned long repl_index;  // snippet index
cow_string repl_source;  // snippet text
cow_string repl_file;  // name of snippet
cow_vector<Value> repl_args;  // script arguments
cow_string repl_heredoc;  // heredoc terminator

cow_string repl_last_source;
cow_string repl_last_file;

void
repl_vprintf(const char* fmt, ::va_list ap) noexcept
  {
    ::flockfile(stderr);
    ::vfprintf(stderr, fmt, ap);
    ::fputc_unlocked('\n', stderr);
    ::funlockfile(stderr);
  }

void
repl_printf(const char* fmt, ...) noexcept
  {
    // Output the string to standard error.
    ::va_list ap;
    va_start(ap, fmt);
    repl_vprintf(fmt, ap);
    va_end(ap);
  }

void
exit_printf(Exit_Status stat, const char* fmt, ...) noexcept
  {
    // Output the string to standard error.
    ::va_list ap;
    va_start(ap, fmt);
    repl_vprintf(fmt, ap);
    va_end(ap);

    // Perform normal exit if verbose mode is on.
    // This helps catching memory leaks upon exit.
    if(repl_verbose)
      ::exit(static_cast<int>(stat));

    // Perform fast exit by default.
    ::fflush(nullptr);
    ::quick_exit(static_cast<int>(stat));
  }

void
initialize_global_context(const void* stack_base)
  {
    repl_script.global().set_recursion_base(stack_base);
  }

}  // namespace asteria
