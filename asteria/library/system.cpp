// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "system.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/garbage_collector.hpp"
#include "../runtime/random_engine.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/compiler_error.hpp"
#include "../compiler/enums.hpp"
#include "../utils.hpp"
#include <spawn.h>  // ::posix_spawnp()
#include <sys/wait.h>  // ::waitpid()
#include <unistd.h>  // ::daemon()
#include <time.h>  // ::clock_gettime()
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

struct S_xparse_array
  {
    V_array arr;
  };

struct S_xparse_object
  {
    V_object obj;
    phsh_string key;
    Source_Location key_sloc;
  };

using Xparse = ::rocket::variant<S_xparse_array, S_xparse_object>;

enum Scope_type
  {
    scope_root = 0,
    scope_node = 1,
  };

void
do_accept_object_key(S_xparse_object& ctxo, Token_Stream& tstrm, Scope_type scope)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      throw Compiler_Error(Compiler_Error::M_status(),
                (scope == scope_root) ? compiler_status_identifier_expected
                    : compiler_status_closed_brace_or_json5_key_expected, tstrm.next_sloc());

    switch(weaken_enum(qtok->index())) {
      case Token::index_identifier:
        ctxo.key = qtok->as_identifier();
        break;

      case Token::index_string_literal:
        ctxo.key = qtok->as_string_literal();
        break;

      default:
        throw Compiler_Error(Compiler_Error::M_status(),
                  (scope == scope_root) ? compiler_status_identifier_expected
                      : compiler_status_closed_brace_or_json5_key_expected, tstrm.next_sloc());
    }
    ctxo.key_sloc = qtok->sloc();
    tstrm.shift();

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon, punctuator_assign });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_colon_expected, tstrm.next_sloc());
  }

Value
do_conf_parse_value_nonrecursive(Token_Stream& tstrm)
  {
    // Implement a non-recursive descent parser.
    Value value;
    cow_vector<Xparse> stack;

    for(;;) {
      // Accept a value. No other things such as closed brackets are allowed.
      auto qtok = tstrm.peek_opt();
      if(!qtok)
        throw Compiler_Error(Compiler_Error::M_format(),
                  compiler_status_expression_expected, tstrm.next_sloc(),
                  "Value expected");

      switch(weaken_enum(qtok->index())) {
        case Token::index_punctuator: {
          // Accept an `[` or `{`.
          auto punct = qtok->as_punctuator();
          switch(weaken_enum(punct)) {
            case punctuator_bracket_op: {
              tstrm.shift();

              // Open an array.
              auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
              if(!kpunct) {
                // Descend into the new array.
                stack.emplace_back(S_xparse_array());
                continue;
              }

              // Accept an empty array.
              value = V_array();
              break;
            }

            case punctuator_brace_op: {
              tstrm.shift();

              // Open an object.
              auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
              if(!kpunct) {
                // Descend into the new object.
                stack.emplace_back(S_xparse_object());
                do_accept_object_key(stack.mut_back().mut<1>(), tstrm, scope_node);
                continue;
              }

              // Accept an empty object.
              value = V_object();
              break;
            }

            default:
              throw Compiler_Error(Compiler_Error::M_format(),
                        compiler_status_expression_expected, tstrm.next_sloc(),
                        "Value expected");
          }
          break;
        }

        case Token::index_identifier: {
          // Accept a literal.
          const auto& name = qtok->as_identifier();
          if(::rocket::is_none_of(name, { "null", "true", "false", "infinity", "nan" }))
            throw Compiler_Error(Compiler_Error::M_format(),
                      compiler_status_expression_expected, tstrm.next_sloc(),
                      "Value expected");

          switch(name[3]) {
            case 'l':
              value = nullopt;
              break;

            case 'e':
              value = true;
              break;

            case 's':
              value = false;
              break;

            case 'i':
              value = ::std::numeric_limits<double>::infinity();
              break;

            case '\0':
              value = ::std::numeric_limits<double>::quiet_NaN();
              break;

            default:
              ROCKET_ASSERT(false);
          }
          tstrm.shift();
          break;
        }

        case Token::index_integer_literal:
          // Accept an integer.
          value = qtok->as_integer_literal();
          tstrm.shift();
          break;

        case Token::index_real_literal:
          // Accept a real number.
          value = qtok->as_real_literal();
          tstrm.shift();
          break;

        case Token::index_string_literal:
          // Accept a UTF-8 string.
          value = qtok->as_string_literal();
          tstrm.shift();
          break;

        default:
          throw Compiler_Error(Compiler_Error::M_format(),
                    compiler_status_expression_expected, tstrm.next_sloc(),
                    "Value expected");
      }

      // A complete value has been accepted. Insert it into its parent array or object.
      for(;;) {
        if(stack.empty())
          return value;

        if(stack.back().index() == 0) {
          auto& ctxa = stack.mut_back().mut<0>();
          ctxa.arr.emplace_back(::std::move(value));

          // Check for termination of this array.
          auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
          if(!kpunct) {
            // Look for the next element.
            break;
          }

          // Close this array.
          value = ::std::move(ctxa.arr);
        }
        else {
          auto& ctxo = stack.mut_back().mut<1>();
          auto pair = ctxo.obj.try_emplace(::std::move(ctxo.key), ::std::move(value));
          if(!pair.second)
            throw Compiler_Error(Compiler_Error::M_status(),
                      compiler_status_duplicate_key_in_object, ctxo.key_sloc);

          // Check for termination of this array.
          auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
          if(!kpunct) {
            // Look for the next element.
            do_accept_object_key(ctxo, tstrm, scope_node);
            break;
          }

          // Close this object.
          value = ::std::move(ctxo.obj);
        }

        stack.pop_back();
      }
    }
  }

}  // namespace

