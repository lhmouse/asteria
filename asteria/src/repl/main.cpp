// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "../simple_script.hpp"
#include "fwd.hpp"
#include <locale.h>  // setlocale()
#include <unistd.h>  // isatty()

namespace {
using namespace ::asteria;

[[noreturn]] int
do_print_help_and_exit(const char* self)
  {
    ::printf(
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
Usage: %s [OPTIONS] [[--] FILE [ARGUMENTS]...]

  -h      show help message then exit
  -I      suppress interactive mode [default = auto]
  -i      force interactive mode [default = auto]
  -O      equivalent to `-O1`
  -O[nn]  set optimization level to `nn` [default = 2]
  -V      show version information then exit
  -v      enable verbose mode

Source code is read from standard input if no FILE is specified or `-` is
given as FILE, and from FILE otherwise. ARGUMENTS following FILE are passed
to the script as strings verbatim, which can be retrieved via `__varg`.

If neither `-I` or `-i` is set, interactive mode is enabled when no FILE is
specified and standard input is connected to a terminal, and is disabled
otherwise. Be advised that specifying `-` explicitly disables interactive
mode.

When running in non-interactive mode, characters are read from FILE, then
compiled and executed. If the script returns an integer, it is truncated to
an 8-bit unsigned integer and then used as the exit status. If the script
returns no value, the exit status is zero. If the script returns a value
that is neither an integer nor void, or throws an exception, the status is
non-zero.

In verbose mode, execution details are printed to standard error. It also
prevents quick termination, which enables some tools such as valgrind to
discover memory leaks upon exit.

Visit the homepage at <%s>.
Report bugs to <%s>.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      self,
      repl_url,
      repl_bug_report);

    ::quick_exit(0);
  }

[[noreturn]] int
do_print_version_and_exit()
  {
    ::printf(
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
%s

Visit the homepage at <%s>.
Report bugs to <%s>.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      repl_package_version,
      repl_url,
      repl_bug_report);

    ::quick_exit(0);
  }

void
do_parse_command_line(int argc, char** argv)
  {
    bool help = false;
    bool version = false;

    opt<bool> verbose;
    opt<bool> interactive;
    opt<int8_t> optimize;

    opt<cow_string> path;
    cow_vector<Value> args;

    // Check for some common options before calling `getopt()`.
    if(argc > 1) {
      if(::strcmp(argv[1], "--help") == 0)
        do_print_help_and_exit(argv[0]);

      if(::strcmp(argv[1], "--version") == 0)
        do_print_version_and_exit();
    }

    // Parse command-line options.
    int ch;
    while((ch = ::getopt(argc, argv, "+hIiO::Vv")) != -1) {
      // Identify a single option.
      switch(ch) {
        case 'h':
          help = true;
          continue;

        case 'I':
          interactive = false;
          continue;

        case 'i':
          interactive = true;
          continue;

        case 'O': {
          // If `-O` is specified without an argument, it is equivalent to `-O1`.
          optimize = int8_t(1);
          if(!optarg || !*optarg)
            continue;

          char* ep;
          long val = ::strtol(optarg, &ep, 10);
          if((*ep != 0) || (val < 0) || (val > 99))
            exit_printf(exit_invalid_argument,
                  "%s: invalid optimization level -- '%s'\n", argv[0], optarg);

          optimize = static_cast<int8_t>(val);
          continue;
        }

        case 'V':
          version = true;
          continue;

        case 'v':
          verbose = true;
          continue;
      }

      // `getopt()` will have written an error message to standard error.
      exit_printf(exit_invalid_argument, "Try `%s -h` for help.\n", argv[0]);
    }

    // Check for early exit conditions.
    if(help)
      do_print_help_and_exit(argv[0]);

    if(version)
      do_print_version_and_exit();

    // If more arguments follow, they denote the script to execute.
    if(optind < argc) {
      // The first non-option argument is the filename to execute.
      // `-` is not special.
      path = V_string(argv[optind]);

      // All subsequent arguments are passed to the script verbatim.
      for(int k = optind + 1;  k != argc;  ++k)
        args.emplace_back(V_string(argv[k]));
    }

    // Verbose mode is off by default.
    if(verbose)
      repl_verbose = *verbose;

    // Interactive mode is enabled when no FILE is given (not even `-`) and
    // standard input is connected to a terminal.
    if(interactive)
      repl_interactive = *interactive;
    else
      repl_interactive = !path && ::isatty(STDIN_FILENO);

    // The default optimization level is `2`.
    // Note again that `-O` without an argument is equivalent to `-O1`, which
    // effectively decreases optimization in comparison to when it wasn't
    // specified.
    if(optimize)
      repl_script.options().optimization_level = *optimize;

    // These arguments are always overwritten.
    repl_file = path.move_value_or(sref("-"));
    repl_args = ::std::move(args);
  }

}  // namespace

int
main(int argc, char** argv)
  try {
    // Select the C locale.
    // UTF-8 is required for wide-oriented standard streams.
    const char* current_locale = ::setlocale(LC_ALL, "C.UTF-8");

    // Note that this function shall not return in case of errors.
    do_parse_command_line(argc, argv);

    // Initialize the global context.
    // The argument is used to check for stack overflows.
    initialize_global_context(&argc);

    // Set up runtime hooks.
    // In non-interactive mode, we would like to run as fast as possible,
    // unless verbosity is requested. In interactive mode, hooks are always
    // installed.
    if(repl_verbose || repl_interactive)
      install_signal_and_verbose_hooks();

    if(repl_verbose)
      repl_script.options().verbose_single_step_traps = true;

    // In non-include mode, read the script, execute it, then exit.
    if(!repl_interactive)
      load_and_execute_single_noreturn();

    // Print the REPL banner.
    repl_printf(
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
%s

  Global locale is now `%s`.

  All REPL commands start with a `:`. Type `:help` for available commands.
  Line breaks within a single snippet shall be escaped with backslashes.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      repl_package_version,
      current_locale);

    prepare_repl_commands();

    for(;;) {
      const STDIO_Sentry sentry;
      ::fputc('\n', stderr);

      // Read a snippet.
      read_execute_print_single();
      if(::ferror(stdin))
        exit_printf(exit_system_error, "! could not read standard input: %m\n");
      else if(::feof(stdin))
        exit_printf(exit_success, "* have a nice day :)\n");
    }
  }
  catch(exception& stdex) {
    // Print a message followed by the backtrace if it is available.
    // There isn't much we can do.
    exit_printf(exit_system_error, "! unhandled exception: %s\n", stdex.what());
  }
