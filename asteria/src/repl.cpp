// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "runtime/reference.hpp"
#include "runtime/variable.hpp"
#include "runtime/global_context.hpp"
#include "runtime/abstract_hooks.hpp"
#include "compiler/parser_error.hpp"
#include "simple_script.hpp"
#include "util.hpp"
#include <locale.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

using namespace asteria;

namespace {

const char*
do_get_build_time()
  {
    // Get the build time.
    // This is done in a static initializer via the "C" locale.
    ::tm tr;
    ::std::memset(&tr, 0, sizeof(tr));
    ::strptime(__DATE__ " " __TIME__, "%b %d %Y %X", &tr);

    // Convert the build time to ISO 8601 format.
    static char s_time_str[64];
    ::strftime(s_time_str, sizeof(s_time_str), "%F %T", &tr);
    return s_time_str;
  }

const auto build_time = do_get_build_time();

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
that is neither an integer nor null, or throws an exception, the status is
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
      PACKAGE_URL,
      PACKAGE_BUGREPORT);

    ::exit(0);
  }

[[noreturn]]
int
do_print_version_and_exit()
  {
    ::printf(
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
%s [built on %s]

Visit the homepage at <%s>.
Report bugs to <%s>.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      PACKAGE_STRING, build_time,
      PACKAGE_URL,
      PACKAGE_BUGREPORT);

    ::exit(0);
  }

// We want to detect Ctrl-C.
volatile ::sig_atomic_t interrupted;

void
do_trap_sigint()
  noexcept
  {
    // Trap Ctrl-C. Failure to set the signal handler is ignored.
    struct ::sigaction sigx = { };
    sigx.sa_handler = [](int) { interrupted = 1;  };
    ::sigaction(SIGINT, &sigx, nullptr);
  }

// Define command-line options here.
struct Command_Line_Options
  {
    // options
    bool verbose = false;
    bool interactive = false;

    // non-options
    cow_string path;
    cow_vector<Value> args;
  };

// These may also be automatic objects but are declared here for
// convenience.
Command_Line_Options cmdline;
Global_Context global;
Simple_Script script;

// These are process exit status codes.
enum Exit_Status : uint8_t
  {
    exit_success            = 0,
    exit_system_error       = 1,
    exit_invalid_argument   = 2,
    exit_parser_error       = 3,
    exit_runtime_error      = 4,
  };

[[noreturn]]
int
do_exit(Exit_Status stat, const char* fmt = nullptr, ...)
  noexcept
  {
    // Output the string to standard error.
    if(fmt) {
      ::va_list ap;
      va_start(ap, fmt);
      ::vfprintf(stderr, fmt, ap);
      va_end(ap);
    }

    if(ROCKET_EXPECT(!cmdline.verbose)) {
      // Perform fast exit by default.
      ::fflush(nullptr);
      ::quick_exit(static_cast<int>(stat));
    }
    else
      // Perform normal exit if verbose mode is on.
      // This helps catching memory leaks upon exit.
      ::exit(static_cast<int>(stat));
  }

class REPL_Hooks
  final
  : public Abstract_Hooks
  {
  private:
    ::rocket::tinyfmt_str m_fmt;  // reusable storage

  private:
    template<typename... ParamsT>
    void
    do_verbose_trace(const Source_Location& sloc, const char* templ,
                     const ParamsT&... params)
      {
        if(ROCKET_EXPECT(!cmdline.verbose))
          return;

        // Compose the string to write.
        this->m_fmt.clear_string();
        this->m_fmt << "REPL running at '" << sloc << "':\n";
        format(this->m_fmt, templ, params...);  // ADL intended

        // Extract the string and write it to standard error.
        // Errors are ignored.
        auto str = this->m_fmt.extract_string();
        write_log_to_stderr(__FILE__, __LINE__, ::std::move(str));

        // Reuse the storage.
        this->m_fmt.set_string(::std::move(str));
      }

  public:
    void
    on_single_step_trap(const Source_Location& sloc)
      override
      {
        if(interrupted)
          ASTERIA_THROW("Interrupt received\n[callback inside '$1']", sloc);
      }

    void
    on_variable_declare(const Source_Location& sloc, const phsh_string& name)
      override
      { this->do_verbose_trace(sloc, "declaring variable `$1`", name);  }

    void
    on_function_call(const Source_Location& sloc, const cow_function& target)
      override
      { this->do_verbose_trace(sloc, "initiating function call: $1", target);  }

    void
    on_function_return(const Source_Location& sloc, const cow_function& target,
                       const Reference&)
      override
      { this->do_verbose_trace(sloc, "returned from function call: $1", target);  }

    void
    on_function_except(const Source_Location& sloc, const cow_function& target,
                       const Runtime_Error&)
      override
      { this->do_verbose_trace(sloc, "caught an exception inside function call: $1",
                                     target);  }
  };

