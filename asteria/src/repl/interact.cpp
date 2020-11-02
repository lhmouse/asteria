// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../runtime/global_context.hpp"
#include "../compiler/parser_error.hpp"
#include "../simple_script.hpp"
#include "../value.hpp"
#include <stdio.h>  // fprintf(), fclearerr(), stdin, stderr

namespace asteria {

void
read_execute_print_single()
  try {
    // Prepare for the next snippet.
    cow_string source;
    bool escape = false;

    cow_string heredoc = ::std::move(repl_heredoc);
    repl_heredoc.clear();
    ++repl_index;
    repl_script.set_options(repl_cmdline.opts);

    // Prompt for the first line.
    int indent;
    long line = 0;
    repl_printf("#%lu:%lu%n> ", repl_index, ++line, &indent);

    for(;;) {
      // Read a character. Break upon read errors.
      int ch = ::fgetc(stdin);
      if(ch == EOF) {
        ::fputc('\n', stderr);
        break;
      }

      // Check for termination.
      if(ch == '\n') {
        if(heredoc.empty()) {
          // In line input mode, the current snippet is terminated by an
          // unescaped line feed.
          if(!escape)
            break;

          // REPL commands can't straddle multiple lines.
          if(!source.empty() && (source.front() == ':'))
            break;
        }
        else {
          // In heredoc mode, the current snippet is terminated by a line
          // consisting of the user-defined terminator, which is not part of
          // the snippet and must be removed.
          if(source.ends_with(heredoc)) {
            source.erase(source.size() - heredoc.size());
            heredoc.clear();
            break;
          }
        }

        // The line feed should be preserved. It'll be appended later.
        // Prompt for the next consecutive line.
        repl_printf("%*lu> ", indent, ++line);
      }
      else if(heredoc.empty()) {
        // In line input mode, backslashes that precede line feeds are deleted.
        // Those that do not precede line feeds are kept as is.
        if(escape)
          source.push_back('\\');

        if(ch == '\\') {
          escape = true;
          continue;
        }
      }

      // Append the character.
      source.push_back(static_cast<char>(ch));
      escape = false;
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

    if(::feof(stdin) && source.empty())
      return;

    // If user input was empty, don't do anything.
    size_t pos = source.find_first_not_of(" \f\n\r\t\v");
    if(pos == cow_string::npos)
      return;

    // Check for REPL commands.
    if(source.front() == ':') {
      source.erase(0, 1);

      // Remove leading spaces.
      pos = source.find_first_not_of(" \t");
      source.erase(0, pos);
      if(source.empty())
        return;

      // Get the command name, which is terminated by a space.
      pos = source.find_first_of(" \t");
      cow_string cmd = source.substr(0, pos);

      // Erase separating space characterss, as well as trailing ones.
      pos = source.find_first_not_of(pos, " \t");
      source.erase(0, pos);
      pos = source.find_last_not_of(" \t");
      source.erase(pos + 1);

      // Process the command.
      return handle_repl_command(::std::move(cmd), ::std::move(source));
    }

    // Name the snippet.
    ::rocket::tinyfmt_str fmt;
    fmt << "snippet #" << repl_index;

    // The snippet might be an expression or a statement list.
    // First, try parsing it as the former. We do this by complementing the
    // expression to a return statement. As that expression is supposed to be
    // start at 'line 1', the `return` statement should start at 'line 0'.
    try {
      repl_script.reload_string(fmt.get_string(), 0,
                                "return ref\n" + source + "\n;");
    }
    catch(Parser_Error&) {
      // If the snippet is not a valid expression, try parsing it as a
      // statement.
      repl_script.reload_string(fmt.get_string(), source);
    }

    // Execute the script.
    auto ref = repl_script.execute(repl_global);

   // If the script exits without returning anything, say void.
    if(ref.is_void()) {
      repl_printf("* result #%lu: void\n", repl_index);
      return;
    }

    // Stringify the value.
    const auto& val = ref.dereference_readonly();
    fmt.clear_string();
    val.dump(fmt);
    repl_printf("* result #%lu: %s\n", repl_index, fmt.c_str());
  }
  catch(exception& stdex) {
    repl_printf("! error: %s\n", stdex.what());
  }

}  // namespace asteria
