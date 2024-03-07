// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "system.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/random_engine.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/compiler_error.hpp"
#include "../compiler/enums.hpp"
#include "../utils.hpp"
#include <spawn.h>  // ::posix_spawnp()
#include <sys/wait.h>  // ::waitpid()
#include <sys/utsname.h>  // ::uname()
#include <sys/socket.h>  // ::socket()
#include <time.h>  // ::clock_gettime()
#include <uuid/uuid.h>  // ::uuid_generate_random()
extern char **environ;
namespace asteria {
namespace {

opt<Punctuator>
do_accept_punctuator_opt(Token_Stream& tstrm, initializer_list<Punctuator> accept)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    if(!qtok->is_punctuator())
      return nullopt;

    auto punct = qtok->as_punctuator();
    if(::rocket::is_none_of(punct, accept))
      return nullopt;

    tstrm.shift();
    return punct;
  }

struct Xparse_array
  {
    V_array arr;
  };

struct Xparse_object
  {
    V_object obj;
    phsh_string key;
    Source_Location key_sloc;
  };

using Xparse = ::rocket::variant<Xparse_array, Xparse_object>;

void
do_accept_object_key(Xparse_object& ctxo, Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      throw Compiler_Error(xtc_status,
                compiler_status_identifier_expected, tstrm.next_sloc());

    if(qtok->is_identifier())
      ctxo.key = qtok->as_identifier();
    else if(qtok->is_string_literal())
      ctxo.key = qtok->as_string_literal();
    else
      throw Compiler_Error(xtc_status,
                compiler_status_identifier_expected, tstrm.next_sloc());

    ctxo.key_sloc = qtok->sloc();
    tstrm.shift();

    // A colon or equals sign may follow, but it has no meaning whatsoever.
    do_accept_punctuator_opt(tstrm, { punctuator_colon, punctuator_assign });
  }

Value
do_conf_parse_value_nonrecursive(Token_Stream& tstrm)
  {
    // Implement a non-recursive descent parser.
    Value value;
    cow_vector<Xparse> stack;

    // Accept a value. No other things such as closed brackets are allowed.
  parse_next:
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      throw Compiler_Error(xtc_format,
                compiler_status_expression_expected, tstrm.next_sloc(),
                "Value expected");

    if(qtok->is_punctuator()) {
      // Accept an `[` or `{`.
      if(qtok->as_punctuator() == punctuator_bracket_op) {
        tstrm.shift();

        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
        if(!kpunct) {
          stack.emplace_back(Xparse_array());
          goto parse_next;
        }

        // Accept an empty array.
        value = V_array();
      }
      else if(qtok->as_punctuator() == punctuator_brace_op) {
        tstrm.shift();

        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
        if(!kpunct) {
          stack.emplace_back(Xparse_object());
          do_accept_object_key(stack.mut_back().mut<Xparse_object>(), tstrm);
          goto parse_next;
        }

        // Accept an empty object.
        value = V_object();
      }
      else
        throw Compiler_Error(xtc_format,
                  compiler_status_expression_expected, tstrm.next_sloc(),
                  "Value expected");
    }
    else if(qtok->is_identifier()) {
      // Accept a literal.
      if(qtok->as_identifier() == "null") {
        tstrm.shift();
        value = nullopt;
      }
      else if(qtok->as_identifier() == "true") {
        tstrm.shift();
        value = true;
      }
      else if(qtok->as_identifier() == "false") {
        tstrm.shift();
        value = false;
      }
      else if((qtok->as_identifier() == "Infinity") || (qtok->as_identifier() == "infinity")) {
        tstrm.shift();
        value = ::std::numeric_limits<double>::infinity();
      }
      else if((qtok->as_identifier() == "NaN") || (qtok->as_identifier() == "nan")) {
        tstrm.shift();
        value = ::std::numeric_limits<double>::quiet_NaN();
      }
      else
        throw Compiler_Error(xtc_format,
                  compiler_status_expression_expected, tstrm.next_sloc(),
                  "Value expected");
    }
    else if(qtok->is_integer_literal()) {
      // Accept an integer.
      value = qtok->as_integer_literal();
      tstrm.shift();
    }
    else if(qtok->is_real_literal()) {
      // Accept a real number.
      value = qtok->as_real_literal();
      tstrm.shift();
    }
    else if(qtok->is_string_literal()) {
      // Accept a UTF-8 string.
      value = qtok->as_string_literal();
      tstrm.shift();
    }
    else
      throw Compiler_Error(xtc_format,
                compiler_status_expression_expected, tstrm.next_sloc(),
                "Value expected");

    while(stack.size()) {
      // Advance to the next element.
      auto& ctx = stack.mut_back();
      switch(ctx.index())
        {
        case 0:
          {
            auto& ctxa = ctx.mut<Xparse_array>();
            ctxa.arr.emplace_back(move(value));

            // A comma or semicolon may follow, but it has no meaning whatsoever.
            do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });

            // Look for the next element.
            auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
            if(!kpunct)
              goto parse_next;

            // Close this array.
            value = move(ctxa.arr);
            break;
          }

        case 1:
          {
            auto& ctxo = ctx.mut<Xparse_object>();
            auto pair = ctxo.obj.try_emplace(move(ctxo.key), move(value));
            if(!pair.second)
              throw Compiler_Error(xtc_status,
                        compiler_status_duplicate_key_in_object, ctxo.key_sloc);

            // A comma or semicolon may follow, but it has no meaning whatsoever.
            do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });

            // Look for the next element.
            auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
            if(!kpunct) {
              do_accept_object_key(stack.mut_back().mut<Xparse_object>(), tstrm);
              goto parse_next;
            }

            // Close this object.
            value = move(ctxo.obj);
            break;
          }

        default:
          ROCKET_ASSERT(false);
      }

      stack.pop_back();
    }

    return value;
  }

}  // namespace

V_string
std_system_get_working_directory()
  {
    // Pass a null pointer to request dynamic allocation.
    // Note this behavior is an extension that exists almost everywhere.
    unique_ptr<char, void (void*)> cwd(::getcwd(nullptr, 0), ::free);
    if(!cwd)
      ASTERIA_THROW((
          "Could not get current working directory",
          "[`getcwd()` failed: ${errno:full}]"));

    V_string str(cwd.get());
    return str;
  }

optV_string
std_system_get_environment_variable(V_string name)
  {
    const char* val = ::getenv(name.safe_c_str());
    if(!val)
      return nullopt;

    // XXX: Use `sref()`?  But environment variables may be unset!
    return cow_string(val);
  }

V_object
std_system_get_environment_variables()
  {
    V_object vars;
    for(char** envp = ::environ;  *envp;  envp ++) {
      cow_string key, val;
      bool in_key = true;

      for(char* sp = *envp;  *sp;  sp ++)
        if(!in_key)
          val += *sp;
        else if(*sp == '=')
          in_key = false;
        else
          key += *sp;

      vars.insert_or_assign(move(key), move(val));
    }
    return vars;
  }

V_object
std_system_get_properties()
  {
    V_object names;
    struct ::utsname uts;
    ::uname(&uts);

    names.try_emplace(&"os",
      V_string(
        uts.sysname  // name of the operating system
      ));

    names.try_emplace(&"kernel",
      V_string(
        cow_string(uts.release) + ' ' + uts.version  // name and release of the kernel
      ));

    names.try_emplace(&"arch",
      V_string(
        uts.machine  // name of the CPU architecture
      ));

    names.try_emplace(&"nprocs",
      V_integer(
        ::sysconf(_SC_NPROCESSORS_ONLN)  // number of active CPU cores
      ));

    return names;
  }

V_string
std_system_random_uuid()
  {
    ::uuid_t uuid;
    ::uuid_generate_random(uuid);
    char str[37];
    ::uuid_unparse_lower(uuid, str);
    return V_string(str, 36);
  }

V_integer
std_system_get_pid()
  {
    return ::getpid();
  }

V_integer
std_system_get_ppid()
  {
    return ::getppid();
  }