void
do_parse_command_line(int argc, char** argv)
  {
    bool help = false;
    bool version = false;

    opt<int8_t> optimize;
    opt<bool> verbose;
    opt<bool> interactive;
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
            do_exit(exit_invalid_argument,
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
      do_exit(exit_invalid_argument, "Try `%s -h` for help.\n", argv[0]);
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
      path = cow_string(argv[optind]);

      // All subsequent arguments are passed to the script verbatim.
      ::std::for_each(argv + optind + 1, argv + argc,
          [&](const char* arg) { args.emplace_back(cow_string(arg));  });
    }

    // Verbose mode is off by default.
    if(verbose)
      cmdline.verbose = *verbose;

    // Interactive mode is enabled when no FILE is given (not even `-`) and
    // standard input is connected to a terminal.
    if(interactive)
      cmdline.interactive = *interactive;
    else
      cmdline.interactive = !path && ::isatty(STDIN_FILENO);

    // These arguments are always overwritten.
    cmdline.path = path.move_value_or(::rocket::sref("-"));
    cmdline.args = ::std::move(args);

    // The default optimization level is `2`.
    // Note again that `-O` without an argument is equivalent to `-O1`, which
    // effectively decreases optimization in comparison to when it wasn't
    // specified.
    if(optimize)
      script.open_options().optimization_level = *optimize;
  }

int
do_repl_help()
  {
    return ::fputs(
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
* list of commands:
  :help      display information about a given command
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      stderr);
  }

int
do_repl_command(cow_string&& cmd, cow_string&& arg)
  {
    // TODO tokenize
    ::fprintf(stderr, "! CMD: %s\n", cmd.c_str());
    ::fprintf(stderr, "! ARG: %s\n", arg.c_str());

    if(cmd == "help")
      return do_repl_help();

    return ::fprintf(stderr, "! unknown command: %s\n", cmd.c_str());
  }

unsigned long index;  // snippet index
cow_string code;  // snippet
cow_string heredoc;

int
do_REP_single()
  {
    // Reset standard streams.
    if(!::freopen(nullptr, "r", stdin))
      do_exit(exit_system_error,
              "! could not reopen standard input (errno was `%d`)\n", errno);

    if(!::freopen(nullptr, "w", stdout))
      do_exit(exit_system_error,
              "! could not reopen standard output (errno was `%d`)\n", errno);

    // Read the next snippet.
    ::fputc('\n', stderr);
    code.clear();
    interrupted = 0;

    // Prompt for the first line.
    bool escape = false;
    long line = 0;
    int indent;
    ::fprintf(stderr, "#%lu:%lu%n> ", ++index, ++line, &indent);

    for(;;) {
      // Read a character. Break upon read errors.
      int ch = ::fgetc(stdin);
      if(ch == EOF) {
        // Force-move the caret to the next line.
        ::fputc('\n', stderr);
        break;
      }

      if(ch == '\n') {
        // Check for termination.
        if(heredoc.empty()) {
          // In line input mode, the current snippet is terminated by an unescaped
          // line feed.
          if(!escape)
            break;

          // REPL commands can't straddle multiple lines.
          if(!code.empty() && (code.front() == ':'))
            break;
        }
        else {
          // In heredoc mode, the current snippet is terminated by a line consisting
          // of the user-defined terminator, which is not part of the snippet and
          // must be removed.
          if(code.ends_with(heredoc)) {
            code.erase(code.size() - heredoc.size());
            heredoc.clear();
            break;
          }
        }

        // The line feed should be preserved. It'll be appended later.
        // Prompt for the next consecutive line.
        ::fprintf(stderr, "%*lu> ", indent, ++line);
      }
      else if(heredoc.empty()) {
        // In line input mode, backslashes that precede line feeds are deleted.
        // Those that do not precede line feeds are kept as is.
        if(escape)
          code.push_back('\\');

        if(ch == '\\') {
          escape = true;
          continue;
        }
      }

      // Append the character.
      code.push_back(static_cast<char>(ch));
      escape = false;
    }
    if(interrupted)
      return ::fprintf(stderr, "! interrupted (type `:exit` to quit)\n");

    // Check for termination.
    if(::ferror(stdin))
      do_exit(exit_system_error,
              "! could not read standard input (errno was `%d`)\n", errno);

    if(::feof(stdin) && code.empty())
      do_exit(exit_success, "* have a nice day :)\n");

    // If user input was empty, don't do anything.
    if(code.find_first_not_of(" \f\n\r\t\v") == cow_string::npos)
      return 0;

    // Check for REPL commands.
    if(code[0] == ':') {
      code.erase(0, 1);

      // Remove leading spaces.
      size_t pos = code.find_first_not_of(" \t");
      code.erase(0, pos);
      if(code.empty())
        return 0;

      // Get the command name, which is terminated by a space, then erase the
      // separating spaces, as well as trailing ones.
      pos = code.find_first_of(" \t");
      cow_string cmd = code.substr(0, pos);

      pos = code.find_first_not_of(pos, " \t");
      code.erase(0, pos);

      pos = code.find_last_not_of(" \t");
      code.erase(pos + 1);

      // Process the command.
      return do_repl_command(::std::move(cmd), ::std::move(code));
    }

    // Name the snippet.
    ::rocket::tinyfmt_str fmt;
    fmt << "snippet #" << index;
    cmdline.path = fmt.get_string();

    // The snippet might be an expression or a statement list.
    try {
      // First, try parsing it as the former. We do this by complementing the
      // expression to a return statement. As the first line of that expression
      // is supposed to be 'line #1', the `return` statement should start at
      // line zero.
      try {
        script.reload_string(cmdline.path, 0, "return ref\n" + code + "\n;");
      }
      catch(Parser_Error&) {
        // In case of failure, make another attempt to parse the string as a
        // statement list.
        script.reload_string(cmdline.path, code);
      }
    }
    catch(exception& stdex) {
      // Bail out upon irrecoverable errors.
      return ::fprintf(stderr, "! error: %s\n", stdex.what());
    }

    // Execute the script as a function, which returns a `Reference`.
    try {
      const auto ref = script.execute(global, ::std::move(cmdline.args));
      auto str = "[void]";
      if(!ref.is_void()) {
        const auto& val = ref.dereference_readonly();
        val.dump(fmt.clear_string());
        str = fmt.c_str();
      }
      return ::fprintf(stderr, "* result #%lu: %s\n", index, str);
    }
    catch(exception& stdex) {
      // If an exception was thrown, print something informative.
      return ::fprintf(stderr, "! error: %s\n", stdex.what());
    }
  }

