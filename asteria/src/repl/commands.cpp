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
    manual()  // the long description for `help [cmd]`
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
      { return "display information about a command";  }

    const char*
    manual()
      const noexcept override
      { return
//       1         2         3         4         5         6         7      |
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
  help [COMMAND]

  When no COMMAND is specified, a list of all available commands with short
  descriptions is displayed.

  When COMMAND is specified, the full description for COMMAND is displayed.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1;
// 4567890123456789012345678901234567890123456789012345678901234567890123456|
//       1         2         3         4         5         6         7      |
      }

    void
    handle(cow_string&& args)
      const override
      {
        ::fprintf(stderr, "TODO args = %s\n", args.c_str());
      }
  };

const uptr<Command> s_commands[] =
  {
    ::rocket::make_unique<Command_help>(),
  };

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

    // Ignore this command
    ::fprintf(stderr, "! unknown command: %s\n", cmd.c_str());
  }

}  // namespace asteria
