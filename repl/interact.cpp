// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../asteria/xprecompiled.hpp"
#include "fwd.hpp"
#include "../asteria/compiler/compiler_error.hpp"
#include "../asteria/compiler/token_stream.hpp"
#include "../asteria/compiler/statement_sequence.hpp"
#include "../asteria/simple_script.hpp"
#include "../asteria/value.hpp"
namespace asteria {

void
read_execute_print_single()
  {
    // Prepare for the next snippet.
    repl_index = (repl_index + 1) % 1000;
    repl_source.clear();
    repl_file.clear();
    repl_args.clear();
    cow_string heredoc = exchange(repl_heredoc);

    bool iscmd = false, more = false;
    long linenum = 1;
    cow_string linestr;
    size_t pos;

    // Prompt for the first line.
    libedit_set_prompt("#%.3lu:1> ", repl_index);

    while(libedit_gets(linestr)) {
      // Remove trailing new line characters, if any.
      more = linestr.ends_with("\n");
      linestr.pop_back(more);
      repl_source.append(linestr);

      if(!heredoc.empty()) {
        // In heredoc mode, a line matching the user-defined terminator ends the
        // current snippet, which is not part of the snippet.
        if(repl_source.ends_with(heredoc))
          break;
      }
      else {
        // Check for commands. A command is not allowed to straddle multiple lines.
        if(repl_source.empty())
          break;

        iscmd = repl_source.front() == repl_cmd_char;
        if(iscmd && (repl_source.back() == '\\'))
          return repl_printf("! dangling \\ at end of command");

        // Check for multi-line inputs. Backslashes that join lines are removed.
        if(repl_source.back() != '\\')
          break;

        repl_source.pop_back();
      }

      // Prompt for the next line.
      repl_source.push_back('\n');
      linenum ++;
      libedit_set_prompt("%6lu> ", linenum);

      // Auto-indent it.
      pos = linestr.find_not_of(" \t");
      linestr.erase(::rocket::min(pos, linestr.size()));
      libedit_puts(linestr);
    }

    // Discard this snippet if Ctrl-C was received.
    if(repl_signal.xchg(0) != 0) {
      libedit_reset();
      return repl_printf("\n! interrupted (type `:exit` to quit)");
    }

    // Remove the heredoc marker.
    if(!heredoc.empty()) {
      if(!repl_source.ends_with(heredoc))
        return repl_printf("\n! heredoc not terminated properly");

      repl_source.pop_back(heredoc.size());
    }

    // Remove leading and trailing blank lines.
    pos = repl_source.find_not_of('\n');
    repl_source.erase(0, pos);

    pos = repl_source.rfind_not_of('\n');
    repl_source.erase(pos + 1);

    // Exit if the end of user input has been reached.
    if(repl_source.empty() && !more)
      exit_printf(exit_success, "\n* have a nice day :)");

    if(iscmd) {
      // Skip space characters after the command initiator.
      // If user input was empty, don't do anything.
      pos = repl_source.find_not_of(1, " \f\n\r\t\v");
      if(pos == cow_string::npos)
        return;

      libedit_add_history(repl_source);

      try {
        // Process the command, which may re-populate `repl_source`.
        auto cmdline = repl_source.substr(pos);
        repl_source.clear();
        handle_repl_command(move(cmdline));
      }
      catch(exception& stdex) {
        return repl_printf("! exception: %s", stdex.what());
      }
    }

    // Skip space characters. If the source string becomes empty, do nothing.
    pos = repl_source.find_not_of(" \f\n\r\t\v");
    if(pos == cow_string::npos)
      return;

    // Add the snippet into history if it is from the user, i.e. if it is not
    // composited by a command.
    if(!iscmd)
      libedit_add_history(repl_source);

    // Tokenize source code.
    cow_string real_name;
    ::rocket::tinyfmt_str fmt;
    Reference ref;

    try {
      // Try parsing the snippet as an expression.
      real_name = repl_file;
      if(ROCKET_EXPECT(real_name.empty())) {
        char strbuf[64];
        ::sprintf(strbuf, "expression #%.3lu", repl_index);
        real_name.assign(strbuf);
      }

      fmt.set_string(repl_source);
      repl_script.reload_oneline(real_name, move(fmt));
      repl_file = move(real_name);
    }
    catch(Compiler_Error& except) {
      if(except.status() >= compiler_status_undeclared_identifier)
        return repl_printf("! exception: %s", except.what());

      try {
        // Try parsing it as a sequence of statements instead.
        real_name = repl_file;
        if(ROCKET_EXPECT(real_name.empty())) {
          char strbuf[64];
          ::sprintf(strbuf, "snippet #%.3lu", repl_index);
          real_name.assign(strbuf);
        }

        fmt.set_string(repl_source);
        repl_script.reload(real_name, 1, move(fmt));
        repl_file = move(real_name);
      }
      catch(Compiler_Error& again) {
        // It can't be parsed either way, so fail.
        return repl_printf("! exception: %s", again.what());
      }
    }

    // Save the accepted snippet.
    repl_last_source.assign(repl_source.begin(), repl_source.end());
    repl_last_file.assign(repl_file.begin(), repl_file.end());

    try {
      // Execute the script.
      repl_printf("* running '%s'...", repl_file.c_str());
      ref = repl_script.execute(move(repl_args));
    }
    catch(exception& stdex) {
      return repl_printf("! exception: %s", stdex.what());
    }

    // Print the result.
    if(ref.is_void())
      fmt.set_string(&"void");
    else {
      const auto& val = ref.dereference_readonly();
      fmt.clear_string();
      val.dump(fmt);
    }

    repl_printf("* result #%.3lu: %s", repl_index, fmt.c_str());
  }

}  // namespace asteria
