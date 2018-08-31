// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "analytic_context.hpp"
#include "reference.hpp"
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
    return this->m_parent_opt;
  }

void Analytic_context::initialize_for_function(const Vector<String> &params)
  {
    // Set up parameters.
    for(const auto &name : params) {
      if(name.empty() == false) {
        if(is_name_reserved(name)) {
          ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", name, "` is reserved and cannot be used.");
        }
        this->set_named_reference(name, { });
      }
    }
    // Set up system variables.
    this->set_named_reference(String::shallow("__file"), { });
    this->set_named_reference(String::shallow("__line"), { });
    this->set_named_reference(String::shallow("__this"), { });
    this->set_named_reference(String::shallow("__varg"), { });
  }

}
