// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../asteria/xprecompiled.hpp"
#include "fwd.hpp"
#include <editline/readline.h>
#include <stdarg.h>
namespace asteria {
namespace {

bool s_init = false;
char s_prompt[64];

}  // namespace

void
libedit_set_prompt(const char* fmt, ...)
  {
    ::va_list ap;
    va_start(ap, fmt);
    ::vsnprintf(s_prompt, sizeof(s_prompt), fmt, ap);
    va_end(ap);
  }

bool
libedit_gets(cow_string& line)
  {
    if(!s_init) {
      ::rl_initialize();
      s_init = true;
    }

    unique_ptr<char, void (void*)> uptr(::readline(s_prompt), ::free);
    if(!uptr)
      return false;

    line.clear();
    line.append(uptr.get());
    line.push_back('\n');
    return true;
  }

void
libedit_puts(cow_stringR text)
  {
    ::rl_insert_text(text.c_str());
  }

void
libedit_reset()
  {
    ::clearerr(stdin);
    ::clearerr(stdout);
    ::clearerr(stderr);
    ::rl_reset_terminal(nullptr);
  }

void
libedit_add_history(cow_stringR text)
  {
    ::add_history(text.c_str());
  }

}  // namespace asteria
