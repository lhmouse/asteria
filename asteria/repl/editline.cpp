// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "fwd.hpp"
#include <stdarg.h>
#include <signal.h>
#include <histedit.h>
namespace asteria {
namespace {

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

    ::rocket::unique_ptr<::FILE, void (::FILE*)> locked_fp(::funlockfile);
    ::flockfile(fp);
    locked_fp.reset(fp);

  r:
    repl_signal.store(0);
    ::clearerr_unlocked(fp);
    ::wint_t wch = ::fgetwc_unlocked(fp);
    *out = (wchar_t)wch;

    if(wch != WEOF)
      return 1;  // success

    if(::feof_unlocked(fp))
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
    ROCKET_ASSERT(!s_history);
    s_history = ::history_init();
    if(!s_history)
      exit_printf(exit_system_error, "! could not initialize history: %m");

    ROCKET_ASSERT(!s_editor);
    s_editor = ::el_init(PACKAGE_TARNAME, stdin, stdout, stderr);
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

    // Load `~/.editrc`. Errors are ignored.
    const char* home = ::secure_getenv("HOME");
    if(!home)
      return;

    cow_string path = sref("/.editrc");
    path.insert(0, home);
    repl_printf("* loading settings from `%s`...", path.c_str());

    ::rocket::unique_ptr<char, void (void*)> abspath(::free);
    abspath.reset(::realpath(path.c_str(), nullptr));
    if(!abspath)
      return repl_printf("* ... ignored: %m");

    ::el_source(s_editor, nullptr);
    repl_printf("* ... done");
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

const char*
editline_gets(int* nchars)
  {
    if(!s_editor) {
      // Does this have to be thread-safe?
      repl_printf("* initializing editline...");
      do_init_once();
      repl_printf("");
    }

    return ::el_gets(s_editor, nchars);
  }

void
editline_reset()
  {
    if(!s_editor)
      return;

    ::el_reset(s_editor);
  }

void
editline_add_history(stringR text)
  {
    if(!s_history)
      return;

    ::HistEvent event;
    ::history(s_history, &event, H_ENTER, text.c_str());
  }

}  // namespace asteria
