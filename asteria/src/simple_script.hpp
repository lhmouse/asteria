// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SIMPLE_SCRIPT_HPP_
#define ASTERIA_SIMPLE_SCRIPT_HPP_

#include "fwd.hpp"

namespace asteria {

class Simple_Script
  {
  private:
    Compiler_Options m_opts;  // static
    cow_vector<phsh_string> m_params;  // constant
    cow_function m_func;  // note type erasure

  public:
    explicit constexpr
    Simple_Script()
      noexcept
      : m_opts()
      { }

    explicit
    Simple_Script(const cow_string& name, tinybuf& cbuf)
      : m_opts()
      { this->reload(name, 1, cbuf);  }

    explicit
    Simple_Script(const cow_string& name, int line, tinybuf& cbuf)
      : m_opts()
      { this->reload(name, line, cbuf);  }

    explicit constexpr
    Simple_Script(const Compiler_Options& opts)
      noexcept
      : m_opts(opts)
      { }

    explicit
    Simple_Script(const Compiler_Options& opts, const cow_string& name, tinybuf& cbuf)
      : m_opts(opts)
      { this->reload(name, 1, cbuf);  }

    explicit
    Simple_Script(const Compiler_Options& opts, const cow_string& name, int line, tinybuf& cbuf)
      : m_opts(opts)
      { this->reload(name, line, cbuf);  }

  public:
    const Compiler_Options&
    get_options()
      const noexcept
      { return this->m_opts;  }

    Compiler_Options&
    open_options()
      noexcept
      { return this->m_opts;  }

    Simple_Script&
    set_options(const Compiler_Options& opts)
      noexcept
      { return this->m_opts = opts, *this;  }

    explicit operator
    bool()
      const noexcept
      { return bool(this->m_func);  }

    Simple_Script&
    clear()
      noexcept
      {
        this->m_func.reset();
        return *this;
      }

    operator
    const cow_function&()
      const noexcept
      { return this->m_func;  }

    // Load a script.
    Simple_Script&
    reload(const cow_string& name, int line, tinybuf& cbuf);

    Simple_Script&
    reload_string(const cow_string& name, const cow_string& code);

    Simple_Script&
    reload_string(const cow_string& name, int line, const cow_string& code);

    Simple_Script&
    reload_stdin();

    Simple_Script&
    reload_stdin(int line);

    Simple_Script&
    reload_file(const char* path);

    // Execute the script that has been loaded.
    Reference
    execute(Global_Context& global, Reference_Stack&& stack)
      const;

    Reference
    execute(Global_Context& global, cow_vector<Value>&& args = { })
      const;
  };

}  // namespace asteria

#endif
