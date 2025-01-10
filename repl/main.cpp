// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../asteria/xprecompiled.hpp"
#include "fwd.hpp"
#include "../asteria/simple_script.hpp"
#include <locale.h>  // setlocale()
#include <unistd.h>  // isatty()
#include <signal.h>  // sigaction()
namespace {
using namespace ::asteria;

#define PACKAGE_STRING      "asteria master"
#define PACKAGE_URL         "https://github.com/lhmouse/asteria"
#define PACKAGE_BUGREPORT   "lh_mouse@126.com"

[[noreturn]]
int
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
  -O[n]   set optimization level to `n` [default = 2]
  -V      show version information then exit
  -v      enable verbose mode

Source code is read from standard input if no FILE is specified or `-` is
given as FILE, and otherwise from FILE. ARGUMENTS following FILE are passed
to the script as strings verbatim, which can be retrieved via `__varg`.

If neither `-I` or `-i` is set, interactive mode is enabled when no FILE is
specified and standard input is connected to a terminal; otherwise it is
disabled. Be advised, specifying `-` explicitly disables interactive mode.

When running in non-interactive mode, characters are read from FILE, then
compiled and executed. If the script returns an integer, it is truncated to
an 8-bit unsigned integer and then used as the exit status. If the script
returns nothing, the exit status is zero. If the script returns a value that
is not an integer, or throws an exception, the exit status is an unspecified
non-zero value.

In verbose mode, execution details are printed to standard error. It also
prevents quick termination, which enables some tools such as valgrind to
discover memory leaks upon exit.

Visit the homepage at <%s>.
Report bugs to <%s>.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      self,
      PACKAGE_URL,
      PACKAGE_BUGREPORT);

    quick_exit();
  }

[[noreturn]]
int
do_print_version_and_exit()
  {
    ::printf(
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
%s (internal %s)

Visit the homepage at <%s>.
Report bugs to <%s>.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      PACKAGE_STRING, ASTERIA_ABI_VERSION_STRING,
      PACKAGE_URL,
      PACKAGE_BUGREPORT);

    quick_exit();
  }

void
do_parse_command_line(int argc, char** argv)
  {
    bool help = false;
    bool version = false;

    opt<bool> verbose, interactive;
    opt<int> optimize;

    opt<cow_string> path;
    cow_vector<Value> args;

    if(argc > 1) {
      // Check for some common options before calling `getopt()`.
      if(::strcmp(argv[1], "--help") == 0)
        do_print_help_and_exit(argv[0]);

      if(::strcmp(argv[1], "--version") == 0)
        do_print_version_and_exit();
    }

    // Parse command-line options.
    int ch;
    while((ch = ::getopt(argc, argv, "+hIiO::Vv")) != -1) {
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

        case 'O':
          if(!optarg || (optarg[0] < '0') || (optarg[0] > '9') || optarg[1])
            exit_printf(exit_invalid_argument,
                  "%s: `-O` requires an integer argument between 0 and 9.", argv[0]);

          optimize = optarg[0] - '0';
          continue;

        case 'V':
          version = true;
          continue;

        case 'v':
          verbose = true;
          continue;
      }

      // `getopt()` will have written an error message to standard error.
      exit_printf(exit_invalid_argument, "Try `%s -h` for help.", argv[0]);
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
      repl_script.mut_options().optimization_level = uint8_t(*optimize);

    // These arguments are always overwritten.
    repl_file = path.move_value_or(&"-");
    repl_args = move(args);
  }

}  // namespace

int
main(int argc, char** argv)
  try {
    // Select the C locale.
    // UTF-8 is required for wide-oriented standard streams.
    const char* const current_locale = ::setlocale(LC_ALL, "C.UTF-8");
    ::srandom((unsigned) ::clock());
    ::tzset();

    // Note that this function shall not return in case of errors.
    do_parse_command_line(argc, argv);

    // Set up signal handlers and runtime hooks.
    // In non-interactive mode, we would like to run as fast as possible,
    // unless verbosity is requested. In interactive mode, hooks are always
    // installed.
    if(repl_verbose || repl_interactive) {
      install_verbose_hooks();

      struct ::sigaction sigact = { };
      sigact.sa_handler = +[](int sig) { repl_signal.store(sig);  };
      ::sigaction(SIGINT, &sigact, nullptr);
      ::sigaction(SIGWINCH, &sigact, nullptr);
      ::sigaction(SIGCONT, &sigact, nullptr);
    }

    // In non-interactive mode, read the script, execute it, then exit.
    if(!repl_interactive)
      load_and_execute_single_noreturn();

    // Enter interactive mode.
    prepare_repl_commands();

    repl_printf(
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
%s (internal %s)

  Global locale is now `%s`.

  All REPL commands start with a `%c`. Type `%chelp` for available commands.
  Line breaks within a single snippet shall be escaped with backslashes.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      PACKAGE_STRING, ASTERIA_ABI_VERSION_STRING,
      current_locale,
      repl_cmd_char, repl_cmd_char);

    for(;;) {
      // Discard unread input data. Flush buffered output data.
      // Clear EOF and error bits. Clear orientation. Errors are ignored.
      (void) !::freopen(nullptr, "r", stdin);
      (void) !::freopen(nullptr, "w", stdout);

      // Read a snippet and execute it.
      // In case of read errors, this function shall not return.
      read_execute_print_single();
      ::fflush(nullptr);
      ::fputc('\n', stderr);
    }
  }
  catch(exception& stdex) {
    // Print a message followed by the backtrace if it is available.
    // There isn't much we can do.
    exit_printf(exit_system_error, "! unhandled exception: %s", stdex.what());
  }
