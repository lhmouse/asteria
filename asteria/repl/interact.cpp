// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "fwd.hpp"
#include "../compiler/compiler_error.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../simple_script.hpp"
#include "../value.hpp"
namespace asteria {

void
read_execute_print_single()
  {
    // Prepare for the next snippet.
    repl_source.clear();
    repl_file.clear();
    repl_args.clear();

    cow_string heredoc;
    heredoc.swap(repl_heredoc);

    const char* linebuf;
    int linelen;
    size_t pos;

    // Prompt for the first line.
    bool iscmd = false;
    long linenum = 0;
    int indent;
    editline_set_prompt("#%lu:%lu%n> ", ++repl_index, ++linenum, &indent);

    while(!!(linebuf = editline_gets(&linelen))) {
      // Append this line.
      repl_source.append(linebuf, (unsigned)linelen);

      // `linebuf` has a trailing line feed if it is not the last line.
      // Remove it for simplicity.
      if(repl_source.ends_with("\n"))
        repl_source.pop_back();

      // In heredoc mode, a line matching the user-defined terminator ends
      // the current snippet, which is not part of the snippet.
      if(!heredoc.empty() && repl_source.ends_with(heredoc)) {
        repl_source.erase(repl_source.size() - heredoc.size());
        break;
      }

      if(heredoc.empty()) {
        iscmd = repl_source[0] == repl_cmd_char;
        if(!repl_source.ends_with("\\"))  // note `repl_source` may be empty
          break;

        // A command is not allowed to straddle multiple lines.
        if(iscmd)
          return repl_printf("! dangling \\ at end of command");

        // Backslashes that join lines are removed, unlike in heredoc mode.
        repl_source.pop_back();
      }

      // Prompt for the next line.
      repl_source.push_back('\n');
      editline_set_prompt("%*lu> ", indent, ++linenum);
    }

    if(!linebuf)
      repl_printf("");

    // Discard this snippet if Ctrl-C was received.
    if(repl_signal.xchg(0) != 0) {
      editline_reset();
      repl_printf("! interrupted (type `:exit` to quit)");
      return;
    }

    // Exit if an error occurred while reading user input.
    if(linelen == -1)
      exit_printf(exit_system_error, "! could not read standard input: %m");

    // Remove leading and trailing blank lines.
    pos = repl_source.find_first_not_of('\n');
    repl_source.erase(0, pos);

    pos = repl_source.find_last_not_of('\n');
    repl_source.erase(pos + 1);

    // Exit if the end of user input has been reached.
    if(!linebuf && repl_source.empty())
      exit_printf(exit_success, "* have a nice day :)");

    if(iscmd) {
      // Skip space characters after the command initiator.
      // If user input was empty, don't do anything.
      pos = repl_source.find_first_not_of(1, " \f\n\r\t\v");
      if(pos == cow_string::npos)
        return;

      editline_add_history(repl_source);

      try {
        // Process the command, which may re-populate `repl_source`.
        auto cmdline = repl_source.substr(pos);
        repl_source.clear();
        handle_repl_command(::std::move(cmdline));
      }
      catch(exception& stdex) {
        return repl_printf("! error: %s", stdex.what());  }
    }

    // Skip space characters. If the source string becomes empty, do nothing.
    pos = repl_source.find_first_not_of(" \f\n\r\t\v");
    if(pos == cow_string::npos)
      return;

    // Add the snippet into history if it is from the user, i.e. if it is
    // not composited by a command.
    if(!iscmd)
      editline_add_history(repl_source);

    // Tokenize source code.
    cow_string real_name;
    Token_Stream tstrm(repl_script.options());
    Statement_Sequence stmtq(repl_script.options());
    Reference ref;

    try {
      // The snippet may be a sequence of statements or an expression.
      // First, try parsing it as the former.
      real_name = repl_file;
      if(ROCKET_EXPECT(real_name.empty())) {
        char strbuf[64];
        ::sprintf(strbuf, "snippet #%lu", repl_index);
        real_name.assign(strbuf);
      }

      ::rocket::tinybuf_str cbuf(repl_source, tinybuf::open_read);
      tstrm.reload(real_name, 1, ::std::move(cbuf));
      stmtq.reload(::std::move(tstrm));

      repl_script.reload(real_name, ::std::move(stmtq));
      repl_file = ::std::move(real_name);
    }
    catch(Compiler_Error& except) {
      // Check whether the input looks like an expression.
      if(except.status() != compiler_status_semicolon_expected)
        return repl_printf("! error: %s", except.what());

      try {
        // Try parsing the snippet as an expression.
        real_name = repl_file;
        if(ROCKET_EXPECT(real_name.empty())) {
          char strbuf[64];
          ::sprintf(strbuf, "expression #%lu", repl_index);
          real_name.assign(strbuf);
        }

        ::rocket::tinybuf_str cbuf(repl_source, tinybuf::open_read);
        tstrm.reload(real_name, 1, ::std::move(cbuf));
        stmtq.reload_oneline(::std::move(tstrm));

        repl_script.reload(real_name, ::std::move(stmtq));
        repl_file = ::std::move(real_name);
      }
      catch(Compiler_Error& again) {
        return repl_printf("! error: %s", again.what());  }
    }

    // Save the accepted snippet.
    repl_last_source.assign(repl_source.begin(), repl_source.end());
    repl_last_file.assign(repl_file.begin(), repl_file.end());

    try {
      // Execute the script.
      repl_printf("* running '%s'...", repl_file.c_str());
      ref = repl_script.execute(::std::move(repl_args));
    }
    catch(exception& stdex) {
      return repl_printf("! error: %s", stdex.what());  }

    // Stringify the result.
    if(ref.is_void())
      return repl_printf("* result #%lu: void", repl_index);

    const auto& val = ref.dereference_readonly();
    ::rocket::tinyfmt_str fmt;
    val.dump(fmt);
    return repl_printf("* result #%lu: %s", repl_index, fmt.c_str());
  }

}  // namespace asteria
