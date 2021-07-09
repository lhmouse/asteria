// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
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

inline void
do_put_FFFF(cow_string::iterator wpos, bool rlowerc, uint64_t value)
  {
    static constexpr char xdigits[] = "00112233445566778899AaBbCcDdEeFf";
    for(long k = 0;  k != 4;  ++k)
      wpos[k] = xdigits[(value >> (12 - 4 * k)) % 16 * 2 + rlowerc];
  }

inline bool
do_check_punctuator(const Token* qtok, initializer_list<Punctuator> accept)
  {
    return qtok && qtok->is_punctuator()
                && ::rocket::is_any_of(qtok->as_punctuator(), accept);
  }

struct Key_with_sloc
  {
    Source_Location sloc;
    size_t length;
    phsh_string name;
  };

opt<Key_with_sloc>
do_accept_object_key_opt(Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    // A key may be either an identifier or a string literal.
    Key_with_sloc key;
    switch(weaken_enum(qtok->index())) {
      case Token::index_identifier:
        key.name = qtok->as_identifier();
        break;

      case Token::index_string_literal:
        key.name = qtok->as_string_literal();
        break;

      default:
        return nullopt;
    }
    key.sloc = qtok->sloc();
    key.length = qtok->length();
    tstrm.shift();

    // Accept the value initiator.
    qtok = tstrm.peek_opt();
    if(!do_check_punctuator(qtok, { punctuator_assign, punctuator_colon }))
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_equals_sign_or_colon_expected, tstrm.next_sloc());

    tstrm.shift();
    return ::std::move(key);
  }

Value&
do_insert_unique(V_object& obj, Key_with_sloc&& key, Value&& value)
  {
    auto pair = obj.try_emplace(::std::move(key.name), ::std::move(value));
    if(!pair.second)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_duplicate_key_in_object, key.sloc);

    return pair.first->second;
  }

struct S_xparse_array
  {
    V_array arr;
  };

struct S_xparse_object
  {
    V_object obj;
    Key_with_sloc key;
  };

using Xparse = ::rocket::variant<S_xparse_array, S_xparse_object>;

