// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/abstract_hooks.hpp"
#include "../source_location.hpp"
#include "../utils.hpp"
#include <signal.h>  // sig_atomic_t, sigaction()
#include <string.h>  // strsignal()

namespace asteria {
namespace {

class Verbose_Hooks
  final
  : public Abstract_Hooks
  {
  private:
    ::rocket::tinyfmt_str m_fmt;  // reusable storage

  public:
    explicit
    Verbose_Hooks()
      noexcept
      = default;

  private:
    template<typename... ParamsT>
    void
    do_verbose_trace(const Source_Location& sloc, const char* templ,
                     const ParamsT&... params)
      {
        if(ROCKET_EXPECT(!repl_cmdline.verbose))
          return;

        // Compose the string to write.
        this->m_fmt.clear_string();
        this->m_fmt << "REPL running at '" << sloc << "':\n";
        format(this->m_fmt, templ, params...);  // ADL intended

        // Extract the string and write it to standard error.
        // Errors are ignored.
        auto str = this->m_fmt.extract_string();
        write_log_to_stderr(__FILE__, __LINE__, ::std::move(str));

        // Reuse the storage.
        this->m_fmt.set_string(::std::move(str));
      }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Verbose_Hooks);

    void
    on_single_step_trap(const Source_Location& sloc)
      override
      {
        int sig = get_and_clear_last_signal();
        if(sig)
          ASTERIA_THROW("$1 received\n[callback inside '$2']",
                        ::strsignal(sig), sloc);
      }

    void
    on_variable_declare(const Source_Location& sloc, const phsh_string& name)
      override
      {
        this->do_verbose_trace(sloc,
                "declaring variable `$1`", name);
      }

    void
    on_function_call(const Source_Location& sloc, const cow_function& target)
      override
      {
        this->do_verbose_trace(sloc,
                "initiating function call: $1", target);
      }

    void
    on_function_return(const Source_Location& sloc, const cow_function& target,
                       const Reference&)
      override
      {
        this->do_verbose_trace(sloc,
                "returned from function call: $1", target);
      }

    void
    on_function_except(const Source_Location& sloc, const cow_function& target,
                       const Runtime_Error&)
      override
      {
        this->do_verbose_trace(sloc,
                "caught an exception inside function call: $1", target);
      }
  };

Verbose_Hooks::
~Verbose_Hooks()
  {
  }

volatile ::sig_atomic_t s_intsig;

}  // namespace

int
get_and_clear_last_signal()
  noexcept
  {
    return ::std::exchange(s_intsig, 0);
  }

void
install_signal_and_verbose_hooks()
  {
    // Trap Ctrl-C. Errors are ignored.
    struct ::sigaction sigx = { };
    sigx.sa_handler = [](int n) { s_intsig = n;  };
    ::sigaction(SIGINT, &sigx, nullptr);

    // Set the hooks class.
    repl_global.set_hooks(::rocket::make_refcnt<Verbose_Hooks>());
  }

} // namespace asteria