V_integer
std_system_get_uid()
  {
    return ::getuid();
  }

V_integer
std_system_get_euid()
  {
    return ::geteuid();
  }

V_integer
std_system_call(V_string cmd, optV_array argv, optV_array envp)
  {
    // Append arguments.
    cow_vector<const char*> ptrs = { cmd.safe_c_str() };
    if(argv) {
      ::rocket::for_each(*argv,
          [&](const Value& arg) { ptrs.emplace_back(arg.as_string().safe_c_str());  });
    }
    auto eoff = ptrs.ssize();  // beginning of environment variables
    ptrs.emplace_back(nullptr);

    // Append environment variables.
    if(envp) {
      eoff = ptrs.ssize();
      ::rocket::for_each(*envp,
         [&](const Value& env) { ptrs.emplace_back(env.as_string().safe_c_str());  });
      ptrs.emplace_back(nullptr);
    }
    auto argv_pp = const_cast<char**>(ptrs.data());
    auto envp_pp = const_cast<char**>(ptrs.data() + eoff);

    // Launch the program.
    ::pid_t pid;
    if(::posix_spawnp(&pid, cmd.c_str(), nullptr, nullptr, argv_pp, envp_pp) != 0)
      ASTERIA_THROW((
          "Could not spawn process '$1'",
          "[`posix_spawnp()` failed: ${errno:full}]"),
          cmd);

    for(;;) {
      // Await its termination.
      // Note: `waitpid()` may return if the child has been stopped or continued.
      int wstat;
      if(::waitpid(pid, &wstat, 0) == -1)
        ASTERIA_THROW((
            "Error awaiting child process '$1'",
            "[`waitpid()` failed: ${errno:full}]"),
            pid);

      if(WIFEXITED(wstat))
        return WEXITSTATUS(wstat);  // exited

      if(WIFSIGNALED(wstat))
        return 128 + WTERMSIG(wstat);  // killed by a signal
    }
  }

void
std_system_daemonize()
  {
    // Create a socket for overwriting standard streams in child
    // processes later.
    ::rocket::unique_posix_fd tfd(::socket(AF_UNIX, SOCK_STREAM, 0));
    if(tfd == -1)
      ASTERIA_THROW((
          "Could not create blackhole stream",
          "[`socket()` failed: ${errno:full}]"));

    // Create the CHILD process and wait.
    ::pid_t cpid = ::fork();
    if(cpid == -1)
      ASTERIA_THROW((
          "Could not create child process",
          "[`fork()` failed: ${errno:full}]"));

    if(cpid != 0) {
      // Wait for the CHILD process and forward its exit code.
      int wstatus;
      for(;;)
        if(::waitpid(cpid, &wstatus, 0) != cpid)
          continue;
        else if(WIFEXITED(wstatus))
          ::_Exit(WEXITSTATUS(wstatus));
        else if(WIFSIGNALED(wstatus))
          ::_Exit(128 + WTERMSIG(wstatus));
    }

    // The CHILD shall create a new session and become its leader. This
    // ensures that a later GRANDCHILD will not be a session leader and
    // will not unintentionally gain a controlling terminal.
    ::setsid();

    // Create the GRANDCHILD process.
    cpid = ::fork();
    if(cpid == -1)
      ASTERIA_TERMINATE((
          "Could not create grandchild process",
          "[`fork()` failed: ${errno:full}]"));

    if(cpid != 0) {
      // Exit so the PARENT process will continue.
      ::_Exit(0);
    }

    // Close standard streams in the GRANDCHILD. Errors are ignored.
    // The GRANDCHILD shall continue execution.
    ::shutdown(tfd, SHUT_RDWR);
    (void)! ::dup2(tfd, STDIN_FILENO);
    (void)! ::dup2(tfd, STDOUT_FILENO);
    (void)! ::dup2(tfd, STDERR_FILENO);
  }

