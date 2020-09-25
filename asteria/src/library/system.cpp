// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "system.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/genius_collector.hpp"
#include "../runtime/random_engine.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/parser_error.hpp"
#include "../compiler/enums.hpp"
#include "../utilities.hpp"
#include <spawn.h>  // ::posix_spawnp()
#include <sys/wait.h>  // ::waitpid()
#include <unistd.h>  // ::daemon()
#include <time.h>  // ::clock_gettime()

namespace asteria {
namespace {

constexpr int64_t xgcgen_newest = static_cast<int64_t>(gc_generation_newest);
constexpr int64_t xgcgen_oldest = static_cast<int64_t>(gc_generation_oldest);
constexpr char s_hex_digits[] = "00112233445566778899AaBbCcDdEeFf";

inline
bool
do_check_punctuator(const Token* qtok, initializer_list<Punctuator> accept)
  {
    return qtok && qtok->is_punctuator() && ::rocket::is_any_of(qtok->as_punctuator(), accept);
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
      throw Parser_Error(parser_status_equals_sign_or_colon_expected, tstrm.next_sloc(), tstrm.next_length());

    tstrm.shift();
    return ::std::move(key);
  }

Value&
do_insert_unique(V_object& obj, Key_with_sloc&& key, Value&& value)
  {
    auto pair = obj.try_emplace(::std::move(key.name), ::std::move(value));
    if(!pair.second)
      throw Parser_Error(parser_status_duplicate_key_in_object, key.sloc, key.length);
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
        throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(), tstrm.next_length());

