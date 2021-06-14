// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../../rocket/tinybuf_file.hpp"

namespace asteria {
namespace {

struct Handler
  {
    virtual
    ~Handler();

    virtual const char*
    cmd() const  // the name of this command
      = 0;

    virtual const char*
    oneline() const  // the one-line description for `help`
      = 0;

    virtual const char*
    help() const  // the long description for `help [cmd]`
      = 0;

    virtual void
    handle(cow_vector<cow_string>&& args)  // do something
      = 0;
  };

Handler::
~Handler()
  {
  }

cow_vector<uptr<Handler>> s_handlers;

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
  : public Handler
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

    [[noreturn]] void
    handle(cow_vector<cow_string>&& args)
      {
        if(args.empty())
          exit_printf(exit_success, "* have a nice day :)\n");

        // Parse the status code.
        Exit_Status stat = exit_non_integer;

        ::rocket::ascii_numget numg;
        uint8_t num;
        const char* bptr = args[0].data();
        const char* eptr = bptr + args[0].size();

        if(numg.get(num, bptr, eptr) && (bptr == eptr))
          stat = static_cast<Exit_Status>(num);
        else
          repl_printf("! warning: invalid exit status: %s\n", args[0].c_str());

        if(args.size() > 1)
          repl_printf("! warning: excess arguments ignored\n");

        // Exit the interpreter now.
        exit_printf(stat, "* exiting: %d\n", stat);
      }
  };

struct Handler_help final
  : public Handler
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

        repl_printf("* list of commands:\n");
        for(const auto& ptr : s_handlers)
          repl_printf("  %-*s  %s\n", max_len, ptr->cmd(), ptr->oneline());
      }

    void
    help_command(cow_string&& cmd) const
      {
        auto qhand = do_find_handler_opt(::std::move(cmd));
        if(!qhand)
          repl_printf("! unknown command `%s`\n", cmd.c_str());
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
  : public Handler
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
          return repl_printf("! exactly one terminator string expected\n");

        repl_heredoc = ::std::move(args.mut(0));
        repl_printf("* the next snippet will be terminated by `%s`\n",
                    repl_heredoc.c_str());
      }
  };

struct Handler_source final
  : public Handler
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
          return repl_printf("! file path expected\n");

        cow_string source;
        long line = 0;
        cow_string textln;

        ::rocket::tinybuf_file file;
        file.open(args[0].safe_c_str(), ::rocket::tinybuf::open_read);

        int indent;
        ::snprintf(nullptr, 0, "#%lu:%lu%n> ", repl_index, line, &indent);
        repl_printf("* loading file '%s'...\n", args[0].c_str());

        int ch;
        bool noeol = false;
        repl_printf("  ----------\n");

        do {
          ch = file.getc();
          if(get_and_clear_last_signal()) {
            ::fputc('\n', stderr);
            repl_printf("! operation cancelled\n");
            return;
          }
          if(ch == EOF) {
            noeol = textln.size() && (textln.back() != '\n');
            if(noeol)
              textln.push_back('\n');
          }
          else
            textln.push_back(static_cast<char>(ch));

          if(textln.size() && (textln.back() == '\n')) {
            source.append(textln);
            repl_printf("%*lu> %s", indent, ++line, textln.c_str());
            textln.clear();
          }
        }
        while(ch != EOF);

        repl_printf("  ----------\n");
        if(noeol)
          repl_printf("! warning: missing new line at end of file\n");

        repl_printf("* finished loading file '%s'\n", args[0].c_str());

        // Set the script to execute.
        repl_source = ::std::move(source);
        repl_file = ::std::move(args.mut(0));
        repl_args.assign(args.move_begin() + 1, args.move_end());
      }
  };

struct Handler_again final
  : public Handler
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
          return repl_printf("! no snippet has been compiled so far\n");

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
                "unmatched %c", quote);

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
                "dangling \\ at end of command");

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
          "unknown command `%s` (type `:help` for available commands)",
          cmd.c_str());

    qhand->handle(::std::move(args));
  }

}  // namespace asteria
