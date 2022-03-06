// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../compiler/compiler_error.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../simple_script.hpp"
#include "../value.hpp"
#include <stdio.h>
#include <histedit.h>

namespace asteria {

void
read_execute_print_single()
  {
    static ::HistEvent el_event[1];
    static ::History* el_history;
    static ::EditLine* el_editor;
    static char el_prompt[64];

    // Initialize the Editline library if it hasn't been initialized.
    if(ROCKET_UNEXPECT(!el_editor)) {
      el_history = ::history_init();
      if(!el_history)
        exit_printf(exit_system_error, "! could not initialize history: %m");

      el_editor = ::el_init(repl_tar_name, stdin, stdout, stderr);
      if(!el_editor)
        exit_printf(exit_system_error, "! could not initialize editline: %m");

#ifndef EL_SAFEREAD
      // Replace the default GETCFN.
      // The default one ignores the first EINTR error and is confusing.
      ::el_set(el_editor, EL_GETCFN,
        +[](::EditLine*, wchar_t* out) {
          ::FILE* const fp = stdin;

          ::flockfile(fp);
          ::clearerr_unlocked(fp);
          ::wint_t wch = ::fgetwc_unlocked(fp);
          int succ = wch != WEOF;
          int err = !succ && ::ferror_unlocked(fp);
          ::funlockfile(fp);

          *out = (wchar_t)wch;
          return succ | -err;
        });
#endif  // EL_SAFEREAD

      // Initialize the editor. Errors are ignored.
      ::el_set(el_editor, EL_TERMINAL, nullptr);
      ::el_set(el_editor, EL_SIGNAL, 1);
      ::el_set(el_editor, EL_EDITOR, "emacs");
      ::el_set(el_editor, EL_PROMPT, +[]{ return el_prompt;  });

      ::history(el_history, el_event, H_SETSIZE, repl_history_size);
      ::history(el_history, el_event, H_SETUNIQUE, 1);
      ::el_set(el_editor, EL_HIST, ::history, el_history);

      // Load `~/.editrc`. Errors are ignored.
      repl_printf("* loading editline settings from `~/.editrc`...");
      if(::el_source(el_editor, nullptr) == 0)
        repl_printf("* ... done");
    }

    // Prepare for the next snippet.
    repl_source.clear();
    repl_file.clear();
    repl_args.clear();

    cow_string heredoc;
    heredoc.swap(repl_heredoc);

    // Prompt for the first line.
    bool iscmd = false;
    long linenum = 0;
    int indent;
    ::sprintf(el_prompt, "#%lu:%lu%n> ", ++repl_index, ++linenum, &indent);

    const char* linebuf;
    int linelen;
    size_t pos;

    ::fputc('\n', stderr);

    while(!!(linebuf = ::el_gets(el_editor, &linelen))) {
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
      ::sprintf(el_prompt, "%*lu> ", indent, ++linenum);
    }

    if(!linebuf)
      ::fputc('\n', stderr);

    // Discard this snippet if Ctrl-C was received.
    if(get_and_clear_last_signal() != 0) {
      ::el_reset(el_editor);
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

      // Add the snippet into history.
      ::history(el_history, el_event, H_ENTER, repl_source.c_str());

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

    // Add the snippet into history, unless it was composed by a command.
    if(!iscmd)
      ::history(el_history, el_event, H_ENTER, repl_source.c_str());

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
