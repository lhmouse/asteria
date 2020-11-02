// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REPL_FWD_HPP_
#define ASTERIA_REPL_FWD_HPP_

#include "../fwd.hpp"

namespace asteria {

// These are process exit status codes.
enum Exit_Status : uint8_t
  {
    exit_success            = 0,
    exit_system_error       = 1,
    exit_invalid_argument   = 2,
    exit_parser_error       = 3,
    exit_runtime_error      = 4,
    exit_non_integer        = 5,
  };

// These are command-line options parsed from `argv`.
struct Command_Line
  {
    // options
    bool verbose = false;
    bool interactive = false;
    Compiler_Options opts;

    // non-options
    cow_string path;
    cow_vector<cow_string> args;
  };

// These are global variables defined in 'globals.cpp'.
extern const char repl_version[];
extern Command_Line repl_cmdline;

extern Global_Context repl_global;
extern Simple_Script repl_script;

extern unsigned long repl_index;  // snippet index
extern cow_string repl_source;  // snippet text
extern cow_string repl_heredoc;  // heredoc terminator

// These functions are defined in 'globals.cpp'.
void
repl_printf(const char* fmt, ...)
  noexcept;

[[noreturn]]
void
exit_printf(Exit_Status stat, const char* fmt = "", ...)
  noexcept;

void
initialize_global_context(const void* stack_base);

// These functions are defined in 'hooks.cpp'.
int
get_and_clear_last_signal()
  noexcept;

void
install_signal_and_verbose_hooks();

// This functions is defined in 'single.cpp'.
[[noreturn]]
void
load_and_execute_single_noreturn();

// This function is defined in 'commands.cpp'.
void
handle_repl_command(cow_string&& cmd, cow_string&& args);

// This function is defined in 'interact.cpp'.
void
read_execute_print_single();

}  // namespace asteria

#endif
