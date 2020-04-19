// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "runtime/reference.hpp"
#include "runtime/variable.hpp"
#include "runtime/global_context.hpp"
#include "runtime/runtime_error.hpp"
#include "runtime/abstract_hooks.hpp"
#include "compiler/parser_error.hpp"
#include "simple_script.hpp"
#include "utilities.hpp"
#include <locale.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Asteria;

namespace {

cow_string
do_xindent(cow_string&& str)
  {
    size_t bp = SIZE_MAX;
    while((bp = str.find('\n', ++bp)) != cow_string::npos)
      str.insert(++bp, 1, '\t');
    return ::std::move(str);
  }

cow_string
do_stringify(const Value& val)
noexcept
  try {
    ::rocket::tinyfmt_str fmt;
    fmt << val;
    return do_xindent(fmt.extract_string());
  }
  catch(exception& stdex) {
    return ::rocket::sref("<bad value>");
  }

cow_string
do_stringify(const Reference& ref)
noexcept
  try {
    if(ref.is_void())
      return ::rocket::sref("<void>");
    if(ref.is_tail_call())
      return ::rocket::sref("<tail call>");
    ::rocket::tinyfmt_str fmt;
    // Print the value category.
    if(auto var = ref.get_variable_opt())
      if(var->is_immutable())
        fmt << "immutable variable ";
      else
        fmt << "variable ";
    else
      if(ref.is_constant())
        fmt << "constant ";
      else if(ref.is_temporary())
        fmt << "temporary ";
      else
        ROCKET_ASSERT(false);
    // Print the value.
    fmt << ref.read();
    return do_xindent(fmt.extract_string());
  }
  catch(exception& stdex) {
    return ::rocket::sref("<bad reference>");
  }

cow_string
do_stringify(const exception& stdex)
noexcept
  try {
    // Write the exception message verbatim, followed by the dynamic type.
    ::rocket::tinyfmt_str fmt;
    format(fmt, "$1\n[exception class `$2`]", stdex.what(), typeid(stdex).name());
    return do_xindent(fmt.extract_string());
  }
  catch(exception& other) {
    return ::rocket::sref("<bad exception>");
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

// These may also be automatic objects. They are declared here for convenience.
Command_Line_Options cmdline;
Global_Context global;
Simple_Script script;

// These hooks help debugging
struct REPL_Hooks : Abstract_Hooks
  {
    void
    on_variable_declare(const Source_Location& sloc, const cow_string& inside, const phsh_string& name)
    override
      {
        if(ROCKET_EXPECT(!cmdline.verbose))
          return;

        ::fprintf(stderr, "~ running: ['%s:%d' inside `%s`] declaring variable: %s\n",
                          sloc.c_file(), sloc.line(), inside.c_str(), name.c_str());
      }

    void
    on_function_call(const Source_Location& sloc, const cow_string& inside, const cow_function& target)
    override
      {
        if(ROCKET_EXPECT(!cmdline.verbose))
          return;

        ::fprintf(stderr, "~ running: ['%s:%d' inside `%s`] initiating function call: %s\n",
                          sloc.c_file(), sloc.line(), inside.c_str(), do_stringify(target).c_str());
      }

    void
    on_function_return(const Source_Location& sloc, const cow_string& inside, const Reference& result)
    override
      {
        if(ROCKET_EXPECT(!cmdline.verbose))
          return;

        ::fprintf(stderr, "~ running: ['%s:%d' inside `%s`] returned from function call: %s\n",
                          sloc.c_file(), sloc.line(), inside.c_str(), do_stringify(result).c_str());
      }

    void
    on_function_except(const Source_Location& sloc, const cow_string& inside, const Runtime_Error& except)
    override
      {
        if(ROCKET_EXPECT(!cmdline.verbose))
          return;

        ::fprintf(stderr, "~ running: ['%s:%d' inside `%s`] caught exception from function call: %s\n",
                          sloc.c_file(), sloc.line(), inside.c_str(), do_stringify(except).c_str());
      }

    void
    on_single_step_trap(const Source_Location& sloc, const cow_string& inside, Executive_Context* /*ctx*/)
    override
      {
        if(ROCKET_EXPECT(!interrupted))
          return;

        ASTERIA_THROW("interrupt received\n[received at '$1' inside `$2`]", sloc, inside);
      }
  };

enum Exit_Code : uint8_t
  {
    exit_success            = 0,
    exit_unspecified        = 1,
    exit_invalid_argument   = 2,
    exit_parser_error       = 3,
    exit_runtime_error      = 4,
  };

[[noreturn]]
int
do_exit(Exit_Code code)
noexcept
  {
    // Perform normal exit if verbose mode is on, which helps catching memory leaks upon exit.
    if(ROCKET_UNEXPECT(cmdline.verbose))
      ::exit(static_cast<int>(code));

    // Perform fast exit by default.
    ::fflush(nullptr);
    ::quick_exit(static_cast<int>(code));
  }

[[noreturn]]
int
do_print_help(const char* self)
  {
    ::printf(
//        1         2         3         4         5         6         7     |
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
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
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
//        1         2         3         4         5         6         7     |
      self,
      PACKAGE_URL,
      PACKAGE_BUGREPORT);

    do_exit(exit_success);
  }

[[noreturn]]
int
do_print_version()
  {
    ::printf(
//        1         2         3         4         5         6         7     |
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
%s [built on %s]

Visit the homepage at <%s>.
Report bugs to <%s>.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
//        1         2         3         4         5         6         7     |
      PACKAGE_STRING, __DATE__,
      PACKAGE_URL,
      PACKAGE_BUGREPORT);

    do_exit(exit_success);
  }

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
        do_print_help(argv[0]);

      if(::strcmp(argv[1], "--version") == 0)
        do_print_version();
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
          if((*ep != 0) || (val < 0) || (val > 99)) {
            ::fprintf(stderr, "%s: invalid optimization level -- '%s'\n", argv[0], optarg);
            do_exit(exit_invalid_argument);
          }
          optimize = int8_t(val);
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
      ::fprintf(stderr, "Try `%s -h` for help.\n", argv[0]);
      do_exit(exit_invalid_argument);
    }

