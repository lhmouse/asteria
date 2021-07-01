// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../compiler/compiler_error.hpp"
#include "../simple_script.hpp"
#include "../value.hpp"
#include <stdio.h>  // fprintf(), fclearerr(), stdin, stderr

namespace asteria {

void
read_execute_print_single()
  try {
    // Prepare for the next snippet.
    repl_source.clear();
    repl_file.clear();
    repl_args.clear();

    char strbuf[64];
    bool escaped = false;
    ++repl_index;

    cow_string heredoc;
    heredoc.swap(repl_heredoc);
    if(!heredoc.empty())
      heredoc.insert(0, 1, '\n');

    // Prompt for the first line.
    long line = 0;
    int indent;
    repl_printf("#%lu:%lu%n> ", repl_index, ++line, &indent);

    for(;;) {
      // Read a character. Break upon read errors.
      int ch = ::fgetc(stdin);
      if(ch == EOF) {
        ::fputc('\n', stderr);
        break;
      }

      if(ch == '\n') {
        // REPL commands can't straddle multiple lines.
        if(heredoc.empty() && escaped && (repl_source[0] == ':')) {
          repl_printf("! dangling \\ at end of command\n");
          return;
        }

        // In line input mode, the current snippet is terminated by an
        // unescaped line feed.
        if(heredoc.empty() && !escaped)
          break;

        // In heredoc mode, it is terminated by a line consisting of the
        // user-defined terminator, which is not part of the snippet and
        // must be removed.
        if(!heredoc.empty() && repl_source.ends_with(heredoc)) {
          repl_source.pop_back(heredoc.size());
          break;
        }

        // The line feed should be preserved. It'll be appended later.
        // Prompt for the next consecutive line.
        repl_printf("%*lu> ", indent, ++line);
      }
      else if(heredoc.empty()) {
        // In line input mode, backslashes that precede line feeds are
        // deleted; those that do not precede line feeds are kept as is.
        if(escaped)
          repl_source.push_back('\\');

        escaped = (ch == '\\');
        if(escaped)
          continue;
      }

      // Append the character.
      repl_source.push_back(static_cast<char>(ch));
      escaped = false;
    }

    // Discard this snippet if Ctrl-C was received.
    if(get_and_clear_last_signal() != 0) {
      repl_printf("! interrupted (type `:exit` to quit)\n");
      ::clearerr(stdin);
      return;
    }

    // Discard this snippet if a read error occurred.
    if(::ferror(stdin))
      return;

    if(::feof(stdin) && repl_source.empty())
      return;

    // If user input was empty, don't do anything.
    bool is_cmd = heredoc.empty() && (repl_source[0] == ':');
    size_t pos = repl_source.find_first_not_of(is_cmd, sref(" \f\n\r\t\v"));
    if(pos == cow_string::npos)
      return;

    if(is_cmd) {
      // Process the command.
      // If the command fills something into `repl_source`, execute it.
      auto cmdline = repl_source.substr(pos);
      repl_source.clear();
      handle_repl_command(::std::move(cmdline));
      if(repl_source.empty())
        return;
    }

    // The snippet might be an expression or a statement list.
    // First, try parsing it as the former. We do this by complementing the
    // expression to a return statement. As that expression is supposed to be
    // start at 'line 1', the `return` statement should start at 'line 0'.
    try {
      cow_string compl_source;
      compl_source.reserve(repl_source.size() + 100);

      // Parentheses are required in case of embedded semicolons.
      compl_source += "return ->(\n";
      compl_source += repl_source;
      compl_source += "\n);";

      cow_string real_name = repl_file;
      if(real_name.empty())
        ::sprintf(strbuf, "expression #%lu", repl_index),
          real_name.append(strbuf);

      repl_script.reload_string(real_name, 0, compl_source);
      repl_file = ::std::move(real_name);
    }
    catch(Compiler_Error&) {
      // If the snippet is not a valid expression, try parsing it as a
      // statement.
      cow_string real_name = repl_file;
      if(real_name.empty())
        ::sprintf(strbuf, "snippet #%lu", repl_index),
          real_name.append(strbuf);

      repl_script.reload_string(real_name, repl_source);
      repl_file = ::std::move(real_name);
    }

    // Save the accepted snippet.
    repl_last_source.assign(repl_source.begin(), repl_source.end());
    repl_last_file.assign(repl_file.begin(), repl_file.end());

    // Execute the script.
    repl_printf("* running '%s'...\n", repl_file.c_str());
    auto ref = repl_script.execute(::std::move(repl_args));

    // Stringify the result.
    if(ref.is_void())
      return repl_printf("* result #%lu: void\n", repl_index);

    const auto& val = ref.dereference_readonly();
    ::rocket::tinyfmt_str fmt;
    val.dump(fmt);
    return repl_printf("* result #%lu: %s\n", repl_index, fmt.c_str());
  }
  catch(exception& stdex) {
    repl_printf("! error: %s\n", stdex.what());
  }

}  // namespace asteria