Value
do_conf_parse_value_nonrecursive(Token_Stream& tstrm)
  {
    Value value;

    // Implement a non-recursive descent parser.
    cow_vector<Xparse> stack;

    for(;;) {
      // Accept a value. No other things such as closed brackets are allowed.
      auto qtok = tstrm.peek_opt();
      if(!qtok)
        throw Compiler_Error(Compiler_Error::M_format(),
                  compiler_status_expression_expected, tstrm.next_sloc(),
                  "value expected");

      switch(weaken_enum(qtok->index())) {
        case Token::index_punctuator: {
          // Accept an `[` or `{`.
          auto punct = qtok->as_punctuator();
          switch(weaken_enum(punct)) {
            case punctuator_bracket_op: {
              tstrm.shift();

              // Open an array.
              qtok = tstrm.peek_opt();
              if(!qtok) {
                throw Compiler_Error(Compiler_Error::M_status(),
                          compiler_status_closed_bracket_or_comma_expected, tstrm.next_sloc());
              }
              else if(!do_check_punctuator(qtok, { punctuator_bracket_cl })) {
                // Descend into the new array.
                S_xparse_array ctxa = { V_array() };
                stack.emplace_back(::std::move(ctxa));
                continue;
              }
              tstrm.shift();

              // Accept an empty array.
              value = V_array();
              break;
            }

            case punctuator_brace_op: {
              tstrm.shift();

              // Open an object.
              qtok = tstrm.peek_opt();
              if(!qtok) {
                throw Compiler_Error(Compiler_Error::M_status(),
                          compiler_status_closed_brace_or_comma_expected, tstrm.next_sloc());
              }
              else if(!do_check_punctuator(qtok, { punctuator_brace_cl })) {
                // Get the first key.
                auto qkey = do_accept_object_key_opt(tstrm);
                if(!qkey)
                  throw Compiler_Error(Compiler_Error::M_status(),
                            compiler_status_closed_brace_or_json5_key_expected, tstrm.next_sloc());

                // Descend into the new object.
                S_xparse_object ctxo = { V_object(), ::std::move(*qkey) };
                stack.emplace_back(::std::move(ctxo));
                continue;
              }
              tstrm.shift();

              // Accept an empty object.
              value = V_object();
              break;
            }

            default:
              throw Compiler_Error(Compiler_Error::M_format(),
                        compiler_status_expression_expected, tstrm.next_sloc(),
                        "value expected");
          }
          break;
        }

        case Token::index_identifier: {
          // Accept a literal.
          const auto& name = qtok->as_identifier();
          if(::rocket::is_none_of(name, { "null", "true", "false", "infinity", "nan" }))
            throw Compiler_Error(Compiler_Error::M_format(),
                      compiler_status_expression_expected, tstrm.next_sloc(),
                      "value expected");

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

            case 0:
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
          // Accept a real.
          value = qtok->as_real_literal();
          tstrm.shift();
          break;

        case Token::index_string_literal:
          // Accept a UTF-8 string.
          value = qtok->as_string_literal();
          tstrm.shift();
          break;

        default:
          throw Compiler_Error(Compiler_Error::M_status(),
                    compiler_status_expression_expected, tstrm.next_sloc());
      }

      // A complete value has been accepted. Insert it into its parent array or object.
      for(;;) {
        if(stack.empty())
          // Accept the root value.
          return value;

        if(stack.back().index() == 0) {
          auto& ctxa = stack.mut_back().as<0>();
          ctxa.arr.emplace_back(::std::move(value));

          // Check for termination.
          qtok = tstrm.peek_opt();
          if(!qtok) {
            throw Compiler_Error(Compiler_Error::M_status(),
                      compiler_status_closed_bracket_or_comma_expected, tstrm.next_sloc());
          }
          else if(!do_check_punctuator(qtok, { punctuator_bracket_cl })) {
            // Look for the next element.
            break;
          }
          tstrm.shift();

          // Close this array.
          value = ::std::move(ctxa.arr);
        }
        else {
          auto& ctxo = stack.mut_back().as<1>();
          do_insert_unique(ctxo.obj, ::std::move(ctxo.key), ::std::move(value));

          // Check for termination.
          qtok = tstrm.peek_opt();
          if(!qtok) {
            throw Compiler_Error(Compiler_Error::M_status(),
                      compiler_status_closed_brace_or_comma_expected, tstrm.next_sloc());
          }
          else if(!do_check_punctuator(qtok, { punctuator_brace_cl })) {
            // Get the next key.
            auto qkey = do_accept_object_key_opt(tstrm);
            if(!qkey)
              throw Compiler_Error(Compiler_Error::M_status(),
                        compiler_status_closed_brace_or_json5_key_expected, tstrm.next_sloc());

            // Look for the next value.
            ctxo.key = ::std::move(*qkey);
            break;
          }
          tstrm.shift();

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
    uint8_t rgen = ::rocket::clamp_cast<uint8_t>(generation, 0, 2);
    if(rgen != generation)
      ASTERIA_THROW_RUNTIME_ERROR("invalid generation `$1`", generation);

    // Get the current number of variables being tracked.
    const auto gcoll = global.garbage_collector();
    size_t nvars = gcoll->count_tracked_variables(rgen);
    return static_cast<int64_t>(nvars);
  }

V_integer
std_system_gc_get_threshold(Global_Context& global, V_integer generation)
  {
    uint8_t rgen = ::rocket::clamp_cast<uint8_t>(generation, 0, 2);
    if(rgen != generation)
      ASTERIA_THROW_RUNTIME_ERROR("invalid generation `$1`", generation);

    // Get the current number of variables being tracked.
    const auto gcoll = global.garbage_collector();
    size_t thres = gcoll->get_threshold(rgen);
    return static_cast<int64_t>(thres);
  }

V_integer
std_system_gc_set_threshold(Global_Context& global, V_integer generation, V_integer threshold)
  {
    uint8_t rgen = ::rocket::clamp_cast<uint8_t>(generation, 0, 2);
    if(rgen != generation)
      ASTERIA_THROW_RUNTIME_ERROR("invalid generation `$1`", generation);

    // Set the threshold and return its old value.
    const auto gcoll = global.garbage_collector();
    size_t oldval = gcoll->get_threshold(rgen);
    gcoll->set_threshold(rgen, ::rocket::clamp_cast<size_t>(threshold, 0, PTRDIFF_MAX));
    return static_cast<int64_t>(oldval);
  }

V_integer
std_system_gc_collect(Global_Context& global, Opt_integer generation_limit)
  {
    uint8_t rglimit = 2;
    if(generation_limit) {
      rglimit = ::rocket::clamp_cast<uint8_t>(*generation_limit, 0, 2);
      if(rglimit != *generation_limit)
        ASTERIA_THROW_RUNTIME_ERROR("invalid generation limit `$1`", *generation_limit);
    }

    // Perform garbage collection up to the generation specified.
    const auto gcoll = global.garbage_collector();
    size_t nvars = gcoll->collect_variables(rglimit);
    return static_cast<int64_t>(nvars);
  }

Opt_string
std_system_env_get_variable(V_string name)
  {
    const char* val = ::getenv(name.safe_c_str());
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
std_system_uuid(Global_Context& global, Opt_boolean lowercase)
  {
    bool rlowerc = lowercase.value_or(false);

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

    uint64_t x = uint64_t(ts.tv_sec) * 30518 + uint64_t(ts.tv_nsec) / 32768;
    uint64_t y = uint32_t(::getpid());
    uint64_t z = uintptr_t((void*)&global) >> 12;
    uint64_t w = uint64_t(prng->bump()) << 32 | prng->bump();

    // Set version and variant.
    y &= 0x0FFF;
    y |= 0x4000;
    z &= 0x7FFF;

    // Compose the UUID string.
    cow_string uuid_str;
    auto wpos = uuid_str.insert(uuid_str.begin(), 36, '-');

    do_put_FFFF(wpos +  0, rlowerc, x >> 32);
    do_put_FFFF(wpos +  4, rlowerc, x >> 16);
    do_put_FFFF(wpos +  9, rlowerc, x >>  0);
    do_put_FFFF(wpos + 14, rlowerc, y);
    do_put_FFFF(wpos + 19, rlowerc, z);
    do_put_FFFF(wpos + 24, rlowerc, w >> 32);
    do_put_FFFF(wpos + 28, rlowerc, w >> 32);
    do_put_FFFF(wpos + 32, rlowerc, w >>  0);
    return uuid_str;
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
std_system_proc_invoke(V_string cmd, Opt_array argv, Opt_array envp)
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
      ASTERIA_THROW_RUNTIME_ERROR("could not spawn process '$2'\n"
                    "[`posix_spawnp()` failed: $1]",
                    format_errno(errno), cmd);

    // Await its termination.
    // Note: `waitpid()` may return if the child has been stopped or continued.
    for(;;) {
      int wstat;
      if(::waitpid(pid, &wstat, 0) == -1)
        ASTERIA_THROW_RUNTIME_ERROR("error awaiting child process '$2'\n"
                      "[`waitpid()` failed: $1]",
                      format_errno(errno), pid);

      if(WIFEXITED(wstat))
        return WEXITSTATUS(wstat);

      if(WIFSIGNALED(wstat))
        return 128 + WTERMSIG(wstat);
    }
  }

void
std_system_proc_daemonize()
  {
    if(::daemon(1, 0) != 0)
      ASTERIA_THROW_RUNTIME_ERROR("could not daemonize process\n"
                    "[`daemon()` failed: $1]",
                    format_errno(errno));
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
    tstrm.reload(path, 1, cbuf);

    // Parse a sequence of key-value pairs.
    V_object root;
    while(auto qkey = do_accept_object_key_opt(tstrm))
      do_insert_unique(root, ::std::move(*qkey), do_conf_parse_value_nonrecursive(tstrm));

    // Ensure all data have been consumed.
    if(!tstrm.empty())
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_identifier_expected, tstrm.next_sloc());

    return root;
  }

void
create_bindings_system(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("gc_count_variables"),
      ASTERIA_BINDING_BEGIN("std.system.gc_count_variables", self, global, reader) {
        V_integer gen;

        reader.start_overload();
        reader.required(gen);      // generation
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_gc_count_variables, global, gen);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("gc_get_threshold"),
      ASTERIA_BINDING_BEGIN("std.system.gc_get_threshold", self, global, reader) {
        V_integer gen;

        reader.start_overload();
        reader.required(gen);      // generation
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_gc_get_threshold, global, gen);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("gc_set_threshold"),
      ASTERIA_BINDING_BEGIN("std.system.gc_set_threshold", self, global, reader) {
        V_integer gen;
        V_integer thrs;

        reader.start_overload();
        reader.required(gen);      // generation
        reader.required(thrs);     // threshold
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_gc_set_threshold, global, gen, thrs);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("gc_collect"),
      ASTERIA_BINDING_BEGIN("std.system.collect", self, global, reader) {
        Opt_integer glim;

        reader.start_overload();
        reader.optional(glim);     // [generation_limit]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_gc_collect, global, glim);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("env_get_variable"),
      ASTERIA_BINDING_BEGIN("std.system.env_get_variable", self, global, reader) {
        V_string name;

        reader.start_overload();
        reader.required(name);     // name
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_env_get_variable, name);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("env_get_variables"),
      ASTERIA_BINDING_BEGIN("std.system.env_get_variables", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_env_get_variables);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("uuid"),
      ASTERIA_BINDING_BEGIN("std.system.uuid", self, global, reader) {
        Opt_boolean lowc;

        reader.start_overload();
        reader.optional(lowc);     // [lowercase]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_uuid, global, lowc);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("proc_get_pid"),
      ASTERIA_BINDING_BEGIN("std.system.proc_get_pid", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_proc_get_pid);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("proc_get_ppid"),
      ASTERIA_BINDING_BEGIN("std.system.proc_get_ppid", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_proc_get_ppid);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("proc_get_uid"),
      ASTERIA_BINDING_BEGIN("std.system.proc_get_uid", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_proc_get_uid);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("proc_get_euid"),
      ASTERIA_BINDING_BEGIN("std.system.proc_get_euid", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_proc_get_euid);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("proc_invoke"),
      ASTERIA_BINDING_BEGIN("std.system.proc_invoke", self, global, reader) {
        V_string cmd;
        Opt_array argv;
        Opt_array envp;

        reader.start_overload();
        reader.required(cmd);      // cmd
        reader.optional(argv);     // [argv]
        reader.optional(envp);     // [envp]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_proc_invoke, cmd, argv, envp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("proc_daemonize"),
      ASTERIA_BINDING_BEGIN("std.system.proc_daemonize", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_proc_daemonize);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("conf_load_file"),
      ASTERIA_BINDING_BEGIN("std.system.conf_load_file", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_system_conf_load_file, path);
      }
      ASTERIA_BINDING_END);
  }

}  // namespace asteria