    // Check for early exit conditions.
    if(help)
      do_print_help(argv[0]);

    if(version)
      do_print_version();

    // If more arguments follow, they denote the script to execute.
    if(optind < argc) {
      // The first non-option argument is the filename to execute. `-` is not special.
      path = cow_string(argv[optind]);
      // All subsequent arguments are passed to the script verbatim.
      ::std::for_each(argv + optind + 1, argv + argc,
                      [&](const char* arg) { args.emplace_back(cow_string(arg));  });
    }

    // Verbose mode is off by default.
    if(verbose)
      cmdline.verbose = *verbose;

    // Interactive mode is enabled when no FILE is given (not even `-`) and standard input is
    // connected to a terminal.
    if(interactive)
      cmdline.interactive = *interactive;
    else
      cmdline.interactive = !path && ::isatty(STDIN_FILENO);

    // These arguments are always overwritten.
    cmdline.path = path.move_value_or(::rocket::sref("-"));
    cmdline.args = ::std::move(args);

    // The default optimization level is `2`.
    // Note again that `-O` without an argument is equivalent to `-O1`, which effectively decreases
    // optimization in comparison to when it wasn't specified.
    if(optimize)
      script.open_options().optimization_level = *optimize;
  }

void
do_REPL_help()
  {
    ::fputs(
//        1         2         3         4         5         6         7     |
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
* list of commands:
  :help      display information about a given command
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
//        1         2         3         4         5         6         7     |
      stderr);
  }

void
do_handle_REPL_command(cow_string&& cmd)
  {
    // TODO tokenize
    if(cmd == "help")
      return do_REPL_help();

    ::fprintf(stderr, "! unknown command: %s\n", cmd.c_str());
  }

unsigned long index;  // snippet index
cow_string code;  // snippet
cow_string heredoc;