      switch(weaken_enum(qtok->index())) {
        case Token::index_punctuator: {
          // Accept a `+`, `-`, `[` or `{`.
          auto punct = qtok->as_punctuator();
          switch(weaken_enum(punct)) {
            case punctuator_add:
            case punctuator_sub: {
              cow_string name;
              qtok = tstrm.peek_opt(1);
              if(qtok && qtok->is_identifier())
                name = qtok->as_identifier();

              // Only `infinity` and `nan` may follow.
              // Note that the tokenizer will have merged sign symbols into adjacent number literals.
              if(::rocket::is_none_of(name, { "infinity", "nan" }))
                throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(),
                                   tstrm.next_length());

              // Accept a special numeric value.
              double sign = (punct == punctuator_add) - 1;
              double real = (name[0] == 'i') ? ::std::numeric_limits<double>::infinity()
                                             : ::std::numeric_limits<double>::quiet_NaN();

              value = ::std::copysign(real, sign);
              tstrm.shift(2);
              break;
            }

            case punctuator_bracket_op: {
              tstrm.shift();

              // Open an array.
              qtok = tstrm.peek_opt();
              if(!qtok) {
                throw Parser_Error(parser_status_closed_bracket_or_comma_expected, tstrm.next_sloc(),
                                   tstrm.next_length());
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
                throw Parser_Error(parser_status_closed_brace_or_comma_expected, tstrm.next_sloc(),
                                   tstrm.next_length());
              }
              else if(!do_check_punctuator(qtok, { punctuator_brace_cl })) {
                // Get the first key.
                auto qkey = do_accept_object_key_opt(tstrm);
                if(!qkey)
                  throw Parser_Error(parser_status_closed_brace_or_json5_key_expected, tstrm.next_sloc(),
                                     tstrm.next_length());

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
              throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(), tstrm.next_length());
          }
          break;
        }

        case Token::index_identifier: {
          // Accept a literal.
          const auto& name = qtok->as_identifier();
          if(::rocket::is_none_of(name, { "null", "true", "false", "infinity", "nan" }))
            throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(), tstrm.next_length());

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
          throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(), tstrm.next_length());
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
            throw Parser_Error(parser_status_closed_bracket_or_comma_expected, tstrm.next_sloc(),
                               tstrm.next_length());
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
            throw Parser_Error(parser_status_closed_brace_or_comma_expected, tstrm.next_sloc(),
                               tstrm.next_length());
          }
          else if(!do_check_punctuator(qtok, { punctuator_brace_cl })) {
            // Get the next key.
            auto qkey = do_accept_object_key_opt(tstrm);
            if(!qkey)
              throw Parser_Error(parser_status_closed_brace_or_json5_key_expected, tstrm.next_sloc(),
                                 tstrm.next_length());

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

Opt_integer
std_system_gc_count_variables(Global_Context& global, V_integer generation)
  {
    auto gc_gen = static_cast<GC_Generation>(::rocket::clamp(generation, xgcgen_newest, xgcgen_oldest));
    if(gc_gen != generation)
      return nullopt;

    // Get the current number of variables being tracked.
    auto gcoll = global.genius_collector();
    size_t count = gcoll->get_collector(gc_gen).count_tracked_variables();
    return static_cast<int64_t>(count);
  }

Opt_integer
std_system_gc_get_threshold(Global_Context& global, V_integer generation)
  {
    auto gc_gen = static_cast<GC_Generation>(::rocket::clamp(generation, xgcgen_newest, xgcgen_oldest));
    if(gc_gen != generation)
      return nullopt;

    // Get the current threshold.
    auto gcoll = global.genius_collector();
    uint32_t thres = gcoll->get_collector(gc_gen).get_threshold();
    return static_cast<int64_t>(thres);
  }

Opt_integer
std_system_gc_set_threshold(Global_Context& global, V_integer generation, V_integer threshold)
  {
    auto gc_gen = static_cast<GC_Generation>(::rocket::clamp(generation, xgcgen_newest, xgcgen_oldest));
    if(gc_gen != generation)
      return nullopt;

    // Set the threshold and return its old value.
    auto gcoll = global.genius_collector();
    uint32_t thres = gcoll->get_collector(gc_gen).get_threshold();
    gcoll->open_collector(gc_gen).set_threshold(static_cast<uint32_t>(::rocket::clamp(threshold, 0, INT32_MAX)));
    return static_cast<int64_t>(thres);
  }

V_integer
std_system_gc_collect(Global_Context& global, Opt_integer generation_limit)
  {
    auto gc_limit = gc_generation_oldest;

    // Unlike others, this function does not fail if `generation_limit` is out of range.
    if(generation_limit)
      gc_limit = static_cast<GC_Generation>(::rocket::clamp(*generation_limit, xgcgen_newest, xgcgen_oldest));

    // Perform garbage collection up to the generation specified.
    auto gcoll = global.genius_collector();
    size_t nvars = gcoll->collect_variables(gc_limit);
    return static_cast<int64_t>(nvars);
  }

Opt_string
std_system_env_get_variable(V_string name)
  {
    const char* val = ::getenv(name.safe_c_str());
    if(!val)
      return nullopt;
    return cow_string(val);
  }

V_object
std_system_env_get_variables()
  {
    V_object vars;
    const char* const* vpos = ::environ;
    while(const char* str = *(vpos++)) {
      // The key is terminated by an equals sign.
      const char* equ = ::std::strchr(str, '=');
      if(ROCKET_UNEXPECT(!equ))
        vars.insert_or_assign(cow_string(str), ::rocket::sref(""));  // no equals sign?
      else
        vars.insert_or_assign(cow_string(str, equ), cow_string(equ + 1));
    }
    return vars;
  }

V_string
std_system_uuid(Global_Context& global, Opt_boolean lowercase)
  {
    // Canonical form: `xxxxxxxx-xxxx-Myyy-Nzzz-wwwwwwwwwwww`
    //  * x: number of 1/10,000 seconds since UNIX Epoch
    //  * M: always `4` (UUID version)
    //  * y: process ID
    //  * N: any of `0`-`7` (UUID variant)
    //  * z: context ID
    //  * w: random bytes
    bool rlowerc = lowercase.value_or(false);
    const auto prng = global.random_engine();
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t x = static_cast<uint64_t>(ts.tv_sec) * 30518 + static_cast<uint64_t>(ts.tv_nsec) / 32768;
    uint64_t y = static_cast<uint32_t>(::getpid());
    uint64_t z = reinterpret_cast<uintptr_t>(::std::addressof(global)) >> 12;
    uint64_t w = static_cast<uint64_t>(prng->bump()) << 32 | static_cast<uint64_t>(prng->bump());

    // Set version and variant.
    y &= 0x0FFF;
    y |= 0x4000;
    z &= 0x7FFF;

    // Compose the string backwards.
    char sbuf[36];
    size_t bp = 36;

    auto put_field_bkwd = [&](uint64_t& value, size_t n)
      {
        for(size_t k = 0;  k != n;  ++k)
          sbuf[--bp] = s_hex_digits[((value << 1) & 0x1E) + rlowerc],
          value >>= 4;
      };

    put_field_bkwd(w, 12);
    sbuf[--bp] = '-';
    put_field_bkwd(z,  4);
    sbuf[--bp] = '-';
    put_field_bkwd(y,  4);
    sbuf[--bp] = '-';
    put_field_bkwd(x,  4);
    sbuf[--bp] = '-';
    put_field_bkwd(x,  8);

    // Return the string. Note there is no null terminator in `sbuf`.
    return cow_string(sbuf, sizeof(sbuf));
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
    if(argv)
      ::rocket::for_each(*argv, [&](const Value& arg) { ptrs.emplace_back(arg.as_string().safe_c_str());  });

    auto eoff = ptrs.ssize();  // beginning of environment variables
    ptrs.emplace_back(nullptr);

    // Append environment variables.
    if(envp)
      eoff = ptrs.ssize(),
      ::rocket::for_each(*envp, [&](const Value& env) { ptrs.emplace_back(env.as_string().safe_c_str());  }),
      ptrs.emplace_back(nullptr);

    // Launch the program.
    ::pid_t pid;
    if(::posix_spawnp(&pid, cmd.c_str(), nullptr, nullptr, const_cast<char**>(ptrs.data()),
                                                           const_cast<char**>(ptrs.data() + eoff)) != 0)
      ASTERIA_THROW("Could not spawn process '$2'\n"
                    "[`posix_spawnp()` failed: $1]",
                    format_errno(errno), cmd);

    // Await its termination.
    for(;;) {
      // Note: `waitpid()` may return if the child has been stopped or continued.
      int wstat;
      if(::waitpid(pid, &wstat, 0) == -1)
        ASTERIA_THROW("Error awaiting child process '$2'\n"
                      "[`waitpid()` failed: $1]",
                      format_errno(errno), pid);

      // Check whether the process has terminated normally.
      if(WIFEXITED(wstat))
        return WEXITSTATUS(wstat);

      // Check whether the process has been terminated by a signal.
      if(WIFSIGNALED(wstat))
        return 128 + WTERMSIG(wstat);
    }
  }

void
std_system_proc_daemonize()
  {
    if(::daemon(1, 0) != 0)
      ASTERIA_THROW("Could not daemonize process\n"
                    "[`daemon()` failed: $1]",
                    format_errno(errno));
  }

V_object
std_system_conf_load_file(V_string path)
  {
    // Open the file denoted by `path` in text mode.
    ::rocket::unique_posix_file fp(::fopen(path.safe_c_str(), "r"), ::fclose);
    if(!fp)
      ASTERIA_THROW("Could not open configuration file '$2'\n"
                     "[`fopen()` failed: $1]",
                     format_errno(errno), path);

    // Parse characters from the file.
    ::setbuf(fp, nullptr);
    ::rocket::tinybuf_file cbuf(::std::move(fp));

    // Initialize tokenizer options.
    // Unlike JSON5, we support _real_ integers and single-quote string literals.
    Compiler_Options opts;
    opts.keywords_as_identifiers = true;

    Token_Stream tstrm(opts);
    tstrm.reload(path, 1, cbuf);

    // Parse a sequence of key-value pairs.
    V_object root;
    while(auto qkey = do_accept_object_key_opt(tstrm))
      do_insert_unique(root, ::std::move(*qkey), do_conf_parse_value_nonrecursive(tstrm));

    // Ensure all data have been consumed.
    if(!tstrm.empty())
      throw Parser_Error(parser_status_identifier_expected, tstrm.next_sloc(), tstrm.next_length());
    return root;
  }

void
create_bindings_system(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.system.gc_count_variables()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_count_variables"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_count_variables(generation)`

  * Gets the number of variables that are being tracked by the
    collector for `generation`. Valid values for `generation` are
    `0`, `1` and `2`.

  * Returns the number of variables being tracked. This value is
    only informative. If `generation` is not valid, `null` is
    returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.gc_count_variables"), ::rocket::cref(args));
    // Parse arguments.
    V_integer generation;
    if(reader.I().v(generation).F()) {
      Reference_root::S_temporary xref = { std_system_gc_count_variables(global, ::std::move(generation)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.gc_get_threshold()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_get_threshold"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_get_threshold(generation)`

  * Gets the threshold of the collector for `generation`. Valid
    values for `generation` are `0`, `1` and `2`.

  * Returns the threshold. If `generation` is not valid, `null` is
    returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.gc_get_threshold"), ::rocket::cref(args));
    // Parse arguments.
    V_integer generation;
    if(reader.I().v(generation).F()) {
      Reference_root::S_temporary xref = { std_system_gc_get_threshold(global, ::std::move(generation)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.gc_set_threshold()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_set_threshold"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_set_threshold(generation, threshold)`

  * Sets the threshold of the collector for `generation` to
    `threshold`. Valid values for `generation` are `0`, `1` and
    `2`. Valid values for `threshold` range from `0` to an
    unspecified positive integer; overlarge values are capped
    silently without failure. A larger `threshold` makes garbage
    collection run less often but slower. Setting `threshold` to
    `0` ensures all unreachable variables be collected immediately.

  * Returns the threshold before the call. If `generation` is not
    valid, `null` is returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.gc_set_threshold"), ::rocket::cref(args));
    // Parse arguments.
    V_integer generation;
    V_integer threshold;
    if(reader.I().v(generation).v(threshold).F()) {
      Reference_root::S_temporary xref = { std_system_gc_set_threshold(global, ::std::move(generation),
                                                                       ::std::move(threshold)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.gc_collect()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gc_collect"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.gc_collect([generation_limit])`

  * Performs garbage collection on all generations including and
    up to `generation_limit`. If it is absent, all generations are
    collected.

  * Returns the number of variables that have been collected in
    total.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.gc_collect"), ::rocket::cref(args));
    // Parse arguments.
    Opt_integer generation_limit;
    if(reader.I().o(generation_limit).F()) {
      Reference_root::S_temporary xref = { std_system_gc_collect(global, ::std::move(generation_limit)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.env_get_variable()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("env_get_variable"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.env_get_variable(name)`

  * Retrieves an environment variable with `name`.

  * Returns the environment variable's value if a match is found,
    or `null` if no such variable exists.

  * Throws an exception if `name` is not valid.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.env_get_variable"), ::rocket::cref(args));
    // Parse arguments.
    V_string name;
    if(reader.I().v(name).F()) {
      Reference_root::S_temporary xref = { std_system_env_get_variable(::std::move(name)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.env_get_variables()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("env_get_variables"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.env_get_variables()`

  * Retrieves all environment variables.

  * Returns an object of strings which consists of copies of all
    environment variables.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.env_get_variables"), ::rocket::cref(args));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_system_env_get_variables() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.uuid()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("uuid"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  * Generates a UUID according to the following specification:

    Canonical form: `xxxxxxxx-xxxx-Myyy-Nzzz-wwwwwwwwwwww`

     * x: number of 1/30,518 seconds since UNIX Epoch
     * M: always `4` (UUID version)
     * y: process ID
     * N: any of `0`-`7` (UUID variant)
     * z: context ID
     * w: random bytes

     Unlike version-1 UUIDs in RFC 4122, the timestamp is written
     in pure big-endian order. This ensures the case-insensitive
     lexicographical order of such UUIDs will match their order of
     creation. If `lowercase` is set to `true`, hexadecimal digits
     above `9` are encoded as `abcdef`; otherwise they are encoded
     as `ABCDEF`.

  * Returns a UUID as a string of 36 characters without braces.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.uuid"), ::rocket::cref(args));
    // Parse arguments.
    Opt_boolean lowercase;
    if(reader.I().o(lowercase).F()) {
      Reference_root::S_temporary xref = { std_system_uuid(global, lowercase) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.proc_get_pid()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("proc_get_pid"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  * Gets the ID of the current process.

  * Returns the process ID as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.proc_get_pid"), ::rocket::cref(args));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_system_proc_get_pid() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.proc_get_ppid()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("proc_get_ppid"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  * Gets the ID of the parent process.

  * Returns the parent process ID as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.proc_get_ppid"), ::rocket::cref(args));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_system_proc_get_ppid() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.proc_get_uid()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("proc_get_uid"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  * Gets the real user ID of the current process.

  * Returns the real user ID as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.proc_get_uid"), ::rocket::cref(args));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_system_proc_get_uid() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.proc_get_euid()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("proc_get_euid"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  * Gets the effective user ID of the current process.

  * Returns the effective user ID as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.proc_get_euid"), ::rocket::cref(args));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_system_proc_get_euid() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.proc_invoke()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("proc_invoke"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.proc_invoke(cmd, [argv], [envp])`

  * Launches the program denoted by `cmd`, awaits its termination,
    and returns its exit status. If `argv` is provided, it shall be
    an array of strings, which specify additional arguments to pass
    to the program along with `cmd`. If `envp` is specified, it
    shall also be an array of strings, which specify environment
    variables to pass to the program.

  * Returns the exit status as an integer. If the process exits due
    to a signal, the exit status is `128+N` where `N` is the signal
    number.

  * Throws an exception if the program could not be launched or its
    exit status could not be retrieved.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.proc_invoke"), ::rocket::cref(args));
    // Parse arguments.
    V_string cmd;
    Opt_array argv;
    Opt_array envp;
    if(reader.I().v(cmd).o(argv).o(envp).F()) {
      Reference_root::S_temporary xref = { std_system_proc_invoke(::std::move(cmd), ::std::move(argv),
                                                                  ::std::move(envp)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.proc_daemonize()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("proc_daemonize"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.proc_daemonize()`

  * Detaches the current process from its controlling terminal and
    continues in the background. The calling process terminates on
    success so this function never returns.

  * Throws an exception on failure.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.proc_daemonize"), ::rocket::cref(args));
    // Parse arguments.
    if(reader.I().F()) {
      std_system_proc_daemonize();
      return self = Reference_root::S_void();
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.system.conf_load_file()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("conf_load_file"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.system.conf_load_file(path)`

  * Loads the configuration file denoted by `path`. Its syntax is
    similar to JSON5, except that commas, semicolons and top-level
    braces are omitted for simplicity, and single-quoted strings do
    not support escapes. A sample configuration file can be found
    at 'doc/sample.conf'.

  * Returns an object of all values from the file, if it was parsed
    successfully.

  * Throws an exception if the file could not be opened, or there
    was an error in it.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.system.conf_load_file"), ::rocket::cref(args));
    // Parse arguments.
    V_string path;
    if(reader.I().v(path).F()) {
      Reference_root::S_temporary xref = { std_system_conf_load_file(::std::move(path)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
  }

}  // namespace asteria