V_integer
std_system_gc_count_variables(Global_Context& global, V_integer generation)
  {
    auto rgen = ::rocket::clamp_cast<GC_Generation>(generation, 0, 2);
    if(rgen != generation)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid generation `$1`"),
          generation);

    // Get the current number of variables being tracked.
    const auto gcoll = global.garbage_collector();
    size_t nvars = gcoll->count_tracked_variables(rgen);
    return static_cast<int64_t>(nvars);
  }

V_integer
std_system_gc_get_threshold(Global_Context& global, V_integer generation)
  {
    auto rgen = ::rocket::clamp_cast<GC_Generation>(generation, 0, 2);
    if(rgen != generation)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid generation `$1`"),
          generation);

    // Get the current number of variables being tracked.
    const auto gcoll = global.garbage_collector();
    size_t thres = gcoll->get_threshold(rgen);
    return static_cast<int64_t>(thres);
  }

V_integer
std_system_gc_set_threshold(Global_Context& global, V_integer generation, V_integer threshold)
  {
    auto rgen = ::rocket::clamp_cast<GC_Generation>(generation, 0, 2);
    if(rgen != generation)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid generation `$1`"),
          generation);

    // Set the threshold and return its old value.
    const auto gcoll = global.garbage_collector();
    size_t oldval = gcoll->get_threshold(rgen);
    gcoll->set_threshold(rgen, ::rocket::clamp_cast<size_t>(threshold, 0, PTRDIFF_MAX));
    return static_cast<int64_t>(oldval);
  }

V_integer
std_system_gc_collect(Global_Context& global, optV_integer generation_limit)
  {
    auto rglimit = gc_generation_oldest;
    if(generation_limit) {
      rglimit = ::rocket::clamp_cast<GC_Generation>(*generation_limit, 0, 2);
      if(rglimit != *generation_limit)
        ASTERIA_THROW_RUNTIME_ERROR((
            "Invalid generation limit `$1`"),
            *generation_limit);
    }

    // Perform garbage collection up to the generation specified.
    const auto gcoll = global.garbage_collector();
    size_t nvars = gcoll->collect_variables(rglimit);
    return static_cast<int64_t>(nvars);
  }

