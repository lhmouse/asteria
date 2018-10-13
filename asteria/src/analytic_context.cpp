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

const Reference * Analytic_context::get_named_reference_opt(const String &name) const
  {
    // Deal with pre-defined variables.
    // If you add new entries or alter existent entries here, you must update `Executive_context` as well.
    if(name == "__file") {
      return &(this->m_dummy);
    }
    if(name == "__line") {
      return &(this->m_dummy);
    }
    if(name == "__func") {
      return &(this->m_dummy);
    }
    if(name == "__this") {
      return &(this->m_dummy);
    }
    if(name == "__varg") {
      return &(this->m_dummy);
    }
    // Call the overriden function with anything unhandled.
    return Abstract_context::get_named_reference_opt(name);
  }

void Analytic_context::initialize_for_function(const Vector<String> &params)
  {
    // Materialize parameters.
    for(const auto &param : params) {
      if(!param.empty()) {
        if(this->is_name_reserved(param)) {
          ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", param, "` is reserved and cannot be used.");
        }
        this->set_named_reference(param, { });
      }
    }
  }

}