int
do_REP_single()
  {
    // Reset standard streams.
    if(!::freopen(nullptr, "r", stdin)) {
      ::fprintf(stderr, "! could not reopen standard input (errno was `%d`)\n", errno);
      ::abort();
    }
    if(!::freopen(nullptr, "w", stdout)) {
      ::fprintf(stderr, "! could not reopen standard output (errno was `%d`)\n", errno);
      ::abort();
    }
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
          // In line input mode, the current snippet is terminated by an unescaped line feed.
          if(!escape)
            break;
          // REPL commands can't straddle multiple lines.
          if(!code.empty() && (code.front() == ':'))
            break;
        }
        else {
          // In heredoc mode, the current snippet is terminated by a line consisting of the
          // user-defined terminator, which is not part of the snippet and must be removed.
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
        // In line input mode, backslashes that precede line feeds are deleted. Those that
        // do not precede line feeds are kept as is.
        if(escape) {
          code.push_back('\\');
        }
        if(ch == '\\') {
          escape = true;
          continue;
        }
      }
      // Append the character.
      code.push_back(static_cast<char>(ch));
      escape = false;
    }
    if(interrupted) {
      // Discard this snippet and read the next one.
      ::fprintf(stderr, "! interrupted\n");
      return SIGINT;
    }
    if(::ferror(stdin)) {
      // Discard the last (partial) snippet in case of read errors.
      ::fprintf(stderr, "! error reading standard input\n");
      do_exit(exit_unspecified);
    }
    if(::feof(stdin) && code.empty()) {
      // Exit normally.
      ::fprintf(stderr, "* have a nice day :)\n");
      do_exit(exit_success);
    }
    if(code.empty()) {
      // Do nothing.
      return 0;
    }
    if(code.front() == ':') {
      // Erase the initiator and process the remaining.
      code.erase(0, 1);
      do_handle_REPL_command(::std::move(code));
      return 0;
    }

    // Name the snippet.
    char name[32];
    size_t nlen = (unsigned)::std::sprintf(name, "snippet #%lu", index);
    cmdline.path.assign(name, nlen);

    // The snippet might be a statement list or an expression.
    // First, try parsing it as the former.
    try {
      script.reload_string(code, cmdline.path);
    }
    catch(Parser_Error& except) {
      // We only want to make another attempt in the case of absence of a semicolon at the end.
      bool retry = (except.status() == parser_status_semicolon_expected) && (except.line() < 0);
      if(retry) {
        // Rewrite the potential expression to a `return` statement.
        code.insert(0, "return& ( ");
        code.append(" );");
        // Try parsing it again.
        try {
          script.reload_string(code, cmdline.path);
        }
        catch(Parser_Error& /*other*/) {
          // If we fail again, it is the previous exception that we are interested in.
          retry = false;
        }
      }
      if(!retry) {
        // Bail out upon irrecoverable errors.
        ::fprintf(stderr, "! %s\n", do_stringify(except).c_str());
        return SIGPIPE;
      }
    }

    // Execute the script as a function, which returns a `Reference`.
    try {
      const auto ref = script.execute(global, ::std::move(cmdline.args));
      // Ensure it is dereferenceable.
      if(!ref.is_void())
        static_cast<void>(ref.read());
      // Print the result.
      ::fprintf(stderr, "* result #%lu: %s\n", index, do_stringify(ref).c_str());
    }
    catch(exception& stdex) {
      // If an exception was thrown, print something informative.
      ::fprintf(stderr, "! %s\n", do_stringify(stdex).c_str());
    }
    return 0;
  }

[[noreturn]]
int
do_REPL_noreturn()
  {
    // Write the title to standard error.
    ::fprintf(stderr,
      //        1         2         3         4         5         6         7      |
      // 34567890123456789012345678901234567890123456789012345678901234567890123456|
      "%s [built on %s]\n"
      "\n"
      "  Global locale is now `%s`.\n"
      "\n"
      "  All REPL commands start with a `:`. Type `:help` for instructions.\n"
      "  Multiple lines may be joined together using trailing backslashes.\n"
      // 34567890123456789012345678901234567890123456789012345678901234567890123456|
      //        1         2         3         4         5         6         7      |
      ,
      PACKAGE_STRING, __DATE__,
      ::setlocale(LC_ALL, nullptr)
    );

    // Trap Ctrl-C. Failure to set the signal handler is ignored.
    // This also makes I/O functions fail immediately, instead of attempting to try again.
    do_trap_sigint();

    // Set up runtime hooks. This is sticky.
    global.set_hooks(::rocket::make_refcnt<REPL_Hooks>());
    script.open_options().verbose_single_step_traps = true;

    // In interactive mode (a.k.a. REPL mode), read user inputs in lines.
    // Outputs from the script go into standard output. Others go into standard error.
    for(;;)
      do_REP_single();
  }

[[noreturn]]
int
do_single_noreturn()
  {
    // Return this if an exception is thrown.
    Exit_Code status = exit_runtime_error;

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
    catch(Parser_Error& except) {
      // Report the error and exit.
      ::fprintf(stderr, "! %s\n", do_stringify(except).c_str());
      do_exit(exit_parser_error);
    }

    // Execute the script.
    try {
      const auto ref = script.execute(global, ::std::move(cmdline.args));
      if(ref.is_void()) {
        // If the script returned no value, exit with zero.
        status = exit_success;
      }
      else {
        // If the script returned an `integer`, forward its lower 8 bits.
        // Any other value indicates failure.
        const auto& val = ref.read();
        if(val.is_integer())
          status = static_cast<Exit_Code>(val.as_integer() & 0xFF);
        else
          status = exit_unspecified;
      }
    }
    catch(exception& stdex) {
      // If an exception was thrown, print something informative.
      ::fprintf(stderr, "! %s\n", do_stringify(stdex).c_str());
    }
    do_exit(status);
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

    // Call other functions which are declared `noreturn`. `main()` itself is not `noreturn` so we
    // don't get stupid warngings like 'function declared `noreturn` has a `return` statement'.
    if(cmdline.interactive)
      do_REPL_noreturn();
    else
      do_single_noreturn();
  }
  catch(exception& stdex) {
    // Print a message followed by the backtrace if it is available. There isn't much we can do.
    ::fprintf(stderr, "! %s\n", do_stringify(stdex).c_str());
    do_exit(exit_unspecified);
  }
