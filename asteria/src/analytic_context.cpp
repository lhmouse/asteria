// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "analytic_context.hpp"
#include "function_header.hpp"
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

const Reference * Analytic_context::get_named_reference_opt(const rocket::prehashed_string &name) const
  {
    // Check for overriden references.
    const auto qref = this->m_dict.get_opt(name);
    if(qref) {
      return qref;
    }
    // Deal with pre-defined variables.
    if(name.rdstr().starts_with("__")) {
      // If you add new entries or alter existent entries here, remember to update `Executive_context` as well.
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
    }
    return nullptr;
  }

void Analytic_context::set_named_reference(const rocket::prehashed_string &name, Reference /*ref*/)
  {
    this->m_dict.set(name, this->m_dummy);
  }

void Analytic_context::initialize_for_function(const Function_header &head)
  {
    for(std::size_t i = 0; i < head.get_param_count(); ++i) {
      const auto &name = head.get_param_name(i);
      if(name.empty()) {
        continue;
      }
      if(name.rdstr().starts_with("__")) {
        ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", name, "` is reserved and cannot be used.");
      }
      this->m_dict.set(name, this->m_dummy);
    }
  }

}
