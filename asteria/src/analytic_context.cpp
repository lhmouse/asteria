// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "analytic_context.hpp"
#include "utilities.hpp"

namespace Asteria {

Analytic_context::~Analytic_context()
  {
  }

bool Analytic_context::is_analytic() const noexcept
  {
    return true;
  }
const Abstract_context * Analytic_context::get_parent_opt() const noexcept
  {
    return m_parent_opt;
  }

void initialize_analytic_function_context(Analytic_context &ctx_out, const Vector<String> &params)
  {
    // Set up parameters.
    for(const auto &name : params) {
      if(name.empty() == false) {
        if(ctx_out.is_name_reserved(name)) {
          ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", name, "` is reserved and cannot be used.");
        }
        ctx_out.set_named_reference(name, { });
      }
    }
    // Set up system variables.
    ctx_out.set_named_reference(String::shallow("__file"), { });
    ctx_out.set_named_reference(String::shallow("__line"), { });
    ctx_out.set_named_reference(String::shallow("__this"), { });
    ctx_out.set_named_reference(String::shallow("__varg"), { });
  }

}
