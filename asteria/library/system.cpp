// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

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
#include <poll.h>  // ::poll()
#include <sys/wait.h>  // ::waitpid()
#include <sys/utsname.h>  // ::uname()
#include <sys/socket.h>  // ::socket()
#include <openssl/rand.h>
#include <time.h>  // ::clock_gettime()
extern "C" char** environ;
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
    phcow_string key;
    Source_Location key_sloc;
  };

using Xparse = ::rocket::variant<Xparse_array, Xparse_object>;

void
do_accept_object_key(Xparse_object& ctxo, Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      ASTERIA_THROW(("identifier or string expected at '$1'"), tstrm.next_sloc());

    if(qtok->is_identifier())
      ctxo.key = qtok->as_identifier();
    else if(qtok->is_string_literal())
      ctxo.key = qtok->as_string_literal();
    else
      ASTERIA_THROW(("identifier or string expected at '$1'"), tstrm.next_sloc());

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
      ASTERIA_THROW(("Value expected at '$1'"), tstrm.next_sloc());

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
        ASTERIA_THROW(("Value expected at '$1'"), tstrm.next_sloc());
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
        ASTERIA_THROW(("Value expected at '$1'"), tstrm.next_sloc());
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
      ASTERIA_THROW(("Value expected at '$1'"), tstrm.next_sloc());

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
              ASTERIA_THROW(("Duplicate key encountered at '$1'"), tstrm.next_sloc());

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
    uint64_t quads[2];
    ::RAND_bytes((unsigned char*) quads, sizeof(quads));

    auto do_char_from_nibble = [](uint64_t quad, unsigned index)
      {
        int value = (int) (quad >> index * 4) & 0x0F;
        int shift_to_digit = 0x30 - (((9 - value) & -0x2700) >> 8);
        return (char) (value + shift_to_digit);
      };

    // `xxxxxxxx-xxxx-4xxx-Vxxx-xxxxxxxxxxxx`
    //            1         2         3
    //  01234567 9012 4567 9012 4567 9012345
    V_string str;
    str.resize(36, '-');
    char* wptr = str.mut_data();

    wptr[ 0] = do_char_from_nibble(    quads[0],   0);
    wptr[ 1] = do_char_from_nibble(    quads[0],   1);
    wptr[ 2] = do_char_from_nibble(    quads[0],   2);
    wptr[ 3] = do_char_from_nibble(    quads[0],   3);
    wptr[ 4] = do_char_from_nibble(    quads[0],   4);
    wptr[ 5] = do_char_from_nibble(    quads[0],   5);
    wptr[ 6] = do_char_from_nibble(    quads[0],   6);
    wptr[ 7] = do_char_from_nibble(    quads[0],   7);
    wptr[ 9] = do_char_from_nibble(    quads[0],   8);
    wptr[10] = do_char_from_nibble(    quads[0],   9);
    wptr[11] = do_char_from_nibble(    quads[0],  10);
    wptr[12] = do_char_from_nibble(    quads[0],  11);
    wptr[14] =                    '4'                ;
    wptr[15] = do_char_from_nibble(    quads[0],  12);
    wptr[16] = do_char_from_nibble(    quads[0],  13);
    wptr[17] = do_char_from_nibble(    quads[0],  14);
    wptr[19] = do_char_from_nibble(8 | quads[1],   0);
    wptr[20] = do_char_from_nibble(    quads[1],   1);
    wptr[21] = do_char_from_nibble(    quads[1],   2);
    wptr[22] = do_char_from_nibble(    quads[1],   3);
    wptr[24] = do_char_from_nibble(    quads[1],   4);
    wptr[25] = do_char_from_nibble(    quads[1],   5);
    wptr[26] = do_char_from_nibble(    quads[1],   6);
    wptr[27] = do_char_from_nibble(    quads[1],   7);
    wptr[28] = do_char_from_nibble(    quads[1],   8);
    wptr[29] = do_char_from_nibble(    quads[1],   9);
    wptr[30] = do_char_from_nibble(    quads[1],  10);
    wptr[31] = do_char_from_nibble(    quads[1],  11);
    wptr[32] = do_char_from_nibble(    quads[1],  12);
    wptr[33] = do_char_from_nibble(    quads[1],  13);
    wptr[34] = do_char_from_nibble(    quads[1],  14);
    wptr[35] = do_char_from_nibble(    quads[1],  15);
    return str;
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
    // Append arguments and environment variables.
    cow_vector<const char*> cstrings;
    cstrings.reserve(16);
    cstrings.push_back(cmd.safe_c_str());
    if(argv)
      for(const auto& arg : *argv)
        cstrings.push_back(arg.as_string().safe_c_str());
    cstrings.push_back(nullptr);

    ptrdiff_t env_start = cstrings.ssize();
    if(envp)
      for(const auto& env : *envp)
        cstrings.push_back(env.as_string().safe_c_str());
    cstrings.push_back(nullptr);

    // Launch the program.
    ::pid_t pid;
    if(::posix_spawnp(&pid, cmd.c_str(), nullptr, nullptr,
                      const_cast<char**>(cstrings.data()),
                      const_cast<char**>(cstrings.data() + env_start)) != 0)
      ASTERIA_THROW((
          "Could not spawn process `$1` with $2",
          "[`posix_spawnp()` failed: ${errno:full}]"),
          cmd, argv);

    for(;;) {
      // Await its termination. `waitpid()` may return if the child has been
      // stopped or continued, so this has to be a loop.
      int wstat;
      if(::waitpid(pid, &wstat, 0) == -1)
        ASTERIA_THROW((
            "Error awaiting child process",
            "[`waitpid()` failed: ${errno:full}]"));

      if(WIFEXITED(wstat))
        return WEXITSTATUS(wstat);  // exited

      if(WIFSIGNALED(wstat))
        return 128 + WTERMSIG(wstat);  // killed by a signal
    }
  }