optV_string
std_system_env_get_variable(V_string name)
  {
    const char* val = ::secure_getenv(name.safe_c_str());
    if(!val)
      return nullopt;

    // XXX: Use `sref()`?  But environment variables may be unset!
    return cow_string(val);
  }

V_object
std_system_env_get_variables()
  {
    const char* const* vpos = ::environ;
    V_object vars;
    while(const char* str = *(vpos++)) {
      // The key is terminated by an equals sign.
      const char* equ = ::std::strchr(str, '=');
      if(ROCKET_UNEXPECT(!equ))
        vars.insert_or_assign(cow_string(str), sref(""));  // no equals sign?
      else
        vars.insert_or_assign(cow_string(str, equ), cow_string(equ + 1));
    }
    return vars;
  }

V_string
std_system_uuid(Global_Context& global)
  {
    // Canonical form: `xxxxxxxx-xxxx-Myyy-Nzzz-wwwwwwwwwwww`
    //  * x: number of 1/10,000 seconds since UNIX Epoch
    //  * M: always `4` (UUID version)
    //  * y: process ID
    //  * N: any of `0`-`7` (UUID variant)
    //  * z: context ID
    //  * w: random bytes
    const auto prng = global.random_engine();
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t x = (uint64_t) ts.tv_sec * 30518 + (uint32_t) ts.tv_nsec / 32768;
    uint64_t y = (uint32_t) ::getpid();
    uint64_t z = (uint64_t)(void*) &global >> 12;
    uint64_t w = (uint64_t) prng->bump() << 32 | prng->bump();

    // Set version and variant.
    y &= 0x0FFF;
    y |= 0x4000;
    z &= 0x7FFF;

    // Compose the UUID string.
    cow_string str;
    auto wpos = str.insert(str.begin(), 36, '-');

    auto put_hex_uint16 = [&](uint64_t value)
      {
        uint32_t ch;
        for(int k = 3;  k >= 0;  --k)
          ch = (uint32_t) (value >> k * 4) & 0x0F,
            *(wpos++) = (char) ('0' + ch + ((9 - ch) >> 29));
      };

    put_hex_uint16(x >> 32);
    put_hex_uint16(x >> 16);
    wpos++;
    put_hex_uint16(x);
    wpos++;
    put_hex_uint16(y);
    wpos++;
    put_hex_uint16(z);
    wpos++;
    put_hex_uint16(w >> 32);
    put_hex_uint16(w >> 32);
    put_hex_uint16(w);
    return str;
  }

V_integer
std_system_proc_get_pid()
  {
    return ::getpid();
  }

V_integer
std_system_proc_get_ppid()
  {
    return ::getppid();
  }

V_integer
std_system_proc_get_uid()
  {
    return ::getuid();
  }

V_integer
std_system_proc_get_euid()
  {
    return ::geteuid();
  }

V_integer
std_system_proc_invoke(V_string cmd, optV_array argv, optV_array envp)
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
      ASTERIA_THROW_RUNTIME_ERROR((
          "Could not spawn process '$2'",
          "[`posix_spawnp()` failed: $1]"),
          format_errno(), cmd);

    for(;;) {
      // Await its termination.
      // Note: `waitpid()` may return if the child has been stopped or continued.
      int wstat;
      if(::waitpid(pid, &wstat, 0) == -1)
        ASTERIA_THROW_RUNTIME_ERROR((
            "Error awaiting child process '$2'",
            "[`waitpid()` failed: $1]"),
            format_errno(), pid);

      if(WIFEXITED(wstat))
        return WEXITSTATUS(wstat);  // exited

      if(WIFSIGNALED(wstat))
        return 128 + WTERMSIG(wstat);  // killed by a signal
    }
  }

void
std_system_proc_daemonize()
  {
    if(::daemon(1, 0) != 0)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Could not daemonize process",
          "[`daemon()` failed: $1]"),
          format_errno());
  }

