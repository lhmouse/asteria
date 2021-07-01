// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SIMPLE_SCRIPT_HPP_
#define ASTERIA_SIMPLE_SCRIPT_HPP_

#include "fwd.hpp"
#include "runtime/global_context.hpp"

namespace asteria {

class Simple_Script
  {
  private:
    Compiler_Options m_opts;
    Global_Context m_global;

    cow_vector<phsh_string> m_params;
    cow_function m_func;

  public:
    explicit
    Simple_Script() noexcept
      { }

    explicit
    Simple_Script(const cow_string& name, tinybuf& cbuf)
      { this->reload(name, 1, cbuf);  }

    explicit
    Simple_Script(const cow_string& name, int line, tinybuf& cbuf)
      { this->reload(name, line, cbuf);  }

  public:
    const Compiler_Options&
    options() const noexcept
      { return this->m_opts;  }

    Compiler_Options&
    options() noexcept
      { return this->m_opts;  }

    const Global_Context&
    global() const noexcept
      { return this->m_global;  }

    Global_Context&
    global() noexcept
      { return this->m_global;  }

    explicit operator
    bool() const noexcept
      { return bool(this->m_func);  }

    operator
    const cow_function&() const noexcept
      { return this->m_func;  }

    Simple_Script&
    reset() noexcept
      {
        this->m_func.reset();
        return *this;
      }

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
    execute(Reference_Stack&& stack);

    Reference
    execute(cow_vector<Value>&& args = { });
  };

}  // namespace asteria

#endif
