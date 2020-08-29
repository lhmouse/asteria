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
    constexpr
    Simple_Script()
    noexcept
      : m_opts()
      { }

    explicit constexpr
    Simple_Script(const Compiler_Options& opts)
    noexcept
      : m_opts(opts)
      { }

    Simple_Script(tinybuf& cbuf, const cow_string& name)
      : m_opts()
      { this->reload(cbuf, name);  }

    Simple_Script(const Compiler_Options& opts, tinybuf& cbuf, const cow_string& name)
      : m_opts(opts)
      { this->reload(cbuf, name);  }

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
    reload(tinybuf& cbuf, const cow_string& name);

    Simple_Script&
    reload_string(const cow_string& code, const cow_string& name);

    Simple_Script&
    reload_file(const char* path);

    Simple_Script&
    reload_stdin();

    // Execute the script that has been loaded.
    Reference
    execute(Global_Context& global, cow_vector<Reference>&& args = { })
    const;

    Reference
    execute(Global_Context& global, cow_vector<Value>&& vals)
    const;
  };

}  // namespace asteria

#endif
