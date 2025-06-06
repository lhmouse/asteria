// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REPL_FWD_
#define ASTERIA_REPL_FWD_

#include "../asteria/fwd.hpp"
#include "../asteria/value.hpp"
namespace asteria {

// These are process exit status codes.
enum Exit_Status : uint8_t
  {
    exit_success            = 0,
    exit_system_error       = 1,
    exit_invalid_argument   = 2,
    exit_compiler_error     = 3,
    exit_runtime_error      = 4,
    exit_non_integer        = 5,
  };

// This character initiates a command.
// Leading blank characters are not allowed.
constexpr char repl_cmd_char = ':';
constexpr int repl_history_size = 1000;

// These are global variables defined in 'globals.cpp'.
extern bool repl_verbose;
extern bool repl_interactive;
extern Simple_Script repl_script;
extern atomic_relaxed<int> repl_signal;

extern unsigned long repl_index;  // snippet index
extern cow_string repl_source;  // snippet text
extern cow_string repl_file;  // name of snippet
extern cow_vector<Value> repl_args;  // script arguments
extern cow_string repl_heredoc;  // heredoc terminator

extern cow_string repl_last_source;
extern cow_string repl_last_file;

// These functions are defined in 'globals.cpp'.
void
repl_vprintf(const char* fmt, ::va_list ap) noexcept;

void
repl_printf(const char* fmt, ...) noexcept;

[[noreturn]]
void
quick_exit(Exit_Status stat = exit_success) noexcept;

[[noreturn]]
void
exit_printf(Exit_Status stat, const char* fmt, ...) noexcept;

void
install_verbose_hooks();

// This function is defined in 'single.cpp'.
[[noreturn]]
void
load_and_execute_single_noreturn();

// These functions are defined in 'commands.cpp'.
void
prepare_repl_commands();

void
handle_repl_command(cow_string&& cmdline);

// This function is defined in 'interact.cpp'.
void
read_execute_print_single();

// These functions are defined in 'editline.cpp'.
void
libedit_set_prompt(const char* fmt, ...);

bool
libedit_gets(cow_string& line);

void
libedit_puts(const cow_string& text);

void
libedit_reset();

void
libedit_add_history(const cow_string& text);

}  // namespace asteria
#endif
