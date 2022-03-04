// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../compiler/compiler_error.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../simple_script.hpp"
#include "../value.hpp"
#include <stdio.h>  // fprintf(), fclearerr(), stdin, stderr

namespace asteria {

void
read_execute_print_single()
  {
    // Prepare for the next snippet.
    repl_index ++;
    long line = 0;
    cow_string heredoc = ::std::move(repl_heredoc);

    repl_source.clear();
    repl_file.clear();
    repl_args.clear();
    repl_heredoc.clear();

    // Prompt for the first line.
    char strbuf[64];
    bool escaped, is_cmd;
    int indent;

    escaped = false;
    repl_printf("#%lu:%lu%n> ", repl_index, ++line, &indent);

    for(;;) {
      // Read a character. Break upon read errors.
      is_cmd = heredoc.empty() && (repl_source[0] == repl_cmd_char);
      int ch = ::fgetc(stdin);
      if(ch == EOF) {
        ::fputc('\n', stderr);
        break;
      }

      if(ch == '\n') {
        // REPL commands can't straddle multiple lines.
        if(escaped && is_cmd)
          return repl_printf("! dangling \\ at end of command\n");

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
    size_t pos = repl_source.find_first_not_of(is_cmd, " \f\n\r\t\v");
    if(pos == cow_string::npos)
      return;

    if(is_cmd) {
      // Process the command.
      // If the command fills something into `repl_source`, execute it.
      auto cmdline = repl_source.substr(pos);
      repl_source.clear();

      try {
        handle_repl_command(::std::move(cmdline));
      }
      catch(exception& stdex) {
        return repl_printf("! error: %s\n", stdex.what());  }

      if(repl_source.empty())
        return;
    }

    // Tokenize source code.
    cow_string real_name;
    Token_Stream tstrm(repl_script.options());
    Statement_Sequence stmtq(repl_script.options());

    try {
      // The snippet may be a sequence of statements or an expression.
      // First, try parsing it as the former.
      real_name = repl_file;
      if(ROCKET_EXPECT(real_name.empty()))
        real_name.assign(strbuf,
              (unsigned) ::sprintf(strbuf, "snippet #%lu", repl_index));

      ::rocket::tinybuf_str cbuf(repl_source, tinybuf::open_read);
      tstrm.reload(real_name, 1, ::std::move(cbuf));
      stmtq.reload(::std::move(tstrm));
      repl_script.reload(real_name, ::std::move(stmtq));
      repl_file = ::std::move(real_name);
    }
    catch(Compiler_Error& except) {
      // Check whether the input looks like an expression.
      if(except.status() != compiler_status_semicolon_expected)
        return repl_printf("! error: %s\n", except.what());

      // Try parsing the snippet as an expression.
      real_name = repl_file;
      if(ROCKET_EXPECT(real_name.empty()))
        real_name.assign(strbuf,
              (unsigned) ::sprintf(strbuf, "expression #%lu", repl_index));

      try {
        ::rocket::tinybuf_str cbuf(repl_source, tinybuf::open_read);
        tstrm.reload(real_name, 1, ::std::move(cbuf));
        stmtq.reload_oneline(::std::move(tstrm));
        repl_script.reload(real_name, ::std::move(stmtq));
        repl_file = ::std::move(real_name);
      }
      catch(Compiler_Error& again) {
        return repl_printf("! error: %s\n", again.what());  }
    }

    // Save the accepted snippet.
    repl_last_source.assign(repl_source.begin(), repl_source.end());
    repl_last_file.assign(repl_file.begin(), repl_file.end());

    // Execute the script.
    Reference ref;
    try {
      repl_printf("* running '%s'...\n", repl_file.c_str());
      ref = repl_script.execute(::std::move(repl_args));
    }
    catch(exception& stdex) {
      return repl_printf("! error: %s\n", stdex.what());  }

    // Stringify the result.
    if(ref.is_void())
      return repl_printf("* result #%lu: void\n", repl_index);

    const auto& val = ref.dereference_readonly();
    ::rocket::tinyfmt_str fmt;
    val.dump(fmt);
    return repl_printf("* result #%lu: %s\n", repl_index, fmt.c_str());
  }

}  // namespace asteria
