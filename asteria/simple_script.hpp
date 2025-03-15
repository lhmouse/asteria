// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

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
    options() const noexcept
      { return this->m_opts;  }

    Compiler_Options&
    mut_options() noexcept
      { return this->m_opts;  }

    const Global_Context&
    global() const noexcept
      { return this->m_global;  }

    Global_Context&
    mut_global() noexcept
      { return this->m_global;  }

    explicit operator bool() const noexcept
      { return static_cast<bool>(this->m_func);  }

    operator const cow_function&() const noexcept
      { return this->m_func;  }

    void
    reset() noexcept
      { this->m_func.reset();  }

    // Manage global variables in the bundled context.
    refcnt_ptr<Variable>
    get_global_variable_opt(phsh_stringR name) const noexcept;

    refcnt_ptr<Variable>
    open_global_variable(phsh_stringR name);

    bool
    erase_global_variable(phsh_stringR name) noexcept;

    // Load something. Calling these functions directly is not recommended.
    void
    reload(cow_stringR name, Statement_Sequence&& stmtq);

    void
    reload(cow_stringR name, Token_Stream&& tstrm);

    void
    reload(cow_stringR name, int line, tinybuf&& cbuf);

    // Load a script.
    void
    reload_string(cow_stringR name, int line, cow_stringR code);

    void
    reload_string(cow_stringR name, cow_stringR code);

    void
    reload_stdin(int line);

    void
    reload_stdin();

    void
    reload_file(cow_stringR path);

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
