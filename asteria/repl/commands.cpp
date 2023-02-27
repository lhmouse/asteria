// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "fwd.hpp"
#include "../utils.hpp"
#include "../../rocket/tinybuf_file.hpp"
namespace asteria {
namespace {

struct Handler
  {
    virtual
    ~Handler() = default;

    virtual
    const char*
    cmd() const = 0;  // the name of this command

    virtual
    const char*
    oneline() const = 0;  // the one-line description for `help`

    virtual
    const char*
    help() const = 0;  // the long description for `help [cmd]`

    virtual
    void
    handle(cow_vector<cow_string>&& args) = 0;  // do something
  };

cow_vector<unique_ptr<Handler>> s_handlers;

Handler*
do_find_handler_opt(cow_string&& cmd)
  {
    char ch;
    for(size_t k = 0;  k != cmd.size();  ++k)
      if((ch = ::rocket::ascii_to_lower(cmd[k])) != cmd[k])
        cmd.mut(k) = ch;

    for(const auto& up : s_handlers)
      if(up->cmd() == cmd)
        return up.get();

    return nullptr;
  }

template<typename HandlerT>
void
do_add_handler()
  {
    s_handlers.emplace_back(::rocket::make_unique<HandlerT>());
  }

struct Handler_exit final
  : Handler
  {
    const char*
    cmd() const override
      { return "exit";  }

    const char*
    oneline() const override
      { return "exit the interpreter";  }

    const char*
    help() const override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  exit [CODE]

  Exit the interpreter. If CODE is absent, the process exits with zero. If
  CODE is specified, it shall be a decimal integer denoting the process exit
  status. If CODE is not a valid integer, the process exits anyway, with an
  unspecified non-zero status.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+3;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    [[noreturn]]
    void
    handle(cow_vector<cow_string>&& args)
      {
        if(args.empty())
          exit_printf(exit_success, "* have a nice day :)");

        // Parse the status code.
        Exit_Status stat = exit_non_integer;
        const auto& s = args.at(0);

        ::rocket::ascii_numget numg;
        if(numg.parse_U(s.data(), s.size()) == s.size()) {
          // Get exit status from the argument.
          uint64_t temp;
          numg.cast_U(temp, 0, UINT8_MAX);
          stat = (Exit_Status) temp;
        }
        else
          repl_printf("! warning: invalid exit status: %s", s.c_str());

        if(args.size() > 1)
          repl_printf("! warning: excess arguments ignored");

        // Exit the interpreter now.
        exit_printf(stat, "* exiting: %d", stat);
      }
  };

struct Handler_help final
  : Handler
  {
    const char*
    cmd() const override
      { return "help";  }

    const char*
    oneline() const override
      { return "obtain information about a command";  }

    const char*
    help() const override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  help [COMMAND]

  When COMMAND is specified, prints the full description of COMMAND. When no
  COMMAND is specified, prints a list of all available commands with short
  descriptions.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+3;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    void
    list_all_commands() const
      {
        int max_len = 8;
        for(const auto& ptr : s_handlers)
          max_len = ::rocket::max(max_len, (int)::strlen(ptr->cmd()));

        repl_printf("* list of commands:");
        for(const auto& ptr : s_handlers)
          repl_printf("  %-*s  %s", max_len, ptr->cmd(), ptr->oneline());
      }

    void
    help_command(cow_string&& cmd) const
      {
        auto qhand = do_find_handler_opt(::std::move(cmd));
        if(!qhand)
          repl_printf("! unknown command `%s`", cmd.c_str());
        else
          repl_printf("* %s", qhand->help());
      }

    void
    handle(cow_vector<cow_string>&& args) override
      {
        if(args.empty())
          return this->list_all_commands();

        for(size_t k = 0;  k != args.size();  ++k)
          this->help_command(::std::move(args.mut(k)));
      }
  };

struct Handler_heredoc final
  : Handler
  {
    const char*
    cmd() const override
      { return "heredoc";  }

    const char*
    oneline() const override
      { return "enter heredoc mode";  }

    const char*
    help() const override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  heredoc DELIM

  Enter heredoc mode. This prevents the interpreter from submitting scripts
  on line breaks. Instead, a script is terminated by a line that matches
  DELIM, without any leading or trailing spaces.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+3;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    void
    handle(cow_vector<cow_string>&& args) override
      {
        if(args.size() != 1)
          return repl_printf("! exactly one terminator string expected");

        repl_heredoc = ::std::move(args.mut(0));
        repl_printf("* the next snippet will be terminated by `%s`",
                    repl_heredoc.c_str());
      }
  };

struct Handler_source final
  : Handler
  {
    const char*
    cmd() const override
      { return "source";  }

    const char*
    oneline() const override
      { return "load and execute a script file";  }

    const char*
    help() const override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  source PATH [ARGUMENTS...]

  Load and execute the file designated by PATH. ARGUMENTS are passed to the
  script as strings, which can be accessed via `__varg()`.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+3;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    void
    handle(cow_vector<cow_string>&& args) override
      {
        if(args.empty())
          return repl_printf("! please specify a source file to load");

        if(args[0].starts_with("~/")) {
          // Replace the tilde prefix with the HOME directory.
          // If the HOME directory cannot be determined, delete the tilde
          // so it becomes an absolute path, like in BASH.
          // I am too lazy to add support for `~+` or `~-` here.
          const char* home = ::secure_getenv("HOME");
          if(!home)
            args.mut(0).erase(0, 1);
          else
            args.mut(0).replace(0, 1, home);
        }

        ::rocket::unique_ptr<char, void (void*)> abspath(::free);
        abspath.reset(::realpath(args[0].safe_c_str(), nullptr));
        if(!abspath)
          ASTERIA_THROW((
              "Could not open script file '$2'",
              "[`realpath()` failed: $1]"),
              format_errno(), args[0]);

        repl_printf("* loading file '%s'...", abspath.get());
        ::rocket::tinybuf_file file;
        file.open(abspath, ::rocket::tinybuf::open_read);
        repl_printf("  ---+");

        cow_string source, textln;
        bool noeol = false;
        long linenum = 0;

        for(;;) {
          int ch = file.getc();
          if(ch == EOF) {
            // Check for errors.
            if(repl_signal.xchg(0) != 0) {
              repl_printf("\n! operation cancelled");
              return;
            }

            // Check for EOF.
            if(textln.empty())
              break;

            // Accept the last line without a line feed.
            ROCKET_ASSERT(textln.back() != '\n');
            noeol = true;
          }
          else if(ch != '\n') {
            // Ensure we don't mistake a binary file.
            int cct = (ch == 0xFF) ? (int)cmask_cntrl : get_cmask((char)ch);
            cct &= cmask_cntrl | cmask_blank;
            if(cct == cmask_cntrl) {
              repl_printf("! this file doesn't seem to contain text; giving up");
              return;
            }

            // Accept a single character.
            textln.push_back((char)ch);
            continue;
          }

          // Accept a line.
          repl_printf("  %3lu| %s", ++linenum, textln.c_str());
          source.append(textln);
          source.push_back('\n');
          textln.clear();
        }

        if(noeol)
          repl_printf("! warning: no line feed at end of file");

        // Set the script to execute.
        repl_printf("  ---+");
        repl_source.swap(source);
        repl_file.assign(abspath.get());
        repl_args.assign(args.move_begin() + 1, args.move_end());

        repl_printf("* finished loading file '%s'", abspath.get());
      }
  };

struct Handler_again final
  : Handler
  {
    const char*
    cmd() const override
      { return "again";  }

    const char*
    oneline() const override
      { return "reload last snippet compiled successfully";  }

    const char*
    help() const override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  again [ARGUMENTS...]

  Reload and execute the last snippet that has been compiled successfully.
  ARGUMENTS are passed to the script as strings, which can be accessed via
  `__varg()`.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+3;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    void
    handle(cow_vector<cow_string>&& args) override
      {
        if(repl_last_source.empty())
          return repl_printf("! no previous snippet");

        // Set the script to execute.
        repl_source = repl_last_source;
        repl_file = repl_last_file;
        repl_args.assign(args.move_begin(), args.move_end());
      }
  };

}  // namesapce

void
prepare_repl_commands()
  {
    if(s_handlers.size())
      return;

    // Create command interfaces. Note the list of commands is printed
    // according to this vector, so please ensure elements are sorted
    // lexicographically.
    do_add_handler<Handler_again>();
    do_add_handler<Handler_exit>();
    do_add_handler<Handler_help>();
    do_add_handler<Handler_heredoc>();
    do_add_handler<Handler_source>();
  }

void
handle_repl_command(cow_string&& cmdline)
  {
    // Tokenize the command line.
    cow_string cmd;
    cow_vector<cow_string> args;

    char quote = 0;
    bool has_token = false;
    size_t pos = 0;

    for(;;) {
      char ch;

      if(quote) {
        if(pos >= cmdline.size())
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                "Unmatched %c", quote);

        ch = cmdline[pos++];

        // Check for the end of this quoted argument.
        if(ch == quote) {
          quote = 0;
          continue;
        }
      }
      else {
        // Blank characters outside quotes terminate arguments.
        size_t next = cmdline.find_first_not_of(pos, sref(" \f\n\r\t\v"));
        if((next != pos) && has_token) {
          args.emplace_back(::std::move(cmd));
          cmd.clear();
          has_token = false;
        }

        pos = next;
        if(pos == cow_string::npos)
          break;

        ch = cmdline[pos++];
        has_token = true;

        // Check for quoted arguments.
        if(::rocket::is_any_of(ch, { '\'', '\"' })) {
          quote = ch;
          continue;
        }
      }

      // Escape sequences are allowed except in single quotes.
      if((quote != '\'') && (ch == '\\')) {
        if(pos >= cmdline.size())
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                "Dangling \\ at end of command");

        ch = cmdline[pos++];
      }

      cmd += ch;
    }

    ROCKET_ASSERT(!has_token);

    if(args.empty())
      return;

    // Pop the command and execute it.
    cmd = ::std::move(args.mut(0));
    args.erase(0, 1);

    auto qhand = do_find_handler_opt(::std::move(cmd));
    if(!qhand)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
          "Unknown command `%s` (type `:help` for available commands)",
          cmd.c_str());

    qhand->handle(::std::move(args));
  }

}  // namespace asteria