optV_string
std_system_pipe(V_string cmd, optV_array argv, optV_array envp, optV_string input)
  {
    // Initialize input and output actions for the new process.
    ::posix_spawn_file_actions_t st_actions;
    if(::posix_spawn_file_actions_init(&st_actions) != 0)
      ASTERIA_THROW((
          "Could not initialize file actions",
          "[`posix_spawn_file_actions_init()` failed: ${errno:full}]"));

    ::rocket::unique_ptr<::posix_spawn_file_actions_t,
                int (::posix_spawn_file_actions_t*)> actions(
                         &st_actions, ::posix_spawn_file_actions_destroy);

    ::rocket::unique_posix_fd out_r, out_w, in_r, in_w;
    int pipe_fds[2];

    if(::pipe(pipe_fds) != 0)
      ASTERIA_THROW((
          "Could not create output pipe",
          "[`pipe()` failed: ${errno:full}]"));

    out_r.reset(pipe_fds[0]);
    out_w.reset(pipe_fds[1]);

    if(::posix_spawn_file_actions_addclose(actions, out_r) != 0)
      ASTERIA_THROW((
          "Could not set output file action",
          "[`posix_spawn_file_actions_addclose()` failed: ${errno:full}]"));

    if(::posix_spawn_file_actions_adddup2(actions, out_w, STDOUT_FILENO) != 0)
      ASTERIA_THROW((
          "Could not set output file action",
          "[`posix_spawn_file_actions_adddup2()` failed: ${errno:full}]"));

    if(::pipe(pipe_fds) != 0)
      ASTERIA_THROW((
          "Could not create input pipe",
          "[`pipe()` failed: ${errno:full}]"));

    in_r.reset(pipe_fds[0]);
    in_w.reset(pipe_fds[1]);

    if(::posix_spawn_file_actions_adddup2(actions, in_r, STDIN_FILENO) != 0)
      ASTERIA_THROW((
          "Could not set input file action",
          "[`posix_spawn_file_actions_adddup2()` failed: ${errno:full}]"));

    if(::posix_spawn_file_actions_addclose(actions, in_w) != 0)
      ASTERIA_THROW((
          "Could not set input file action",
          "[`posix_spawn_file_actions_addclose()` failed: ${errno:full}]"));

    // Append arguments and environment variables.
    cow_vector<const char*> cstrings;
    cstrings.reserve(16);
    cstrings.push_back(cmd.safe_c_str());
    if(argv)
      for(const auto& arg : *argv)
        cstrings.push_back(arg.as_string().safe_c_str());
    cstrings.push_back(nullptr);

    ptrdiff_t env_start = cstrings.ssize();
    if(envp)
      for(const auto& env : *envp)
        cstrings.push_back(env.as_string().safe_c_str());
    cstrings.push_back(nullptr);

    // Launch the program.
    ::pid_t pid;
    if(::posix_spawnp(&pid, cmd.c_str(), actions, nullptr,
                      const_cast<char**>(cstrings.data()),
                      const_cast<char**>(cstrings.data() + env_start)) != 0)
      ASTERIA_THROW((
          "Could not spawn process `$1` with $2",
          "[`posix_spawnp()` failed: ${errno:full}]"),
          cmd, argv);

    out_w.reset();
    in_r.reset();

    if(!input || input->empty())
      in_w.reset();

    optV_string output = V_string();
    constexpr size_t out_batch = 65536;
    size_t in_written = 0;

    for(;;) {
      // Check whether there's something to do.
      ::pollfd fds[2];
      fds[0].fd = out_r;
      fds[0].events = POLLIN;
      fds[0].revents = 0;
      fds[1].fd = in_w;
      fds[1].events = POLLOUT;
      fds[1].revents = 0;
      if(::poll(fds, 2, 60000) < 0)
        ASTERIA_THROW((
            "Could not wait for data from process `$1` with $2",
            "[`poll()` failed: ${errno:full}]"),
            cmd, argv);

      if(fds[0].revents & POLLIN) {
        // Read standard output.
        output->append(out_batch, '/');
        ::ssize_t io_n = ::read(out_r, output->mut_data() + output->size() - out_batch, out_batch);
        if(io_n < 0)
          ASTERIA_THROW((
              "Could not receive output data from process `$1` with $2",
              "[`read()` failed: ${errno:full}]"),
              cmd, argv);

        output->pop_back(out_batch - static_cast<size_t>(io_n));
        if(io_n == 0)
          out_r.reset();
      }
      else if(fds[0].revents & (POLLHUP | POLLERR))
        out_r.reset();

      if(fds[1].revents & POLLOUT) {
        // Write standard input.
        ::ssize_t io_n = ::write(in_w, input->data() + in_written, input->size() - in_written);
        if(io_n < 0)
          ASTERIA_THROW((
              "Could not send input data to process `$1` with $2",
              "[`write()` failed: ${errno:full}]"),
              cmd, argv);

        in_written += static_cast<size_t>(io_n);
        if(in_written >= input->size())
          in_w.reset();
      }
      else if(fds[1].revents & (POLLHUP | POLLERR))
        in_w.reset();

      if(!out_r && !in_w)
        break;
    }

    for(;;) {
      // Await its termination. `waitpid()` may return if the child has been
      // stopped or continued, so this has to be a loop.
      int wstat;
      if(::waitpid(pid, &wstat, 0) == -1)
        ASTERIA_THROW((
            "Error awaiting child process",
            "[`waitpid()` failed: ${errno:full}]"));

      if(WIFEXITED(wstat) && (WEXITSTATUS(wstat) != 0))
        output.reset();

      if(WIFEXITED(wstat))
        return output;  // exited

      if(WIFSIGNALED(wstat))
        return nullopt;  // killed by a signal
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
        ASTERIA_THROW(("Duplicate key encountered at '$1'"), tstrm.next_sloc());

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
        "std.system.call", "cmd, [argv, [envp]]",
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

    result.insert_or_assign(&"pipe",
      ASTERIA_BINDING(
        "std.system.pipe", "cmd, [argv, [envp]], [input]",
        Argument_Reader&& reader)
      {
        V_string cmd;
        optV_array argv, envp;
        optV_string input;

        reader.start_overload();
        reader.required(cmd);
        reader.save_state(0);
        reader.optional(input);
        if(reader.end_overload())
          return (Value) std_system_pipe(cmd, nullopt, nullopt, input);

        reader.load_state(0);
        reader.optional(argv);
        reader.save_state(1);
        reader.optional(input);
        if(reader.end_overload())
          return (Value) std_system_pipe(cmd, argv, nullopt, input);

        reader.load_state(1);
        reader.optional(envp);
        reader.optional(input);
        if(reader.end_overload())
          return (Value) std_system_pipe(cmd, argv, envp, input);

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
