// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "analytic_context.hpp"
#include "../utilities.hpp"

namespace Asteria {

Analytic_Context::~Analytic_Context()
  {
  }

void Analytic_Context::prepare_function_parameters(const Cow_Vector<PreHashed_String>& params)
  {
    // Set parameters, which are local references.
    for(std::size_t i = 0; i < params.size(); ++i) {
      const auto& param = params.at(i);
      if(param.empty()) {
        continue;
      }
      if(param == "...") {
        // Nothing is set for the variadic placeholder, but the parameter list terminates here.
        ROCKET_ASSERT(i == params.size() - 1);
        break;
      }
      if(param.rdstr().starts_with("__")) {
        ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", param, "` is reserved and cannot be used.");
      }
      // Its contents are out of interest.
      this->open_named_reference(param) /*= Reference_Root::S_null()*/;
    }
    // Set pre-defined references.
    // N.B. If you have ever changed these, remember to update 'executive_context.cpp' as well.
    this->open_named_reference(rocket::sref("__varg")) /*= Reference_Root::S_null()*/;
    this->open_named_reference(rocket::sref("__this")) /*= Reference_Root::S_null()*/;
    this->open_named_reference(rocket::sref("__func")) /*= Reference_Root::S_null()*/;
  }

}  // namespace Asteria