V_real
std_system_sleep(V_real duration)
  {
    // Take care of NaNs.
    if(!::std::isgreaterequal(duration, 0))
      return 0;

    V_real secs = duration * 0.001;
    ::timespec ts, rem;

    if(secs > 0x7FFFFFFFFFFFFC00) {
      ts.tv_sec = 0x7FFFFFFFFFFFFC00;
      ts.tv_nsec = 0;
    }
    else if(secs > 0) {
      secs += 0.000000000999;
      ts.tv_sec = (time_t) secs;
      ts.tv_nsec = (long) ((secs - (double) ts.tv_sec) * 1000000000);
    }
    else {
      ts.tv_sec = 0;
      ts.tv_nsec = 0;
    }

    if(::nanosleep(&ts, &rem) == 0)
      return 0;

    // Return the remaining time.
    return (double) rem.tv_sec * 1000 + (double) rem.tv_nsec * 0.000001;
  }

V_object
std_system_load_conf(V_string path)
  {
    // Initialize tokenizer options. Unlike JSON5, we support genuine integers
    // and single-quoted string literals.
    Compiler_Options opts;
    opts.keywords_as_identifiers = true;

    Token_Stream tstrm(opts);
    ::rocket::tinybuf_file cbuf(path.safe_c_str(), tinybuf::open_read);
    tstrm.reload(path, 1, move(cbuf));

    Xparse_object ctxo;
    while(!tstrm.empty()) {
      // Parse the stream for a key-value pair.
      do_accept_object_key(ctxo, tstrm);
      auto value = do_conf_parse_value_nonrecursive(tstrm);

      auto pair = ctxo.obj.try_emplace(move(ctxo.key), move(value));
      if(!pair.second)
        throw Compiler_Error(xtc_status,
                  compiler_status_duplicate_key_in_object, ctxo.key_sloc);

      // A comma or semicolon may follow, but it has no meaning whatsoever.
      do_accept_punctuator_opt(tstrm, { punctuator_comma, punctuator_semicol });
    }

    // Extract the value.
    return move(ctxo.obj);
  }

void
create_bindings_system(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(&"get_working_directory",
      ASTERIA_BINDING(
        "std.system.get_working_directory", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_get_working_directory();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"get_environment_variable",
      ASTERIA_BINDING(
        "std.system.get_environment_variable", "name",
        Argument_Reader&& reader)
      {
        V_string name;

        reader.start_overload();
        reader.required(name);
        if(reader.end_overload())
          return (Value) std_system_get_environment_variable(name);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"get_environment_variables",
      ASTERIA_BINDING(
        "std.system.get_environment_variables", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_get_environment_variables();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"get_properties",
      ASTERIA_BINDING(
        "std.system.get_properties", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_get_properties();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"random_uuid",
      ASTERIA_BINDING(
        "std.system.random_uuid", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_random_uuid();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"get_pid",
      ASTERIA_BINDING(
        "std.system.get_pid", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_get_pid();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"get_ppid",
      ASTERIA_BINDING(
        "std.system.get_ppid", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_get_ppid();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"get_uid",
      ASTERIA_BINDING(
        "std.system.get_uid", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_get_uid();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"get_euid",
      ASTERIA_BINDING(
        "std.system.get_euid", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_get_euid();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"call",
      ASTERIA_BINDING(
        "std.system.call", "cmd, [argv], [envp]",
        Argument_Reader&& reader)
      {
        V_string cmd;
        optV_array argv, envp;

        reader.start_overload();
        reader.required(cmd);
        reader.optional(argv);
        reader.optional(envp);
        if(reader.end_overload())
          return (Value) std_system_call(cmd, argv, envp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"daemonize",
      ASTERIA_BINDING(
        "std.system.daemonize", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (void) std_system_daemonize();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"sleep",
      ASTERIA_BINDING(
        "std.system.sleep", "duration",
        Argument_Reader&& reader)
      {
        V_real duration;

        reader.start_overload();
        reader.required(duration);
        if(reader.end_overload())
          return (Value) std_system_sleep(duration);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"load_conf",
      ASTERIA_BINDING(
        "std.system.load_conf", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_system_load_conf(path);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
