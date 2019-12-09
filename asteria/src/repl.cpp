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
#include <stdio.h>

using namespace Asteria;

namespace {

template<typename XbaseT> int do_backtrace(const XbaseT& xbase) noexcept
  try {
    const auto& except = dynamic_cast<const Runtime_Error&>(xbase);
    rocket::tinyfmt_str fmt;
    // Print stack frames to the standard error stream.
    for(unsigned long i = 0; i != except.count_frames(); ++i) {
      const auto& frm = except.frame(i);
      // Format the value.
      fmt.clear_string();
      fmt << frm.value();
      // Print a line of backtrace.
      ::fprintf(stderr, "  [%lu] %s:%ld (%s) -- %s\n",
                        i, frm.sloc().file().c_str(), frm.sloc().line(), frm.what_type(),
                        fmt.get_c_string());
    }
    return ::fprintf(stderr, "  -- end of backtrace\n");
  }
  catch(std::exception& stdex) {
    return ::fprintf(stderr, "  -- no backtrace available\n");
  }

cow_string do_stringify_value(const Value& val) noexcept
  try {
    rocket::tinyfmt_str fmt;
    fmt << val;
    return fmt.extract_string();
  }
  catch(std::exception& stdex) {
    return rocket::sref("<invalid value>");
  }

cow_string do_stringify_reference(const Reference& ref) noexcept
  try {
    const char* prefix;
    if(ref.is_lvalue())
      prefix = "lvalue ";
    else if(ref.is_rvalue())
      prefix = "rvalue ";
    else
      return rocket::sref("<tail call>");
    // Read a value from the refrence and format it.
    rocket::tinyfmt_str fmt;
    fmt << prefix << ref.read();
    return fmt.extract_string();
  }
  catch(std::exception& other) {
    return rocket::sref("<invalid reference>");
  }

// These hooks just print everything to the standard error stream.
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
                          do_stringify_value(target).c_str());
      }
    void on_function_return(const Source_Location& sloc, const phsh_string& inside,
                            const Reference& result) noexcept override
      {
        ::fprintf(stderr, "~ running: %s:%ld [inside `%s`] -- returned from function call: %s\n",
                          sloc.file().c_str(), sloc.line(), inside.c_str(),
                          do_stringify_reference(result).c_str());
      }
    void on_function_except(const Source_Location& sloc, const phsh_string& inside,
                            const Runtime_Error& except) noexcept override
      {
        ::fprintf(stderr, "~ running: %s:%ld [inside `%s`] -- caught exception from function call: %s\n",
                          sloc.file().c_str(), sloc.line(), inside.c_str(),
                          do_stringify_value(except.value()).c_str());
      }
  };

}

