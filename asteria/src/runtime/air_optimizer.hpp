// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_OPTIMIZER_HPP_
#define ASTERIA_RUNTIME_AIR_OPTIMIZER_HPP_

#include "../fwd.hpp"
#include "air_node.hpp"

namespace asteria {

class AIR_Optimizer
  {
  private:
    Compiler_Options m_opts;
    cow_vector<phsh_string> m_params;
    cow_vector<AIR_Node> m_code;

  public:
    explicit constexpr
    AIR_Optimizer(const Compiler_Options& opts) noexcept
      : m_opts(opts)
      { }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(AIR_Optimizer);

    // These are accessors and modifiers of options for optimizing.
    const Compiler_Options&
    get_options() const noexcept
      { return this->m_opts;  }

    Compiler_Options&
    open_options() noexcept
      { return this->m_opts;  }

    AIR_Optimizer&
    set_options(const Compiler_Options& opts) noexcept
      { this->m_opts = opts;  return *this;  }

    // These are accessors and modifiers of tokens in this stream.
    bool
    empty() const noexcept
      { return this->m_code.empty();  }

    operator
    const cow_vector<AIR_Node>&() const noexcept
      { return this->m_code;  }

    AIR_Optimizer&
    clear() noexcept
      {
        this->m_code.clear();
        return *this;
      }

    // This function performs code generation.
    // `ctx_opt` is the parent context this closure.
    AIR_Optimizer&
    reload(Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
           const Global_Context& global, const cow_vector<Statement>& stmts);

    // This function loads some already-generated code.
    // `ctx_opt` is the parent context this closure.
    AIR_Optimizer&
    rebind(Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
           const cow_vector<AIR_Node>& code);

    // Create a closure value that can be assigned to a variable.
    cow_function
    create_function(const Source_Location& sloc, const cow_string& name);
  };

}  // namespace asteria

#endif
