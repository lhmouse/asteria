// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include <config.h>
#include "utilities.hpp"
#include "runtime/reference.hpp"
#include "runtime/global_context.hpp"
#include "runtime/simple_script.hpp"
#include "runtime/runtime_error.hpp"
#include "runtime/abstract_hooks.hpp"
#include "compiler/parser_error.hpp"
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Asteria;

namespace {

enum Exit_Code : uint8_t
  {
    exit_success            = 0,
    exit_unspecified        = 1,
    exit_invalid_argument   = 2,
    exit_parser_error       = 3,
    exit_runtime_error      = 4,
  };

[[noreturn]] void do_quick_exit(Exit_Code code) noexcept
  {
    ::fflush(nullptr);
    ::quick_exit(static_cast<int>(code));
  }

cow_string&& do_xindent(cow_string&& str)
  {
    size_t i, bpos = 0;
    while((i = str.find('\n', bpos)) != cow_string::npos) {
      bpos = i + 2;
      str.replace(i, 1, "\n\t");
    }
    return ::rocket::move(str);
  }

cow_string do_stringify(const Value& val) noexcept
  try {
    ::rocket::tinyfmt_str fmt;
    fmt << val;
    return do_xindent(fmt.extract_string());
  }
  catch(::std::exception& stdex) {
    return ::rocket::sref("<invalid value>");
  }

cow_string do_stringify(const Reference& ref) noexcept
  try {
    ::rocket::tinyfmt_str fmt;
    if(ref.is_constant())
      fmt << "constant ";
    else if(ref.is_temporary())
      fmt << "temporary ";
    else if(ref.is_variable())
      fmt << "variable ";
    else
      return ::rocket::sref("<tail call>");
    fmt << ref.read();
    return do_xindent(fmt.extract_string());
  }
  catch(::std::exception& other) {
    return ::rocket::sref("<invalid reference>");
  }

cow_string do_stringify(const Runtime_Error& except) noexcept
  try {
    ::rocket::tinyfmt_str fmt;
    if(except.value().is_string())
      fmt << except.value().as_string();
    else
      fmt << except.value();
    return do_xindent(fmt.extract_string());
  }
  catch(::std::exception& other) {
    return ::rocket::sref("<invalid exception>");
  }

int do_backtrace(const Runtime_Error& except) noexcept
  try {
    ::rocket::tinyfmt_str fmt;
    ::fprintf(stderr, "\t-- backtrace:\n");
    for(unsigned long i = 0; i != except.count_frames(); ++i) {
      const auto& f = except.frame(i);
      // Format the value.
      fmt.clear_string();
      fmt << f.value();
      // Print a line of backtrace.
      ::fprintf(stderr, "\t * [%lu] %s:%ld (%s) -- %s\n",
                        i, f.sloc().file().c_str(), f.sloc().line(), f.what_type(),
                        fmt.get_c_string());
    }
    return ::fprintf(stderr, "\t-- end of backtrace\n");
  }
  catch(::std::exception& stdex) {
    return ::fprintf(stderr, "\t-- no backtrace available\n");
  }

// These hooks just print everything to standard error.
struct Debug_Hooks : Abstract_Hooks
  {
    void on_variable_declare(const Source_Location& sloc, const phsh_string& inside,
                             const phsh_string& name) noexcept override
      {
        ::fprintf(stderr, "~ running: %s:%ld [inside `%s`] -- declaring variable: %s\n",
                          sloc.file().c_str(), sloc.line(), inside.c_str(),
                          name.c_str());
      }
    void on_function_call(const Source_Location& sloc, const phsh_string& inside,
                          const ckptr<Abstract_Function>& target) noexcept override
      {
        ::fprintf(stderr, "~ running: %s:%ld [inside `%s`] -- initiating function call: %s\n",
                          sloc.file().c_str(), sloc.line(), inside.c_str(),
                          do_stringify(target).c_str());
      }
    void on_function_return(const Source_Location& sloc, const phsh_string& inside,
                            const Reference& result) noexcept override
      {
        ::fprintf(stderr, "~ running: %s:%ld [inside `%s`] -- returned from function call: %s\n",
                          sloc.file().c_str(), sloc.line(), inside.c_str(),
                          do_stringify(result).c_str());
      }
    void on_function_except(const Source_Location& sloc, const phsh_string& inside,
                            const Runtime_Error& except) noexcept override
      {
        ::fprintf(stderr, "~ running: %s:%ld [inside `%s`] -- caught exception from function call: %s\n",
                          sloc.file().c_str(), sloc.line(), inside.c_str(),
                          do_stringify(except).c_str());
      }
  };

// Define command-line options here.
struct Command_Line_Options
  {
    bool help = false;
    bool version = false;

