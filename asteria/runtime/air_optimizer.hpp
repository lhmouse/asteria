// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_OPTIMIZER_
#define ASTERIA_RUNTIME_AIR_OPTIMIZER_

#include "../fwd.hpp"
namespace asteria {

class AIR_Optimizer
  {
  private:
    Compiler_Options m_opts;
    cow_vector<phcow_string> m_params;
    cow_vector<AIR_Node> m_code;

  public:
    explicit constexpr AIR_Optimizer(const Compiler_Options& opts) noexcept
      :
        m_opts(opts)
      { }

  public:
    AIR_Optimizer(const AIR_Optimizer&) = delete;
    AIR_Optimizer& operator=(const AIR_Optimizer&) & = delete;
    ~AIR_Optimizer();

    // These are accessors and modifiers of options for optimizing.
    const Compiler_Options&
    get_options() const noexcept
      { return this->m_opts;  }

    Compiler_Options&
    mut_options() noexcept
      { return this->m_opts;  }

    void
    set_options(const Compiler_Options& opts) noexcept
      { this->m_opts = opts;  }

    // These are accessors and modifiers of tokens in this stream.
    bool
    empty() const noexcept
      { return this->m_code.empty();  }

    const cow_vector<AIR_Node>&
    get_code() const noexcept
      { return this->m_code;  }

    cow_vector<AIR_Node>&
    mut_code() noexcept
      { return this->m_code;  }

    void
    clear() noexcept;

    // This function performs code generation.
    // `ctx_opt` is the parent context of this closure.
    void
    reload(const Abstract_Context* ctx_opt, const cow_vector<phcow_string>& params,
           const Global_Context& global, const cow_vector<Statement>& stmts);

    // This function loads some already-generated code.
    // `ctx_opt` is the parent context of this closure.
    void
    rebind(const Abstract_Context* ctx_opt, const cow_vector<phcow_string>& params,
           const cow_vector<AIR_Node>& code);

    // Create a closure value that can be assigned to a variable.
    cow_function
    create_function(const Source_Location& sloc, const cow_string& name);
  };

}  // namespace asteria
#endif
