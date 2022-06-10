// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "fwd.hpp"
#include "../simple_script.hpp"
#include "../source_location.hpp"
#include "../runtime/abstract_hooks.hpp"
#include "../utils.hpp"
#include <stdarg.h>  // va_list, va_start(), va_end()
#include <stdlib.h>  // exit(), quick_exit()
#include <stdio.h>  // fflush(), fprintf(), stderr
#include <signal.h>  // sys_siglist

namespace asteria {
namespace {

struct Verbose_Hooks final
  :  Abstract_Hooks
  {
    ::rocket::tinyfmt_str m_fmt;  // reusable storage

    template<typename... ParamsT>
    void
    do_verbose_trace(const Source_Location& sloc, const char* templ, const ParamsT&... params)
      {
        if(ROCKET_EXPECT(!repl_verbose))
          return;

        // Compose the string to write.
        this->m_fmt.clear_string();
        this->m_fmt << "REPL running at '" << sloc << "':\n";
        format(this->m_fmt, templ, params...);  // ADL intended

        // Extract the string and write it to standard error.
        // Errors are ignored.
        auto str = this->m_fmt.extract_string();
        write_log_to_stderr(__FILE__, __LINE__, __func__, ::std::move(str));

        // Reuse the storage.
        this->m_fmt.set_string(::std::move(str));
      }

    void
    on_single_step_trap(const Source_Location& sloc) override
      {
        uint32_t sig = (uint32_t) repl_signal.xchg(0);
        if(sig == 0)
          return;

        this->do_verbose_trace(sloc,
            "Received signal $1: $2", sig, ::sys_siglist[sig]);

        ASTERIA_THROW((
            "Signal $1 received: $2",
            "[thrown from '$3']"),
            sig, ::sys_siglist[sig], sloc);
      }

    void
    on_variable_declare(const Source_Location& sloc, const phsh_string& name) override
      {
        this->do_verbose_trace(sloc,
            "Declaring variable `$1`", name);
      }

    void
    on_function_call(const Source_Location& sloc, const cow_function& target) override
      {
        this->do_verbose_trace(sloc,
            "Initiating function call: $1", target);
      }

    void
    on_function_return(const Source_Location& sloc, const cow_function& target,
                       const Reference&) override
      {
        this->do_verbose_trace(sloc,
            "Returned from function call: $1", target);
      }

    void
    on_function_except(const Source_Location& sloc, const cow_function& target,
                       const Runtime_Error&) override
      {
        this->do_verbose_trace(sloc,
            "Caught an exception inside function call: $1", target);
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
    ::fputc_unlocked('\n', stderr);
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

void
exit_printf(Exit_Status stat, const char* fmt, ...) noexcept
  {
    // Output the string to standard error.
    ::va_list ap;
    va_start(ap, fmt);
    repl_vprintf(fmt, ap);
    va_end(ap);

    // Perform normal exit if verbose mode is on.
    // This helps catching memory leaks upon exit.
    if(repl_verbose)
      ::exit((int) stat);

    // Perform fast exit by default.
    ::fflush(nullptr);
    ::quick_exit((int) stat);
  }

void
initialize_global_context(const void* stack_base)
  {
    repl_script.global().set_recursion_base(stack_base);
  }

void
install_verbose_hooks()
  {
    repl_script.global().set_hooks(::rocket::make_refcnt<Verbose_Hooks>());
  }

}  // namespace asteria