    uint8_t optimize = 0;
    bool verbose = false;
    bool interactive = false;

    cow_string path;
    cow_vector<Value> args;
  };

// These may also be automatic objects. They are declared here for convenience.
Command_Line_Options cmdline;
Compiler_Options options;
Simple_Script script;
Global_Context global;

volatile ::sig_atomic_t interrupted;   // ... except this one, of course.
cow_string heredoc;

void do_parse_command_line(int argc, char** argv)
  {
    opt<bool> help;
    opt<bool> version;
    opt<long> optimize;
    opt<bool> verbose;
    opt<bool> interactive;
    opt<cow_string> path;
    cow_vector<Value> args;

    for(;;) {
      int ch = ::getopt(argc, argv, "+hIiO::Vv");
      if(ch == -1) {
        // There are no more arguments.
        break;
      }
      switch(ch) {
        {{
      case 'h':
          help = true;
          break;
        }{
      case 'I':
          interactive = false;
          break;
        }{
      case 'i':
          interactive = true;
          break;
        }{
      case 'O':
          // If `-O` is specified without an argument, it is equivalent to `-O1`.
          const char* arg = ::optarg;
          if(!arg || !*arg) {
            optimize = 1;
            break;
          }
          // Parse the level.
          char* eptr;
          long ival = ::strtol(arg, &eptr, 10);
          if((*eptr != 0) || (ival < 0) || (ival > 99)) {
            ::fprintf(stderr, "%s: invalid optimization level -- '%s'\n", argv[0], arg);
            do_quick_exit(exit_invalid_argument);
          }
          optimize = ival;
          break;
        }{
      case 'V':
          version = true;
          break;
        }{
      case 'v':
          verbose = true;
          break;
        }}
      default:
        // `getopt()` will have written an error message to standard error.
        ::fprintf(stderr, "Try `%s -h` for help.\n", argv[0]);
        do_quick_exit(exit_invalid_argument);
      }
    }
    if(::optind < argc) {
      // The first non-option argument is the filename to execute. `-` is not special.
      path = G_string(argv[::optind]);
      // All subsequent arguments are passed to the script verbatim.
      ::std::for_each(argv + ::optind + 1, argv + argc,
                      [&](const char* arg) { args.emplace_back(G_string(arg));  });
    }

    // Copy options into `*this`.
    ::cmdline.help = help.value_or(false);
    ::cmdline.version = version.value_or(false);
    // The default optimization level is `2`.
    // Note again that `-O` without an argument is equivalent to `-O1`, which effectively decreases
    // optimization in comparison to when it wasn't specified.
    ::cmdline.optimize = static_cast<uint8_t>(::rocket::clamp(optimize.value_or(2), 0, UINT8_MAX));
    ::cmdline.verbose = verbose.value_or(false);
    // Interactive mode is enabled when no FILE is given (not even `-`) and standard input is
    // connected to a terminal.
    ::cmdline.interactive = interactive ? *interactive : (!path && ::isatty(STDIN_FILENO));
    ::cmdline.path = path.move_value_or(::rocket::sref("-"));
    ::cmdline.args = ::rocket::move(args);
  }

[[noreturn]] void do_print_help_and_exit(const char* self)
  {
    ::printf(
      //        1         2         3         4         5         6         7      |
      // 34567890123456789012345678901234567890123456789012345678901234567890123456|
      "Usage: %s [OPTIONS] [[--] FILE [ARGUMENTS]...]\n"
      "\n"
      "OPTIONS\n"
      "  -h      show help message then exit\n"
      "  -I      suppress interactive mode [default = auto]\n"
      "  -i      force interactive mode [default = auto]\n"
      "  -O      equivalent to `-O1`\n"
      "  -O[nn]  set optimization level to `nn` [default = 2]\n"
      "  -V      show version information then exit\n"
      "  -v      print execution details to standard error\n"
      "\n"
      "Source code is read from standard input if no FILE is specified or `-` is\n"
      "given as FILE, and from FILE otherwise. ARGUMENTS following FILE are passed\n"
      "to the script as `string`s verbatim, which can be retrieved via `__varg`.\n"
      "\n"
      "If neither `-I` or `-i` is set, interactive mode is enabled when no FILE is\n"
      "specified and standard input is connected to a terminal, and is disabled\n"
      "otherwise. Be advised that specifying `-` explicitly disables interactive\n"
      "mode.\n"
      "\n"
      "When running in non-interactive mode, characters are read from FILE, then\n"
      "compiled and executed. If the script returns an `integer`, it is truncated\n"
      "to 8 bits as an unsigned integer and the result denotes the exit status. If\n"
      "the script returns `null`, the exit status is zero. If the script returns a\n"
      "value that is neither an `integer` nor `null`, or throws an exception, the\n"
      "exit status is non-zero.\n"
      "\n"
      "Visit the homepage at <%s>.\n"
      "Report bugs to <%s>.\n"
      // 34567890123456789012345678901234567890123456789012345678901234567890123456|
      //        1         2         3         4         5         6         7      |
      ,
      self,
      PACKAGE_URL,
      PACKAGE_BUGREPORT
    );
    do_quick_exit(exit_success);
  }

[[noreturn]] void do_print_version_and_exit()
  {
    ::printf(
      //        1         2         3         4         5         6         7      |
      // 34567890123456789012345678901234567890123456789012345678901234567890123456|
      "%s [built on %s]\n"
      "\n"
      "Visit the homepage at <%s>.\n"
      "Report bugs to <%s>.\n"
      // 34567890123456789012345678901234567890123456789012345678901234567890123456|
      //        1         2         3         4         5         6         7      |
      ,
      PACKAGE_STRING, __DATE__,
      PACKAGE_URL,
      PACKAGE_BUGREPORT
    );
    do_quick_exit(exit_success);
  }

void do_trap_sigint()
  {
    // Trap Ctrl-C. Failure to set the signal handler is ignored.
    struct ::sigaction sa = { };
    sa.sa_handler = [](int) { ::interrupted = 1;  };
    sa.sa_flags = 0;  // non-restartable
    ::sigaction(SIGINT, &sa, nullptr);
  }

void do_handle_repl_command(cow_string&& cmd)
  {
    ::printf("TODO: REPL CMD %s\n", cmd.c_str());
  }

[[noreturn]] void do_repl_noreturn()
  {
    // Write the title to standard error.
    ::fprintf(stderr,
      //        1         2         3         4         5         6         7      |
      // 34567890123456789012345678901234567890123456789012345678901234567890123456|
      "%s [built on %s]\n"
      "\n"
      "  All REPL commands start with a backslash. Type `\\help` for instructions.\n"
      "  Multiple lines may be joined together using trailing backslashes.\n"
      // 34567890123456789012345678901234567890123456789012345678901234567890123456|
      //        1         2         3         4         5         6         7      |
      ,
      PACKAGE_STRING, __DATE__
    );

    // Trap Ctrl-C. Failure to set the signal handler is ignored.
    // This also makes I/O functions fail immediately, instead of attempting to try again.
    do_trap_sigint();

    // In interactive mode (a.k.a. REPL mode), read user inputs in lines.
    // Outputs from the script go into standard output. Others go into standard error.
    unsigned long index = 0;
    long line;
    int indent, ch;
    bool escape;
    cow_string code;

    do {
      // Clear output.
      ::fputc('\n', stderr);
      // Check for exit condition.
      if(::ferror(stdin)) {
        ::fprintf(stderr, "! error reading standard input\n");
        do_quick_exit(exit_unspecified);
      }
      if(::feof(stdin)) {
        ::fprintf(stderr, "~ have a nice day :)\n");
        do_quick_exit(exit_success);
      }
      // Move on and read the next snippet.
      code.clear();
      escape = false;
      line = 0;
      ::fprintf(stderr, "#%lu:%lu%n> ", ++index, ++line, &indent);
      ::interrupted = 0;

      for(;;) {
        // Read a character. Break upon read errors.
        ch = ::fgetc(stdin);
        if(ch == EOF) {
          break;
        }
        if(ch == '\n') {
          // Check for termination.
          if(heredoc.empty()) {
            // In line input mode, the current snippet is terminated by an unescaped line feed.
            if(!escape) {
              break;
            }
            // REPL commands can't straddle multiple lines.
            if(code.empty() || (code.front() == '\\')) {
              break;
            }
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
      if(::interrupted) {
        // Discard this snippet. Recover the stream so we can read the next one.
        ::clearerr(stdin);
        ::fprintf(stderr, "\n! interrupted\n");
        continue;
      }
      if(code.empty()) {
        continue;
      }

      // Check for REPL commands.
      if(code.front() == '\\') {
        code.erase(0, 1);
        do_handle_repl_command(::rocket::move(code));
        continue;
      }

      // Name the snippet.
      char name[32];
      ch = ::std::sprintf(name, "snippet #%lu", index);
      ::cmdline.path.assign(name, static_cast<unsigned>(ch));

      // The snippet might be a statement list or an expression.
      // First, try parsing it as the former.
      ::script.set_options(::options);
      try {
        ::script.reload_string(code, ::cmdline.path);
      }
      catch(Parser_Error& except) {
        // We only want to make another attempt in the case of absence of a semicolon at the end.
        bool retry = (except.status() == parser_status_semicolon_expected) &&
                     (except.line() <= 0);
        if(retry) {
          // Rewrite the potential expression to a `return` statement.
          code.insert(0, "return& ( ");
          code.append(" );");
          // Try parsing it again.
          try {
            ::script.reload_string(code, ::cmdline.path);
          }
          catch(Parser_Error& /*other*/) {
            // If we fail again, it is the previous exception that we are interested in.
            retry = false;
          }
        }
        if(!retry) {
          // Bail out upon irrecoverable errors.
          ::fprintf(stderr, "! parser error: %s\n", except.what());
          continue;
        }
      }
      // Execute the script as a function, which returns a `Reference`.
      try {
        const auto ref = ::script.execute(::global, ::rocket::move(::cmdline.args));
        // Print the result.
        ::fprintf(stderr, "* result #%lu: %s\n", index, do_stringify(ref).c_str());
      }
      catch(Runtime_Error& except) {
        // If an exception was thrown, print something informative.
        ::fprintf(stderr, "! runtime error: %s\n", do_stringify(except).c_str());
        do_backtrace(except);
      }
      catch(::std::exception& stdex) {
        // If an exception was thrown, print something informative.
        ::fprintf(stderr, "! unhandled exception: %s\n", do_xindent(::rocket::sref(stdex.what())).c_str());
      }
    } while(true);
  }

[[noreturn]] void do_single_noreturn()
  {
    // Consume all data from standard input.
    ::script.set_options(::options);
    try {
      if(::cmdline.path == "-")
        ::script.reload_stdin();
      else
        ::script.reload_file(::cmdline.path);
    }
    catch(Parser_Error& except) {
      // Report the error and exit.
      ::fprintf(stderr, "! parser error: %s\n", except.what());
      do_quick_exit(exit_parser_error);
    }

    // Execute the script.
    Exit_Code status = exit_runtime_error;
    try {
      const auto ref = ::script.execute(::global, ::rocket::move(::cmdline.args));
      const auto& val = ref.read();
      // If the script returned an `integer`, forward its lower 8 bits.
      // Otherwise, `null` indicates success and all other values indicate failure.
      if(val.is_integer())
        status = static_cast<Exit_Code>(val.as_integer());
      else
        status = val.is_null() ? exit_success : exit_unspecified;
    }
    catch(Runtime_Error& except) {
      // If an exception was thrown, print something informative.
      ::fprintf(stderr, "! runtime error: %s\n", do_stringify(except).c_str());
      do_backtrace(except);
    }
    catch(::std::exception& stdex) {
      // If an exception was thrown, print something informative.
      ::fprintf(stderr, "! unhandled exception: %s\n", do_xindent(::rocket::sref(stdex.what())).c_str());
    }
    do_quick_exit(status);
  }

}  // namespace

int main(int argc, char** argv)
  try {
    // Note that this function shall not return in case of errors.
    do_parse_command_line(argc, argv);

    // Check for early exit conditions.
    if(::cmdline.help) {
      do_print_help_and_exit(argv[0]);
    }
    if(::cmdline.version) {
      do_print_version_and_exit();
    }

    // Set debug hooks if verbose mode is enabled. This is sticky.
    if(::cmdline.verbose) {
      global.set_hooks(::rocket::make_refcnt<Debug_Hooks>());
    }
    // Protect against stack overflows.
    global.set_recursion_base(&argc);

    // Call other functions which are declared `noreturn`. `main()` itself is not `noreturn` so we
    // don't get stupid warngings like 'function declared `noreturn` has a `return` statement'.
    if(::cmdline.interactive)
      do_repl_noreturn();
    else
      do_single_noreturn();
  }
  catch(::std::exception& stdex) {
    // Print a message followed by the backtrace if it is available. There isn't much we can do.
    ::fprintf(stderr, "! unhandled exception: %s\n", do_xindent(::rocket::sref(stdex.what())).c_str());
    do_quick_exit(exit_unspecified);
  }