V_object
std_system_conf_load_file(V_string path)
  {
    // Initialize tokenizer options.
    // Unlike JSON5, we support _real_ integers and single-quote string literals.
    Compiler_Options opts;
    opts.keywords_as_identifiers = true;

    Token_Stream tstrm(opts);
    ::rocket::tinybuf_file cbuf(path.safe_c_str(), tinybuf::open_read);
    tstrm.reload(path, 1, ::std::move(cbuf));

    // Parse a sequence of key-value pairs.
    S_xparse_object ctxo = { };
    while(!tstrm.empty()) {
      do_accept_object_key(ctxo, tstrm, scope_root);
      auto value = do_conf_parse_value_nonrecursive(tstrm);

      auto pair = ctxo.obj.try_emplace(::std::move(ctxo.key), ::std::move(value));
      if(!pair.second)
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_duplicate_key_in_object, ctxo.key_sloc);
    }
    return ::std::move(ctxo.obj);
  }

void
create_bindings_system(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("gc_count_variables"),
      ASTERIA_BINDING(
        "std.system.gc_count_variables", "generation",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_integer gen;

        reader.start_overload();
        reader.required(gen);
        if(reader.end_overload())
          return (Value) std_system_gc_count_variables(global, gen);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("gc_get_threshold"),
      ASTERIA_BINDING(
        "std.system.gc_get_threshold", "generation",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_integer gen;

        reader.start_overload();
        reader.required(gen);
        if(reader.end_overload())
          return (Value) std_system_gc_get_threshold(global, gen);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("gc_set_threshold"),
      ASTERIA_BINDING(
        "std.system.gc_set_threshold", "generation, threshold",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_integer gen, thrs;

        reader.start_overload();
        reader.required(gen);
        reader.required(thrs);
        if(reader.end_overload())
          return (Value) std_system_gc_set_threshold(global, gen, thrs);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("gc_collect"),
      ASTERIA_BINDING(
        "std.system.gc_collect", "[generation_limit]",
        Global_Context& global, Argument_Reader&& reader)
      {
        optV_integer glim;

        reader.start_overload();
        reader.optional(glim);
        if(reader.end_overload())
          return (Value) std_system_gc_collect(global, glim);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("env_get_variable"),
      ASTERIA_BINDING(
        "std.system.env_get_variable", "name",
        Argument_Reader&& reader)
      {
        V_string name;

        reader.start_overload();
        reader.required(name);
        if(reader.end_overload())
          return (Value) std_system_env_get_variable(name);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("env_get_variables"),
      ASTERIA_BINDING(
        "std.system.env_get_variables", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_env_get_variables();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("uuid"),
      ASTERIA_BINDING(
        "std.system.uuid", "",
        Global_Context& global, Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_uuid(global);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("proc_get_pid"),
      ASTERIA_BINDING(
        "std.system.proc_get_pid", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_proc_get_pid();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("proc_get_ppid"),
      ASTERIA_BINDING(
        "std.system.proc_get_ppid", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_proc_get_ppid();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("proc_get_uid"),
      ASTERIA_BINDING(
        "std.system.proc_get_uid", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_proc_get_uid();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("proc_get_euid"),
      ASTERIA_BINDING(
        "std.system.proc_get_euid", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_system_proc_get_euid();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("proc_invoke"),
      ASTERIA_BINDING(
        "std.system.proc_invoke", "cmd, [argv], [envp]",
        Argument_Reader&& reader)
      {
        V_string cmd;
        optV_array argv, envp;

        reader.start_overload();
        reader.required(cmd);
        reader.optional(argv);
        reader.optional(envp);
        if(reader.end_overload())
          return (Value) std_system_proc_invoke(cmd, argv, envp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("proc_daemonize"),
      ASTERIA_BINDING(
        "std.system.proc_daemonize", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (void) std_system_proc_daemonize();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("conf_load_file"),
      ASTERIA_BINDING(
        "std.system.conf_load_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_system_conf_load_file(path);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
