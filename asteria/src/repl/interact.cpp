// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

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
    repl_script.set_options(repl_opts);
    repl_source.clear();
    repl_file.clear();
    repl_args.clear();

    cow_string heredoc;
    heredoc.swap(repl_heredoc);

    bool escape = false;
    ++repl_index;

    // Prompt for the first line.
    int ch;
    int indent;
    long line = 0;
    repl_printf("#%lu:%lu%n> ", repl_index, ++line, &indent);

    for(;;) {
      // Read a character. Break upon read errors.
      ch = ::fgetc(stdin);
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
          if(!repl_source.empty() && (repl_source.front() == ':'))
            break;
        }
        else {
          // In heredoc mode, the current snippet is terminated by a line
          // consisting of the user-defined terminator, which is not part of
          // the snippet and must be removed.
          if(repl_source.ends_with(heredoc)) {
            repl_source.erase(repl_source.size() - heredoc.size());
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
          repl_source.push_back('\\');

        if(ch == '\\') {
          escape = true;
          continue;
        }
      }

      // Append the character.
      repl_source.push_back(static_cast<char>(ch));
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

    if(::feof(stdin) && repl_source.empty())
      return;

    // If user input was empty, don't do anything.
    size_t pos = repl_source.find_first_not_of(" \f\n\r\t\v");
    if(pos == cow_string::npos)
      return;

    // Check for REPL commands.
    if(repl_source.front() == ':') {
      repl_source.erase(0, 1);

      // Remove leading spaces.
      pos = repl_source.find_first_not_of(" \t");
      repl_source.erase(0, pos);
      if(repl_source.empty())
        return;

      // Get the command name, which is terminated by a space.
      pos = repl_source.find_first_of(" \t");
      auto cmd = repl_source.substr(0, pos);

      // Erase separating space characterss, as well as trailing ones.
      pos = repl_source.find_first_not_of(pos, " \t");
      repl_source.erase(0, pos);
      pos = repl_source.find_last_not_of(" \t");
      auto args = repl_source.substr(0, pos + 1);

      // Process the command.
      // If the command fills something into `repl_source`, execute it.
      repl_source.clear();
      handle_repl_command(::std::move(cmd), ::std::move(args));
      if(repl_source.empty())
        return;
    }

    // Name the snippet.
    if(repl_file.empty()) {
      char name[128];
      pos = (uint32_t)::snprintf(name, sizeof(name), "snippet #%lu", repl_index);
      repl_file.assign(name, pos);
    }

    // The snippet might be an expression or a statement list.
    // First, try parsing it as the former. We do this by complementing the
    // expression to a return statement. As that expression is supposed to be
    // start at 'line 1', the `return` statement should start at 'line 0'.
    try {
      // Parentheses are required in case of embedded semicolons.
      repl_script.reload_string(repl_file, 0, "return ->(\n" + repl_source + "\n);");
    }
    catch(Parser_Error&) {
      // If the snippet is not a valid expression, try parsing it as a
      // statement.
      repl_script.reload_string(repl_file, repl_source);
    }

    // Save the snippet.
    repl_last_source.assign(repl_source.begin(), repl_source.end());
    repl_last_file.assign(repl_file.begin(), repl_file.end());

    // Execute the script.
    repl_printf("* running '%s'...\n", repl_file.c_str());
    auto ref = repl_script.execute(repl_global, ::std::move(repl_args));

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
