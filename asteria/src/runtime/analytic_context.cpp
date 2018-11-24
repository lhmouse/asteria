// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "analytic_context.hpp"
#include "reference.hpp"
#include "../utilities.hpp"

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

Reference & Analytic_context::mutate_named_reference(const rocket::prehashed_string &name)
  {
    return this->m_dict.mut(name);
  }

void Analytic_context::initialize_for_function(const rocket::cow_vector<rocket::prehashed_string> &params)
  {
    for(std::size_t i = 0; i < params.size(); ++i) {
      const auto &param = params.at(i);
      if(param.empty()) {
        continue;
      }
      if(param.rdstr().starts_with("__")) {
        ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", param, "` is reserved and cannot be used.");
      }
      // Ensure the named reference exists, whose contents are out of interest.
      this->m_dict.mut(param) /*= Reference_root::S_null()*/;
    }
  }

}
