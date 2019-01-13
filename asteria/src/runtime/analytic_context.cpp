// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "analytic_context.hpp"
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
      this->open_named_reference(param) /*= Reference_root::S_null()*/;
    }
    // Set pre-defined variables.
    // N.B. You must keep these elements sorted.
    // N.B. If you have ever changed these, remember to update 'executive_context.cpp' as well.
    this->open_named_reference(std::ref("__file")) /*= Reference_root::S_null()*/;
    this->open_named_reference(std::ref("__func")) /*= Reference_root::S_null()*/;
    this->open_named_reference(std::ref("__line")) /*= Reference_root::S_null()*/;
    this->open_named_reference(std::ref("__this")) /*= Reference_root::S_null()*/;
    this->open_named_reference(std::ref("__varg")) /*= Reference_root::S_null()*/;
  }

}
