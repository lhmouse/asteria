// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "system.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/random_engine.hpp"
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

    auto y = [](uint64_t quad, unsigned index)
      {
        int value = (int) (quad >> index * 4) & 0x0F;
        int shift_to_digit = 0x30 - (((9 - value) & -0x2700) >> 8);
        return (char) (value + shift_to_digit);
      };

    // `xxxxxxxx-xxxx-4xxx-Vxxx-xxxxxxxxxxxx`
    //            1         2         3
    //  01234567 9012 4567 9012 4567 9012345
    char temp[36];
    char* wptr = temp;

    *(wptr ++) = y(quads[0],  0);
    *(wptr ++) = y(quads[0],  1);
    *(wptr ++) = y(quads[0],  2);
    *(wptr ++) = y(quads[0],  3);
    *(wptr ++) = y(quads[0],  4);
    *(wptr ++) = y(quads[0],  5);
    *(wptr ++) = y(quads[0],  6);
    *(wptr ++) = y(quads[0],  7);
    *(wptr ++) =   '-';
    *(wptr ++) = y(quads[0],  8);
    *(wptr ++) = y(quads[0],  9);
    *(wptr ++) = y(quads[0], 10);
    *(wptr ++) = y(quads[0], 11);
    *(wptr ++) =   '-';
    *(wptr ++) =   '4';
    *(wptr ++) = y(quads[0], 12);
    *(wptr ++) = y(quads[0], 13);
    *(wptr ++) = y(quads[0], 14);
    *(wptr ++) =   '-';
    *(wptr ++) = y(8 | quads[1], 0);
    *(wptr ++) = y(quads[1],  1);
    *(wptr ++) = y(quads[1],  2);
    *(wptr ++) = y(quads[1],  3);
    *(wptr ++) =   '-';
    *(wptr ++) = y(quads[1],  4);
    *(wptr ++) = y(quads[1],  5);
    *(wptr ++) = y(quads[1],  6);
    *(wptr ++) = y(quads[1],  7);
    *(wptr ++) = y(quads[1],  8);
    *(wptr ++) = y(quads[1],  9);
    *(wptr ++) = y(quads[1], 10);
    *(wptr ++) = y(quads[1], 11);
    *(wptr ++) = y(quads[1], 12);
    *(wptr ++) = y(quads[1], 13);
    *(wptr ++) = y(quads[1], 14);
    *(wptr ++) = y(quads[1], 15);

    return V_string(temp, wptr);
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
    struct timespec ts, rem;

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
    ::rocket::unique_ptr<::FILE, int (::FILE*)> fp(::fopen(path.safe_c_str(), "rb"),::fclose);
    if(!fp)
      ASTERIA_THROW((
          "Could not open configuration file '$1'",
          "[`fopen()` failed: ${errno:full}]"),
          path);

    // Define the internal state of the parser.
    int next_ln = 1, next_col = 1;
    int ch = -1;
    int tok_ln = 1, tok_col = 1;
    cow_string token;
    array<bool, 256> valid_digits;
    int exp_ch = 0;

    auto do_load_next = [&]
      {
        ch = getc_unlocked(fp);
        if(ch < 0)
          return;

        if((ch >= 0x80) && (ch <= 0xBF))
          ASTERIA_THROW(("Invalid UTF-8 byte at '$1:$2:$3'"), path, next_ln, next_col);
        else if(ROCKET_UNEXPECT(ch > 0x7F)) {
          // Parse a multibyte Unicode character.
          uint32_t u8len = ::rocket::lzcnt32((static_cast<uint32_t>(ch) << 24) ^ UINT32_MAX);
          ch &= (1 << (7 - u8len)) - 1;

          unsigned char tbytes[4];
          if(::fread(tbytes, 1, u8len - 1, fp) != u8len - 1)
            ASTERIA_THROW(("Incomplete UTF-8 sequence at '$1:$2:$3'"), path, next_ln, next_col);

          for(uint32_t k = 0;  k != u8len - 1;  ++k)
            if((tbytes[k] < 0x80) || (tbytes[k] > 0xBF))
              ASTERIA_THROW(("Invalid UTF-8 sequence at '$1:$2:$3'"), path, next_ln, next_col);
            else {
              ch <<= 6;
              ch |= tbytes[k] & 0x3F;
            }

          if((ch < 0x80)  // overlong
              || (ch < (1 << (u8len * 5 - 4)))  // overlong
              || ((ch >= 0xD800) && (ch <= 0xDFFF))  // surrogates
              || (ch > 0x10FFFF))
            ASTERIA_THROW(("Invalid Unicode character at '$1:$2:$3'"), path, next_ln, next_col);
        }

        if(ch == '\n') {
          next_ln ++;
          next_col = 1;
        }
        else if(ch == '\t')
          next_col += 8 - ((next_col - 1) & 7);
        else if(ch >= 0)
          next_col += ::rocket::max(0, ::wcwidth(static_cast<wchar_t>(ch)));
      };

    auto do_token = [&]
      {
        // Clear the current token and skip whitespace.
        token.clear();

        if(ch < 0) {
          tok_ln = next_ln;
          tok_col = next_col;
          do_load_next();
          if(ch < 0)
            return;

          // Skip the UTF-8 BOM, if any.
          if((tok_ln == 1) && (tok_col == 1) && (ch == 0xFEFF)) {
            tok_ln = next_ln;
            tok_col = next_col;
            do_load_next();
            if(ch < 0)
              return;
          }
        }

        while(::rocket::is_any_of(ch, { '/', ' ', '\t', '\r', '\n', '\v', '\f' })) {
          if(ch == '/') {
            do_load_next();
            switch(ch)
              {
              case '/':
                // line comment
                for(;;) {
                  do_load_next();
                  if(ch < 0)
                    return;
                  else if(ch == '\n')
                    break;
                }
                break;

              case '*':
                // block comment
                for(;;) {
                  do_load_next();
                  if(ch < 0)
                    ASTERIA_THROW(("Incomplete comment at '$1:$2:$3'"), path, tok_ln, tok_col);
                  else if(ch == '*') {
                    do_load_next();
                    if(ch < 0)
                      ASTERIA_THROW(("Incomplete comment at '$1:$2:$3'"), path, tok_ln, tok_col);
                    else if(ch == '/')
                      break;
                  }
                }
                break;

              default:
                ASTERIA_THROW(("Invalid character at '$1:$2:$3'"), path, tok_ln, tok_col);
              }
          }

          tok_ln = next_ln;
          tok_col = next_col;
          do_load_next();
          if(ch < 0)
            return;
        }

        switch(ch)
          {
          case '[':
          case ']':
          case '{':
          case '}':
          case ':':
          case '=':
          case ',':
          case ';':
            // Take each of these characters as a single token; do not attempt to get
            // the next character, as some of these tokens may terminate the input,
            // and the stream may be blocking but we can't really know whether there
            // are more data.
            token.push_back(static_cast<char>(ch));
            ch = -1;
            break;

          case '_':
          case '$':
          case 'A' ... 'Z':
          case 'a' ... 'z':
            // Take an identifier.
            do {
              token.push_back(static_cast<char>(ch));
              do_load_next();
            }
            while((ch == '_') || (ch == '$')  || ((ch >= 'A') && (ch <= 'Z'))
                  || ((ch >= 'a') && (ch <= 'z')) || ((ch >= '0') && (ch <= '9')));

            // An identifier has been accepted.
            ROCKET_ASSERT(token.size() != 0);
            break;

          case '+':
          case '-':
            // Take a floating-point number. `Infinity` and `NaN` are not supported.
            token.push_back(static_cast<char>(ch));
            do_load_next();

            if((ch < '0') || (ch > '9'))
              ASTERIA_THROW(("Invalid number at '$1:$2:$3'"), path, tok_ln, tok_col);

            // fallthrough
          case '0' ... '9':
            token.push_back(static_cast<char>(ch));
            do_load_next();
            if(ch == '`')
              do_load_next();

            valid_digits.fill(false);
            if((token.back() == '0') && ((ch | 0x20) == 'b')) {
              // binary
              token.push_back('b');
              do_load_next();

              for(uint32_t digit = '0';  digit <= '1';  ++ digit)
                valid_digits.mut(digit) = true;
              exp_ch = 'p';
            }
            else if((token.back() == '0') && ((ch | 0x20) == 'x')) {
              // hexadecimal
              token.push_back('x');
              do_load_next();

              for(uint32_t digit = '0';  digit <= '9';  ++ digit)
                valid_digits.mut(digit) = true;
              for(uint32_t digit = 'a';  digit <= 'f';  ++ digit)
                valid_digits.mut(digit) = true;
              for(uint32_t digit = 'A';  digit <= 'F';  ++ digit)
                valid_digits.mut(digit) = true;
              exp_ch = 'p';
            }
            else {
              // decimal
              for(uint32_t digit = '0';  digit <= '9';  ++ digit)
                valid_digits.mut(digit) = true;
              exp_ch = 'e';
            }

            while(valid_digits[static_cast<uint32_t>(ch)]) {
              // collect digits
              token.push_back(static_cast<char>(ch));
              do_load_next();
              if(ch == '`')
                do_load_next();
            }

            if(ch == '.') {
              token.push_back(static_cast<char>(ch));
              do_load_next();

              if(!valid_digits[static_cast<uint32_t>(ch)])
                ASTERIA_THROW(("Invalid number at '$1:$2:$3'"), path, tok_ln, tok_col);

              do {
                token.push_back(static_cast<char>(ch));
                do_load_next();
                if(ch == '`')
                  do_load_next();
              } while(valid_digits[static_cast<uint32_t>(ch)]);
            }

            if((ch | 0x20) == exp_ch) {
              token.push_back(static_cast<char>(ch));
              do_load_next();

              if((ch == '+') || (ch == '-')) {
                token.push_back(static_cast<char>(ch));
                do_load_next();
              }

              if((ch < '0') || (ch > '9'))
                ASTERIA_THROW(("Invalid number at '$1:$2:$3'"), path, tok_ln, tok_col);

              do {
                token.push_back(static_cast<char>(ch));
                do_load_next();
                if(ch == '`')
                  do_load_next();
              } while((ch >= '0') && (ch <= '9'));
            }

            // A number has been accepted.
            ROCKET_ASSERT(token.size() != 0);
            break;

          case '\'':
            // Take a single-quote string. When stored in `token`, it shall start
            // with a double-quote character, followed by the decoded string. No
            // terminating double-quote character is appended.
            token.push_back('\"');
            for(;;) {
              int ch_ln = next_ln;
              int ch_col = next_col;
              do_load_next();
              if(ch < 0)
                ASTERIA_THROW(("Incomplete string at '$1:$2:$3'"), path, tok_ln, tok_col);
              else if((ch <= 0x1F) || (ch == 0x7F))
                ASTERIA_THROW(("Control character not allowed at '$1:$2:$3'"), path, ch_ln, ch_col);
              else if(ch == '\'')
                break;

              // Escape sequences are not allowed.
              token.push_back(static_cast<char>(ch));
            }

            // Discard the terminating single-quote character.
            ROCKET_ASSERT(token.size() != 0);
            ch = -1;
            break;

          case '\"':
            // Take a double-quote string. When stored in `token`, it shall start
            // with a double-quote character, followed by the decoded string. No
            // terminating double-quote character is appended.
            token.push_back('\"');
            for(;;) {
              int ch_ln = next_ln;
              int ch_col = next_col;
              do_load_next();
              if(ch < 0)
                ASTERIA_THROW(("Incomplete string at '$1:$2:$3'"), path, tok_ln, tok_col);
              else if((ch <= 0x1F) || (ch == 0x7F))
                ASTERIA_THROW(("Control character not allowed at '$1:$2:$3'"), path, ch_ln, ch_col);
              else if(ch == '\"')
                break;

              if(ROCKET_UNEXPECT(ch == '\\')) {
                // Read an escape sequence.
                do_load_next();
                if(ch < 0)
                  ASTERIA_THROW(("Incomplete escape sequence at '$1:$2:$3'"), path, ch_ln, ch_col);

                switch(ch)
                  {
                  case '\\':
                  case '\"':
                  case '/':
                    break;

                  case 'b':
                    ch = '\b';
                    break;

                  case 'f':
                    ch = '\f';
                    break;

                  case 'n':
                    ch = '\n';
                    break;

                  case 'r':
                    ch = '\r';
                    break;

                  case 't':
                    ch = '\t';
                    break;

                  case 'u':
                    {
                      // Read the first UTF-16 code unit.
                      int utf_ch = 0;
                      for(uint32_t k = 0;  k != 4;  ++k) {
                        do_load_next();
                        if(ch < 0)
                          ASTERIA_THROW(("Incomplete escape sequence at '$1:$2:$3'"),
                                        path, ch_ln, ch_col);

                        utf_ch <<= 4;
                        if((ch >= '0') && (ch <= '9'))
                          utf_ch |= ch - '0';
                        else if((ch >= 'A') && (ch <= 'F'))
                          utf_ch |= ch - 'A' + 10;
                        else if((ch >= 'a') && (ch <= 'f'))
                          utf_ch |= ch - 'a' + 10;
                        else
                          ASTERIA_THROW(("Incomplete escape sequence at '$1:$2:$3'"),
                                        path, ch_ln, ch_col);
                      }

                      if((utf_ch >= 0xDC00) && (utf_ch <= 0xDFFF))
                        ASTERIA_THROW(("Dangling UTF-16 trailing surrogate at '$1:$2:$3'"),
                                      path, ch_ln, ch_col);

                      if((utf_ch >= 0xD800) && (utf_ch <= 0xDBFF)) {
                        // Look for the trailing surrogate, which must also be a
                        // UTF-16 escape sequence.
                        do_load_next();
                        if(ch != '\\')
                          ASTERIA_THROW(("Missing UTF-16 trailing surrogate at '$1:$2:$3'"),
                                        path, ch_ln, ch_col);

                        do_load_next();
                        if(ch != 'u')
                          ASTERIA_THROW(("Missing UTF-16 trailing surrogate at '$1:$2:$3'"),
                                        path, ch_ln, ch_col);

                        int ts_ch = 0;
                        for(uint32_t k = 0;  k != 4;  ++k) {
                          do_load_next();
                          if(ch < 0)
                            ASTERIA_THROW(("Incomplete escape sequence at '$1:$2:$3'"),
                                          path, ch_ln, ch_col);

                          ts_ch <<= 4;
                          if((ch >= '0') && (ch <= '9'))
                            ts_ch |= ch - '0';
                          else if((ch >= 'A') && (ch <= 'F'))
                            ts_ch |= ch - 'A' + 10;
                          else if((ch >= 'a') && (ch <= 'f'))
                            ts_ch |= ch - 'a' + 10;
                          else
                            ASTERIA_THROW(("Incomplete escape sequence at '$1:$2:$3'"),
                                          path, ch_ln, ch_col);
                        }

                        if((ts_ch < 0xDC00) || (ts_ch > 0xDFFF))
                          ASTERIA_THROW(("Invalid UTF-16 trailing surrogate at '$1:$2:$3'"),
                                        path, ch_ln, ch_col);

                        utf_ch = 0x10000 + ((utf_ch - 0xD800) << 10) + (ts_ch - 0xDC00);
                      }

                      ch = utf_ch;
                    }
                    break;

                  case 'U':
                    {
                      // Read a UTF-32 code unit.
                      int utf_ch = 0;
                      for(uint32_t k = 0;  k != 6;  ++k) {
                        do_load_next();
                        if(ch < 0)
                          ASTERIA_THROW(("Incomplete escape sequence at '$1:$2:$3'"),
                                        path, ch_ln, ch_col);

                        utf_ch <<= 4;
                        if((ch >= '0') && (ch <= '9'))
                          utf_ch |= ch - '0';
                        else if((ch >= 'A') && (ch <= 'F'))
                          utf_ch |= ch - 'A' + 10;
                        else if((ch >= 'a') && (ch <= 'f'))
                          utf_ch |= ch - 'a' + 10;
                        else
                          ASTERIA_THROW(("Incomplete escape sequence at '$1:$2:$3'"),
                                        path, ch_ln, ch_col);
                      }

                      if(utf_ch >= 0x10FFFF)
                        ASTERIA_THROW(("Invalid UTF-32 character at '$1:$2:$3'"),
                                      path, ch_ln, ch_col);

                      if((utf_ch >= 0xD800) && (utf_ch <= 0xDFFF))
                        ASTERIA_THROW(("Dangling UTF-16 surrogate at '$1:$2:$3'"),
                                      path, ch_ln, ch_col);

                      ch = utf_ch;
                    }
                    break;

                  default:
                    ASTERIA_THROW(("Invalid escape sequence at '$1:$2:$3'"), path, ch_ln, ch_col);
                  }
              }

              // Move the unescaped character into the token.
              if(ROCKET_EXPECT(ch <= 0x7F))
                token.push_back(static_cast<char>(ch));
              else {
                char mbs[4];
                char* eptr = mbs;
                utf8_encode(eptr, static_cast<char32_t>(ch));
                token.append(mbs, eptr);
              }
            }

            // Discard the terminating double-quote character.
            ROCKET_ASSERT(token.size() != 0);
            ch = -1;
            break;

          default:
            ASTERIA_THROW(("Invalid character at '$1:$2:$3'"), path, tok_ln, tok_col);
          }
      };

    // Break deep recursion with a handwritten stack.
    struct xFrame
      {
        int ln, col;
        V_array* psa;
        V_object* pso;
      };

    V_object root;
    V_array* psa = nullptr;
    V_object* pso = &root;
    Value* pstor = nullptr;
    cow_vector<xFrame> stack;
    ::rocket::ascii_numget numg;

    do_token();
    if(token.empty())
      return root;

  do_pack_value_loop_:
    if(stack.size() > 32)
      ASTERIA_THROW(("Nesting limit exceeded at '$1:$2:$3'"), path, tok_ln, tok_col);

    if(psa) {
      // in array
      pstor = &(psa->emplace_back());
    }
    else {
      // in object
      cow_string key;
      if(token[0] == '\"')
        key.assign(token, 1);
      else if((token[0] == '_') || (token[0] == '$') || ((token[0] >= 'A') && (token[0] <= 'Z'))
              || ((token[0] >= 'a') && (token[0] <= 'z')))
        key.assign(token, 0);
      else
        ASTERIA_THROW(("Key expected at '$1:$2:$3'"), path, tok_ln, tok_col);

      do_token();
      if((token[0] == ':') || (token[0] == '='))
        do_token();

      auto emr = pso->try_emplace(key);
      if(!emr.second)
        ASTERIA_THROW(("Duplicate key `$4` at '$1:$2:$3'"), path, tok_ln, tok_col, key);

      pstor = &(emr.first->second);
    }

    if(token[0] == '[') {
      // array
      do_token();
      if(token.empty())
        ASTERIA_THROW(("Array not terminated properly at '$1:$2:$3'"), path, tok_ln, tok_col);

      if(token[0] != ']') {
        // open
        auto& frm = stack.emplace_back();
        frm.ln = tok_ln;
        frm.col = tok_col;
        frm.psa = ::std::exchange(psa, &(pstor->open_array()));
        frm.pso = ::std::exchange(pso, nullptr);
        goto do_pack_value_loop_;
      }

      // empty
      pstor->open_array();
    }
    else if(token[0] == '{') {
      // object
      do_token();
      if(token.empty())
        ASTERIA_THROW(("Object not terminated properly at '$1:$2:$3'"), path, tok_ln, tok_col);

      if(token[0] != '}') {
        // open
        auto& frm = stack.emplace_back();
        frm.ln = tok_ln;
        frm.col = tok_col;
        frm.psa = ::std::exchange(psa, nullptr);
        frm.pso = ::std::exchange(pso, &(pstor->open_object()));
        goto do_pack_value_loop_;
      }

      // empty
      pstor->open_object();
    }
    else if((token[0] == '+') || (token[0] == '-') || ((token[0] >= '0') && (token[0] <= '9'))) {
      // number
      if(token.find('.') == cow_string::npos) {
        // integer
        size_t n = numg.parse_I(token.data(), token.size());
        ROCKET_ASSERT(n == token.size());
        numg.cast_I(pstor->open_integer(), INT64_MIN, INT64_MAX);
        if(numg.overflowed())
          ASTERIA_THROW(("Integer out of range at '$1:$2:$3'"), path, tok_ln, tok_col);
      }
      else {
        // real number
        size_t n = numg.parse_D(token.data(), token.size());
        ROCKET_ASSERT(n == token.size());
        numg.cast_D(pstor->open_real(), -DBL_MAX, DBL_MAX);
        if(numg.overflowed())
          ASTERIA_THROW(("Real number out of range at '$1:$2:$3'"), path, tok_ln, tok_col);
      }
    }
    else if(token[0] == '\"') {
      // string
      pstor->open_string().assign(token.data() + 1, token.size() - 1);
    }
    else if(token == "null")
      pstor->clear();
    else if(token == "true")
      pstor->open_boolean() = true;
    else if(token == "false")
      pstor->open_boolean() = false;
    else
      ASTERIA_THROW(("Invalid token at '$1:$2:$3'"), path, tok_ln, tok_col);

    do_token();
    if((token[0] == ',') || (token[0] == ';'))
      do_token();

    while(!token.empty()) {
      // still in array or object
      if(psa) {
        if(token[0] != ']')
          goto do_pack_value_loop_;
      }
      else {
        if(token[0] != '}')
          goto do_pack_value_loop_;
      }

      if(stack.empty())
        ASTERIA_THROW(("Dangling `$4` at '$1:$2:$3'"), path, tok_ln, tok_col, token);

      // close
      psa = stack.back().psa;
      pso = stack.back().pso;
      stack.pop_back();

      do_token();
      if((token[0] == ',') || (token[0] == ';'))
        do_token();
    }

    if(psa)
      ASTERIA_THROW(("Array not terminated properly at '$1:$2:$3'"),
                    path, stack.back().ln, stack.back().col);

    if(!stack.empty())
      ASTERIA_THROW(("Object not terminated properly at '$1:$2:$3'"),
                    path, stack.back().ln, stack.back().col);

    return root;
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
