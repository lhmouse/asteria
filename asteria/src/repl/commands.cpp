// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace asteria {
namespace {

struct Command
  {
    virtual
    ~Command();

    virtual
    const char*
    cmd()  // the name of this command
      const noexcept
      = 0;

    virtual
    const char*
    oneline()  // the one-line description for `help`
      const noexcept
      = 0;

    virtual
    const char*
    description()  // the long description for `help [cmd]`
      const noexcept
      = 0;

    virtual
    void
    handle(cow_string&& args)  // do something
      const
      = 0;
  };

Command::
~Command()
  {
  }

struct Command_exit
  final
  : public Command
  {
    const char*
    cmd()
      const noexcept override
      { return "exit";  }

    const char*
    oneline()
      const noexcept override
      { return "exits the interpreter";  }

    const char*
    description()
      const noexcept override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
* exit [CODE]

  Exits the interpreter. If CODE is specified, it shall be a decimal integer
  which is used as the process exit status. If CODE is not a valid integer,
  the process exits with an unspecified non-zero status. If CODE is absent,
  the process exits with zero.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    [[noreturn]]
    void
    handle(cow_string&& args)
      const override
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

struct Command_help
  final
  : public Command
  {
    const char*
    cmd()
      const noexcept override
      { return "help";  }

    const char*
    oneline()
      const noexcept override
      { return "obtains information about a command";  }

    const char*
    description()
      const noexcept override
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
    handle(cow_string&& args)
      const override;
  };

const uptr<const Command> s_commands[] =
  {
    // Please keep this list in lexicographical order.
    ::rocket::make_unique<Command_exit>(),
    ::rocket::make_unique<Command_help>(),
  };

void
Command_help::
handle(cow_string&& args)
  const
  {
    if(!args.empty()) {
      // Convert all letters in `args` into lowercase.
      ::std::for_each(args.mut_begin(), args.mut_end(),
          [&](char& ch) { ch = ::rocket::ascii_to_lower(ch);  });

      // Search for the argument.
      for(const auto& ptr : s_commands)
        if(ptr->cmd() == args)
          return (void)::fputs(ptr->description(), stderr);

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
      repl_printf("  :%-*s  %s\n", static_cast<int>(max_len), ptr->cmd(),
                                  ptr->oneline());
  }

}  // namesapce

void
handle_repl_command(cow_string&& cmd, cow_string&& args)
  {
    // Convert all letters in `cmd` into lowercase.
    ::std::for_each(cmd.mut_begin(), cmd.mut_end(),
        [&](char& ch) { ch = ::rocket::ascii_to_lower(ch);  });

    // Find a command and execute it.
    for(const auto& ptr : s_commands)
      if(ptr->cmd() == cmd)
        return ptr->handle(::std::move(args));

    // Ignore this command.
    repl_printf(
        "! unknown command `%s` (type `:help` for available commands)\n",
        cmd.c_str());
  }

}  // namespace asteria