[[noreturn]]
int
do_repl_noreturn()
  {
    ::fprintf(stderr,
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
%s [built on %s]

  Global locale is now `%s`.

  All REPL commands start with a `:`. Type `:help` for instructions.
  Multiple lines may be joined together using trailing backslashes.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      PACKAGE_STRING, build_time,
      ::setlocale(LC_ALL, nullptr)
    );

    // Trap Ctrl-C. Failure to set the signal handler is ignored.
    // This also makes I/O functions fail immediately, instead of attempting to
    // try again.
    do_trap_sigint();

    // Set up runtime hooks. This is sticky.
    global.set_hooks(::rocket::make_refcnt<REPL_Hooks>());
    script.open_options().verbose_single_step_traps = true;

    // In interactive mode (a.k.a. REPL mode), read user inputs in lines.
    // Outputs from the script go into standard output. Others go into standard
    // error.
    for(;;)
      do_REP_single();
  }

[[noreturn]]
int
do_single_noreturn()
  {
    // Set up runtime hooks if verbosity is requested.
    if(cmdline.verbose) {
      global.set_hooks(::rocket::make_refcnt<REPL_Hooks>());
      script.open_options().verbose_single_step_traps = true;
    }

    // Consume all data from standard input.
    try {
      if(cmdline.path == "-")
        script.reload_stdin();
      else
        script.reload_file(cmdline.path.c_str());
    }
    catch(exception& stdex) {
      // Report the error and exit.
      do_exit(exit_parser_error, "! error: %s\n", stdex.what());
    }

    // Execute the script.
    try {
      const auto ref = script.execute(global, ::std::move(cmdline.args));
      auto stat = exit_success;
      if(!ref.is_void()) {
        const auto& val = ref.dereference_readonly();
        stat = val.is_integer() ? static_cast<Exit_Status>(val.as_integer())
                                : exit_system_error;
      }
      do_exit(stat);
    }
    catch(exception& stdex) {
      // If an exception was thrown, print something informative.
      do_exit(exit_runtime_error, "! error: %s\n", stdex.what());
    }
  }

}  // namespace

int
main(int argc, char** argv)
  try {
    // Select the C locale. UTF-8 is required for wide-oriented standard streams.
    ::setlocale(LC_ALL, "C.UTF-8");

    // Note that this function shall not return in case of errors.
    do_parse_command_line(argc, argv);

    // Protect against stack overflows.
    global.set_recursion_base(&argc);

    // Call other functions which are declared `noreturn`. `main()` itself is
    // not `noreturn` so we don't get stupid warngings like 'function declared
    // `noreturn` has a `return` statement'.
    if(cmdline.interactive)
      do_repl_noreturn();
    else
      do_single_noreturn();
  }
  catch(exception& stdex) {
    // Print a message followed by the backtrace if it is available. There isn't
    // much we can do.
    do_exit(exit_system_error, "! unhandled exception: %s\n", stdex.what());
  }
