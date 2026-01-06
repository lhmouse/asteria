// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SIMPLE_SCRIPT_
#define ASTERIA_SIMPLE_SCRIPT_

#include "fwd.hpp"
#include "runtime/global_context.hpp"
#include "runtime/reference.hpp"
#include "llds/reference_stack.hpp"
namespace asteria {

class Simple_Script
  {
  private:
    Compiler_Options m_opts;
    Global_Context m_global;
    cow_function m_func;

  public:
    explicit Simple_Script(API_Version version = api_version_latest)
      :
        m_global(version)
      { }

  public:
    const Compiler_Options&
    options()
      const noexcept
      { return this->m_opts;  }

    Compiler_Options&
    mut_options()
      noexcept
      { return this->m_opts;  }

    const Global_Context&
    global()
      const noexcept
      { return this->m_global;  }

    Global_Context&
    mut_global()
      noexcept
      { return this->m_global;  }

    explicit
    operator bool()
      const noexcept
      { return static_cast<bool>(this->m_func);  }

    operator const cow_function&()
      const noexcept
      { return this->m_func;  }

    void
    reset()
      noexcept
      { this->m_func.reset();  }

    // Manage global variables in the bundled context.
    refcnt_ptr<Variable>
    get_global_variable_opt(const phcow_string& name)
      const noexcept;

    refcnt_ptr<Variable>
    open_global_variable(const phcow_string& name);

    bool
    erase_global_variable(const phcow_string& name)
      noexcept;

    // Load a script, which may be either a sequence of statements or a
    // single expression.
    void
    reload(const cow_string& name, int start_line, tinyfmt&& cbuf);

    void
    reload_oneline(const cow_string& name, tinyfmt&& cbuf);

    void
    reload_string(const cow_string& name, int start_line, const cow_string& code);

    void
    reload_string(const cow_string& name, const cow_string& code);

    void
    reload_oneline(const cow_string& name, const cow_string& code);

    void
    reload_stdin(int start_line);

    void
    reload_stdin();

    void
    reload_file(const cow_string& path);

    // Execute the script that has been loaded.
    Reference
    execute(Reference_Stack&& stack);

    Reference
    execute(cow_vector<Value>&& args);

    Reference
    execute();
  };

}  // namespace asteria
#endif
