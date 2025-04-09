// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../asteria/xprecompiled.hpp"
#include "fwd.hpp"
#include "../asteria/simple_script.hpp"
#include "../asteria/source_location.hpp"
#include "../asteria/runtime/abstract_hooks.hpp"
#include "../asteria/runtime/instantiated_function.hpp"
#include "../asteria/runtime/runtime_error.hpp"
#include "../asteria/utils.hpp"
#include <stdarg.h>  // va_list, va_start(), va_end()
#include <stdlib.h>  // exit(), _Exit()
#include <stdio.h>  // fflush(), fprintf(), stderr
#include <signal.h>  // sys_siglist
namespace asteria {
namespace {

struct Verbose_Hooks
  :
     Abstract_Hooks
  {
    ::rocket::tinyfmt_str m_fmt;  // reusable storage

    template<typename... xParams>
    void
    do_verbose_trace(const Source_Location& sloc, const char* templ, const xParams&... params)
      {
        if(ROCKET_EXPECT(!repl_verbose))
          return;

        // Compose the string to write.
        this->m_fmt.clear_string();
        format(this->m_fmt, "REPL running at '$1':\n", sloc);  // ADL intended
        format(this->m_fmt, templ, params...);

        // Extract the string and write it to standard error.
        // Errors are ignored.
        auto str = this->m_fmt.extract_string();
        ASTERIA_WRITE_STDERR(move(str));

        // Reuse the storage. This is not use after move.
        this->m_fmt.set_string(move(str));
      }

    void
    on_trap(const Source_Location& sloc, Executive_Context& /*ctx*/) override
      {
        int sig = repl_signal.xchg(0);
        if(sig == 0)
          return;

        // Does the REPL have to be thread-safe anyway?
        const char* sigstr = ::strsignal(sig);
        this->do_verbose_trace(sloc, "Received signal `$1: $2`", sig, sigstr);

        ::rocket::sprintf_and_throw<::std::runtime_error>(
              "Received signal `%d: %s`\n[interrupted at '%s:%d']",
              sig, sigstr, sloc.c_file(), sloc.line());
      }

    void
    on_call(const Source_Location& sloc, const cow_function& target) override
      {
        this->do_verbose_trace(sloc, "Calling $1", target);
      }

    void
    on_return(const Source_Location& sloc, PTC_Aware /*ptc*/) override
      {
        this->do_verbose_trace(sloc, "Returning");
      }

    void
    on_throw(const Source_Location& sloc, Value& except) override
      {
        this->do_verbose_trace(sloc, "Throwing exception:\n$1", except);
      }
  };

}  // namespace

bool repl_verbose;
bool repl_interactive;
Simple_Script repl_script;
atomic_relaxed<int> repl_signal;

unsigned long repl_index;  // snippet index
cow_string repl_source;  // snippet text
cow_string repl_file;  // name of snippet
cow_vector<Value> repl_args;  // script arguments
cow_string repl_heredoc;  // heredoc terminator

cow_string repl_last_source;
cow_string repl_last_file;

void
repl_vprintf(const char* fmt, ::va_list ap) noexcept
  {
    ::flockfile(stderr);
    ::vfprintf(stderr, fmt, ap);
    putc_unlocked('\n', stderr);
    ::funlockfile(stderr);
  }

void
repl_printf(const char* fmt, ...) noexcept
  {
    // Output the string to standard error.
    ::va_list ap;
    va_start(ap, fmt);
    repl_vprintf(fmt, ap);
    va_end(ap);
  }

[[noreturn]]
void
quick_exit(Exit_Status stat) noexcept
  {
    ::fflush(nullptr);
    ::_Exit(stat);
  }

void
exit_printf(Exit_Status stat, const char* fmt, ...) noexcept
  {
    // Output the string to standard error.
    ::va_list ap;
    va_start(ap, fmt);
    repl_vprintf(fmt, ap);
    va_end(ap);

    // Perform normal exit if verbose mode is on. This will help
    // catching memory leaks upon exit.
    if(repl_verbose)
      ::exit(stat);
    else
      quick_exit(stat);
  }

void
install_verbose_hooks()
  {
    repl_script.mut_global().set_hooks(::rocket::make_refcnt<Verbose_Hooks>());
    repl_script.mut_options().verbose_single_step_traps = true;
  }

}  // namespace asteria
