// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../asteria/xprecompiled.hpp"
#include "fwd.hpp"
#include "../rocket/once_flag.hpp"
#include <stdarg.h>
#include <signal.h>
#include <histedit.h>
namespace asteria {
namespace {

::rocket::once_flag s_once;
::History* s_history;
::EditLine* s_editor;
char s_prompt[64];

int
do_getcfn(::EditLine* el, wchar_t* out)
  {
    // Report EOF if no stream has been associated.
    ::FILE* fp;
    if(::el_get(el, EL_GETFP, 0, &fp) != 0)
      return 0;

    ::flockfile(fp);
    const unique_ptr<::FILE, void (::FILE*)> lock(fp, ::funlockfile);

  r:
    repl_signal.store(0);
    ::clearerr(fp);
    ::wint_t wch = ::fgetwc(fp);
    *out = (wchar_t) wch;

    if(wch != WEOF)
      return 1;  // success

    if(::feof(fp))
      return 0;  // EOF

    if(errno != EINTR)
      return -1;  // failure

    // Check for recoverable errors, which are signals (except `SIGSTOP`)
    // that are ignored by default.
    switch(repl_signal.load()) {
      case SIGSTOP:
      case SIGURG:
      case SIGCHLD:
        goto r;  // ignore these

      case SIGCONT:
        el_set(el, EL_REFRESH);
        goto r;

      case SIGWINCH:
        el_resize(el);
        goto r;
    }

    return -1;  // failure
  }

void
do_init_once()
  {
    repl_printf("* initializing editline...");

    ROCKET_ASSERT(!s_history);
    s_history = ::history_init();
    if(!s_history)
      exit_printf(exit_system_error, "! could not initialize history: %m");

    ROCKET_ASSERT(!s_editor);
    s_editor = ::el_init("asteria", stdin, stdout, stderr);
    if(!s_editor)
      exit_printf(exit_system_error, "! could not initialize editline: %m");

    // Replace the default GETCFN.
    // The default one used to ignore the first EINTR error and is confusing.
    // Eventually we have decided to replace it unconditionally.
    ::el_set(s_editor, EL_GETCFN, do_getcfn);

    // Initialize the editor. Errors are ignored.
    ::el_set(s_editor, EL_TERMINAL, nullptr);
    ::el_set(s_editor, EL_EDITOR, "emacs");
    ::el_set(s_editor, EL_PROMPT, +[]{ return s_prompt;  });

    ::HistEvent event;
    ::history(s_history, &event, H_SETSIZE, repl_history_size);
    ::history(s_history, &event, H_SETUNIQUE, 1);
    ::el_set(s_editor, EL_HIST, ::history, s_history);

    // Make {Ctrl,Alt}+{Left,Right} move by words, like in bash.
    ::el_set(s_editor, EL_BIND, R"(\e[1;3C)", "em-next-word", nullptr);
    ::el_set(s_editor, EL_BIND, R"(\e[1;5C)", "em-next-word", nullptr);
    ::el_set(s_editor, EL_BIND, R"(\e[5C)", "em-next-word", nullptr);
    ::el_set(s_editor, EL_BIND, R"(\e\e[C)", "em-next-word", nullptr);

    ::el_set(s_editor, EL_BIND, R"(\e[1;3D)", "ed-prev-word", nullptr);
    ::el_set(s_editor, EL_BIND, R"(\e[1;5D)", "ed-prev-word", nullptr);
    ::el_set(s_editor, EL_BIND, R"(\e[5D)", "ed-prev-word", nullptr);
    ::el_set(s_editor, EL_BIND, R"(\e\e[D)", "ed-prev-word", nullptr);

    if(const char* home = ::getenv("HOME")) {
      auto path = cow_string(home) + "/.editrc";
      repl_printf("* loading settings from `%s`...", path.c_str());

      if(::access(path.c_str(), R_OK) == 0) {
        // Try loading `~/.editrc`. Errors are ignored.
        ::el_source(s_editor, path.c_str());
        repl_printf("* ... done.");
      }
      else {
        repl_printf("* ... ignored: %m");
      }
    }

    // Done.
    repl_printf("");
  }

}  // namespace

void
editline_set_prompt(const char* fmt, ...)
  {
    ::va_list ap;
    va_start(ap, fmt);
    ::vsnprintf(s_prompt, sizeof(s_prompt), fmt, ap);
    va_end(ap);
  }

bool
editline_gets(cow_string& line)
  {
    line.clear();

    s_once.call(do_init_once);
    int len = 0;
    const char* str = ::el_gets(s_editor, &len);

    // Check for errors.
    if((len == -1) && (errno == EINTR))
      return false;

    if(len == -1)
      exit_printf(exit_system_error, "! could not read standard input: %m");

    if(!str)
      return false;

    // Accept trailing new line characters, if any.
    line.append(str, (unsigned) len);
    return true;
  }

void
editline_puts(cow_stringR text)
  {
    s_once.call(do_init_once);
    ::el_push(s_editor, text.c_str());
  }

void
editline_reset()
  {
    s_once.call(do_init_once);
    ::el_reset(s_editor);
  }

void
editline_add_history(cow_stringR text)
  {
    s_once.call(do_init_once);
    ::HistEvent event;
    ::history(s_history, &event, H_ENTER, text.c_str());
  }

}  // namespace asteria
