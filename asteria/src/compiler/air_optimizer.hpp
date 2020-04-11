// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_AIR_OPTIMIZER_HPP_
#define ASTERIA_COMPILER_AIR_OPTIMIZER_HPP_

#include "../fwd.hpp"
#include "../runtime/air_node.hpp"

namespace Asteria {

class AIR_Optimizer
  {
  private:
    Compiler_Options m_opts;
    cow_vector<phsh_string> m_params;
    cow_vector<AIR_Node> m_code;

  public:
    explicit constexpr AIR_Optimizer(const Compiler_Options& opts) noexcept
      : m_opts(opts)
      { }

    ~AIR_Optimizer();

    AIR_Optimizer(const AIR_Optimizer&)
      = delete;

    AIR_Optimizer& operator=(const AIR_Optimizer&)
      = delete;

  public:
    // These are accessors and modifiers of options for optimizing.
    const Compiler_Options& get_options() const noexcept
      { return this->m_opts;  }

    Compiler_Options& open_options() noexcept
      { return this->m_opts;  }

    AIR_Optimizer& set_options(const Compiler_Options& opts) noexcept
      { return this->m_opts = opts, *this;  }

    // These are accessors and modifiers of tokens in this stream.
    bool empty() const noexcept
      { return this->m_code.empty();  }

    operator const cow_vector<AIR_Node>& () const noexcept
      { return this->m_code;  }

    AIR_Optimizer& clear() noexcept
      { return this->m_code.clear(), *this;  }

    // This function performs code generation.
    // `ctx_opt` is the parent context this closure.
    AIR_Optimizer& reload(const Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
                          const cow_vector<Statement>& stmts);

    // This function loads some already-generated code.
    // `ctx_opt` is the parent context this closure.
    AIR_Optimizer& rebind(const Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
                          const cow_vector<AIR_Node>& code);

    // Create a closure value that can be assigned to a variable.
    cow_function create_function(const Source_Location& sloc, const cow_string& name);
  };

}  // namespace Asteria

#endif