int main(int argc, char** argv)
  try {
    // Add neutral options here.
    bool print_version = false;
    bool print_help = false;

    // Add source options here.
    opt<bool> interactive;
    opt<cow_string> path;
    cow_vector<Value> args;

    // Add miscellaneous options here.
    opt<int> optimization_level;
    bool verbose = false;

    // Parse command line options.
    const char* s;
    int ch;
    size_t n;

    for(;;) {
      ch = ::getopt(argc, argv, "+hIiO::Vv");
      if(ch == -1) {
        // There is no more argument.
        break;
      }
      switch(ch) {
        {{
      case 'h':
          print_help = true;
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
          s = ::optarg;
          // If `-O` is specified without an argument, it is equivalent to `-O1`.
          if(!s || !*s) {
            optimization_level = 1;
            break;
          }
          // Parse the level.
          n = std::strspn(s, "0123456789");
          if((s[n] != 0) || (n >= 4)) {
            ::fprintf(stderr, "%s: invalid argument for `-%c` -- '%s'\n", *argv, ch, s);
            return 127;
          }
          optimization_level = std::atoi(s);
          break;
        }{
      case 'V':
          print_version = true;
          break;
        }{
      case 'v':
          verbose = true;
          break;
        }}
      default:
        // `getopt()` will have written an error message to the standard error stream.
        ::fprintf(stderr, "Try `%s -h` for help.\n", *argv);
        return 127;
      }
    }
    if(::optind < argc) {
      // The first non-option argument is the filename to execute. `-` is not special.
      n = static_cast<uint32_t>(::optind);
      path = G_string(argv[n]);
      // All subsequent arguments are passed to the script verbatim.
      while(++n != static_cast<uint32_t>(argc))
        args.emplace_back(G_string(argv[n]));
    }

    // If version or help information is requested, write it to the standard output then exit.
    if(print_version) {
      // Print version information and exit.
      // Utilize macros from automake.
      ::printf("%s [built on %s]\n"
               "\n"
               "Visit the homepage at <%s>.\n"
               "Report bugs to <%s>.\n",
               PACKAGE_STRING, __DATE__,
               PACKAGE_URL,
               PACKAGE_BUGREPORT);
      return 0;
    }
    if(print_help) {
      // Print help information and exit.
      //       |0        1         2         3         4         5         6         7      |
      //       |1234567890123456789012345678901234567890123456789012345678901234567890123456|
      ::printf("Usage: %s [OPTIONS] [[--] FILE [ARGUMENTS]...]\n"
               "\n"
               "OPTIONS\n"
               "  -h       show help message then exit\n"
               "  -I       suppress interactive mode [default is auto-detecting]\n"
               "  -i       force interactive mode [default is auto-detecting]\n"
               "  -O       equivalent to `-O1`\n"
               "  -O[LVL]  set optimization level to the integer LVL [default is 2]\n"
               "  -V       show version information then exit\n"
               "  -v       print execution details to the standard error\n"
               "\n"
               "Source code is read from the standard input if no FILE is specified or `-`\n"
               "is given as FILE, and from FILE otherwise. All ARGUMENTS following FILE are\n"
               "passed to the script verbatim.\n"
               "\n"
               "If neither `-I` or `-i` is set, interactive mode is enabled when no FILE is\n"
               "given (not even `-`) and the standard input is connected to a terminal, and\n"
               "otherwise it is disabled.\n"
               "\n"
               "When running in non-interactive mode, characters are read from FILE, then\n"
               "compiled and executed. If the script returns an `integer`, it is truncated\n"
               "to 8 bits as an unsigned integer and the result denotes the exit status. If\n"
               "the script returns `null`, the exit status is zero. If the script returns a\n"
               "value that is neither an `integer` nor `null`, or throws an exception, the\n"
               "exit status is non-zero.\n"
               "\n"
               "Visit the homepage at <%s>.\n"
               "Report bugs to <%s>.\n",
               *argv,
               PACKAGE_URL,
               PACKAGE_BUGREPORT);
      return 0;
    }

    // Interactive mode is enabled when no FILE is given (not even `-`) and the standard input is
    // connected to a terminal.
    if(!interactive) {
      interactive = !path && ::isatty(STDIN_FILENO);
    }
    // Read the standard input if no path is specified.
    if(!path) {
      path = rocket::sref("-");
    }
    // Set the default optimization level if no one has been specified.
    // The default optimization level is `2`. Note again that `-O` without an argument is equivalent
    // to `-O1`, which effectively decreases optimization.
    if(!optimization_level) {
      optimization_level = 2;
    }

    // Prepare compiler options.
    Compiler_Options opts = { };
    // An optimization level of zero disables all optimizations.
    if(*optimization_level == 0) {
      opts.no_optimization = true;
    }

    // Prepare the global context.
    Global_Context global;
    // Set a hook struct as requested.
    if(verbose) {
      global.set_hooks(rocket::make_refcnt<Debug_Hooks>());
    }

    if(!*interactive) {
      // Read the script directly from the input stream.
      Simple_Script script;
      script.set_options(opts);
      if(*path == "-")
        script.reload_stdin();
      else
        script.reload_file(*path);
      // Execute the script.
      // The script returns a `Reference` so let's dereference it.
      const auto ref = script.execute(global, rocket::move(args));
      // If the script returns an `integer`, forward its lower 8 bits.
      const auto& val = ref.read();
      if(val.is_integer()) {
        return val.as_integer() & 0xFF;
      }
      // Otherwise, `null` indicates success and all other values indicate failure.
      // N.B. Both boolean values indicate failure.
      return !val.is_null();
    }

    // Write the version information to the standard error stream.
    //                |0        1         2         3         4         5         6         7      |
    //                |1234567890123456789012345678901234567890123456789012345678901234567890123456|
    ::fprintf(stderr, "%s [built on %s]\n"
                      "\n"
                      "  All REPL commands start with a backslash. Type `\\help` for instructions.\n"
                      "  Multiple lines may be joined together using trailing backslashes.\n",
                      PACKAGE_STRING, __DATE__);

    // Trap Ctrl-C. Failure to set the signal handler is ignored.
    static volatile ::sig_atomic_t s_sigint;
    struct ::sigaction sigact = { };
    sigact.sa_handler = [](int) { s_sigint = 1;  };
    sigact.sa_flags = 0;  // non-restartable
    ::sigaction(SIGINT, &sigact, nullptr);

    // In interactive mode (a.k.a. REPL mode), read user inputs in lines.
    unsigned long index = 0;
    int indent;
    cow_string code;
    bool escape;
    long line;

    for(;;) {
      // Check for exit condition.
      if(::ferror(stdin)) {
        ::fprintf(stderr, "\n! error reading standard input\n");
        break;
      }
      if(::feof(stdin)) {
        ::fprintf(stderr, "\n~ have a nice day\n");
        break;
      }
      // Move on and read the next snippet.
      code.clear();
      escape = false;
      line = 0;
      ::fprintf(stderr, "\n#%lu%n:%3lu> ", ++index, &indent, ++line);
      s_sigint = 0;

      for(;;) {
        // Read a character. Break upon read errors.
        ch = ::getc(stdin);
        if(ch == EOF) {
          break;
        }
        if(ch == '\n') {
          // REPL commands can't straddle multiple lines.
          if(code.empty() || (code.front() == '\\')) {
            break;
          }
          // If a line feed isn't escaped, the current snippet is terminated.
          if(!escape) {
            break;
          }
          // Otherwise, a line break is appended and the preceding backslash is deleted.
          // Prompt for the next consecutive line.
          ::fprintf(stderr, "%*c%3lu> ", indent, ':', ++line);
        }
        else {
          // Backslashes not preceding line feeds are preserved as is.
          if(escape) {
            code += '\\';
          }
          // If the character is a backslash, stash it.
          if(ch == '\\') {
            escape = true;
            continue;
          }
        }
        // Append the character.
        code += static_cast<char>(ch);
        escape = false;
      }
      if(s_sigint) {
        // An interrupt has been received. Recover the stream.
        ::clearerr(stdin);
        // Discard all characters that have been read so far.
        ::fprintf(stderr, "\n! interrupted\n");
        continue;
      }
      if(code.empty()) {
        // There is nothing to do.
        continue;
      }

      // Handle REPL commands.
      if(code.front() == '\\') {
        // TODO
        ::printf("! CMD = %s\n", code.c_str());
        continue;
      }

      // Name the snippet.
      cow_string name(64, '*');
      n = (unsigned)::sprintf(name.mut_data(), "snippet #%lu", index);
      name.erase(n);

      // Compile the snippet.
      Simple_Script script;
      script.set_options(opts);
      try {
        // The user may input a complete snippet or merely an expression.
        // First, try parsing it as a statement list.
        script.reload_string(code, name);
      }
      catch(Parser_Error& except) {
        // We only want to make a second attempt in the case of absence of a semicolon at the end.
        if((except.status() != parser_status_semicolon_expected) || (except.line() > 0)) {
          // Bail out upon irrecoverable errors.
          ::fprintf(stderr, "! parser error: %s\n", except.what());
          continue;
        }
      }
      if(!script) {
        // Rewrite the potential expression to a `return` statement.
        code.insert(0, "return ");
        code.append(" ;");
        try {
          // Try parsing it again.
          script.reload_string(code, name);
        }
        catch(Parser_Error& except) {
          // Bail out upon irrecoverable errors.
          ::fprintf(stderr, "! parser error: %s\n", except.what());
          continue;
        }
      }
      // Execute the script.
      try {
        // The script returns a `Reference` so let's dereference it.
        const auto ref = script.execute(global, rocket::move(args));
        // Print the value.
        ::fprintf(stderr, "* value #%lu: %s\n", index, do_stringify_reference(ref).c_str());
      }
      catch(Runtime_Error& except) {
        // Print the exception and discard this snippet.
        ::fprintf(stderr, "! runtime error: %s\n", do_stringify_value(except.value()).c_str());
        do_backtrace(except);
      }
      catch(std::exception& stdex) {
        // Print the exception and discard this snippet.
        ::fprintf(stderr, "! runtime error: %s\n", stdex.what());
        do_backtrace(stdex);
      }
    }
    return 0;
  }
  catch(std::exception& stdex) {
    // Print a message followed by the backtrace if it is available. There isn't much we can do.
    ::fprintf(stderr, "! unhandled exception: %s\n", stdex.what());
    do_backtrace(stdex);
    return 3;
  }
