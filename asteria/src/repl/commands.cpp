// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../../rocket/tinybuf_file.hpp"

namespace asteria {
namespace {

void
do_lowercase_string(cow_string& str)
  {
    char ch;
    for(size_t k = 0;  k != str.size();  ++k)
      if((ch = ::rocket::ascii_to_lower(str[k])) != str[k])
        str.mut(k) = ch;
  }

struct Command
  {
    virtual
    ~Command();

    virtual const char*
    cmd()  // the name of this command
      const noexcept
      = 0;

    virtual const char*
    oneline()  // the one-line description for `help`
      const noexcept
      = 0;

    virtual const char*
    description()  // the long description for `help [cmd]`
      const noexcept
      = 0;

    virtual void
    handle(cow_string&& args)  // do something
      const
      = 0;
  };

Command::
~Command()
  {
  }

struct Command_exit final
  : public Command
  {
    const char*
    cmd() const noexcept override
      { return "exit";  }

    const char*
    oneline() const noexcept override
      { return "exit the interpreter";  }

    const char*
    description() const noexcept override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
* exit [CODE]

  Exit the interpreter. If CODE is absent, the process exits with zero. If
  CODE is specified, it shall be a decimal integer denoting the process exit
  status. If CODE is not a valid integer, the process exits anyway, with an
  unspecified non-zero status.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    [[noreturn]] void
    handle(cow_string&& args) const override
      {
        const char* bptr = args.data();
        const char* eptr = bptr + args.size();
        if(bptr == eptr)
          exit_printf(exit_success, "* have a nice day :)\n");

        uint8_t stat;
        ::rocket::ascii_numget numg;
        if(!numg.get(stat, bptr, eptr) || (bptr != eptr)) {
          repl_printf("! warning: invalid exit status: %s\n", args.c_str());
          stat = exit_non_integer;
        }
        exit_printf(static_cast<Exit_Status>(stat), "* exiting: %d\n", stat);
      }
  };

struct Command_help final
  : public Command
  {
    const char*
    cmd() const noexcept override
      { return "help";  }

    const char*
    oneline() const noexcept override
      { return "obtain information about a command";  }

    const char*
    description() const noexcept override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
* help [COMMAND]

  When COMMAND is specified, prints the full description of COMMAND. When no
  COMMAND is specified, prints a list of all available commands with short
  descriptions.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    void
    handle(cow_string&& args) const override;
  };

struct Command_heredoc final
  : public Command
  {
    const char*
    cmd() const noexcept override
      { return "heredoc";  }

    const char*
    oneline() const noexcept override
      { return "enter heredoc mode";  }

    const char*
    description() const noexcept override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
* heredoc DELIM

  Enter heredoc mode. This prevents the interpreter from submitting scripts
  on line breaks. Instead, a script is terminated by a line that matches
  DELIM, without any leading or trailing spaces.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    void
    handle(cow_string&& args) const override
      {
        if(args.empty())
          return repl_printf("! `heredoc` requires a terminator string\n");

        repl_heredoc = ::std::move(args);
        repl_printf("* the next snippet will be terminated by `%s`\n",
                    repl_heredoc.c_str());
      }
  };

struct Command_source final
  : public Command
  {
    const char*
    cmd() const noexcept override
      { return "source";  }

    const char*
    oneline() const noexcept override
      { return "load and execute a script file";  }

    const char*
    description() const noexcept override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
* source PATH

  Load and execute the file designated by PATH.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    void
    handle(cow_string&& args) const override
      {
        if(args.empty())
          return repl_printf("! `source` requires a file path\n");

        cow_string source;
        long line = 0;
        cow_string textln;

        ::rocket::tinybuf_file file;
        file.open(args.safe_c_str(), ::rocket::tinybuf::open_read);

        int indent;
        ::snprintf(nullptr, 0, "#%lu:%lu%n> ", repl_index, line, &indent);
        repl_printf("* loading file '%s'...\n", args.c_str());

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

        repl_printf("* finished loading file '%s'\n", args.c_str());

        repl_source = ::std::move(source);
        repl_file = ::std::move(args);
      }
  };

struct Command_again final
  : public Command
  {
    const char*
    cmd() const noexcept override
      { return "again";  }

    const char*
    oneline() const noexcept override
      { return "reload last snippet compiled successfully";  }

    const char*
    description() const noexcept override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
* again

  Reload and execute the last snippet that has been compiled successfully.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    void
    handle(cow_string&& args) const override
      {
        if(args.size())
          return repl_printf("! `again` takes no argument\n");

        if(repl_last_source.empty())
          return repl_printf("! no snippet has been compiled so far\n");

        repl_source = repl_last_source;
        repl_file = repl_last_file;
      }
  };

const uptr<const Command> s_commands[] =
  {
    // Please keep this list in lexicographical order.
    ::rocket::make_unique<Command_again>(),
    ::rocket::make_unique<Command_exit>(),
    ::rocket::make_unique<Command_help>(),
    ::rocket::make_unique<Command_heredoc>(),
    ::rocket::make_unique<Command_source>(),
  };

void
Command_help::
handle(cow_string&& args) const
  {
    if(!args.empty()) {
      // Search for the argument.
      do_lowercase_string(args);

      for(const auto& ptr : s_commands)
        if(ptr->cmd() == args)
          return repl_printf("%s", ptr->description());

      // Report the failure, followed by all commands for reference.
      repl_printf("! unknown command `%s`\n", args.c_str());
    }

    // Get the maximum length of commands.
    size_t max_len = 8;
    for(const auto& ptr : s_commands)
      max_len = ::rocket::max(max_len, ::std::strlen(ptr->cmd()));

    // List all commands.
    repl_printf("* list of commands:\n");
    for(const auto& ptr : s_commands)
      repl_printf("  %-*s  %s\n", static_cast<int>(max_len), ptr->cmd(),
                                  ptr->oneline());
  }

}  // namesapce

void
handle_repl_command(cow_string&& cmd, cow_string&& args)
  {
    // Find a command and execute it.
    do_lowercase_string(cmd);

    for(const auto& ptr : s_commands)
      if(ptr->cmd() == cmd)
        return ptr->handle(::std::move(args));

    // Ignore this command.
    repl_printf(
        "! unknown command `%s` (type `:help` for available commands)\n",
        cmd.c_str());
  }

}  // namespace asteria
