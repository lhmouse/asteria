// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "analytic_context.hpp"
#include "../util.hpp"

namespace asteria {

Analytic_Context::
Analytic_Context(M_function, Abstract_Context* parent_opt,
                 const cow_vector<phsh_string>& params)
  : m_parent_opt(parent_opt)
  {
    // Set parameters, which are local references.
    for(size_t i = 0;  i < params.size();  ++i) {
      const auto& name = params.at(i);
      if(name.empty())
        continue;

      if(name.rdstr().starts_with("__"))
        ASTERIA_THROW("Reserved name not declarable as parameter (name `", name, "`)");

      if(name == "...") {
        // Nothing is set for the variadic placeholder, but the parameter list terminates here.
        ROCKET_ASSERT(i == params.size() - 1);
        break;
      }

      // Its contents are out of interest.
      this->open_named_reference(name) /*= Reference::S_uninit()*/;
    }

    // Set pre-defined references.
    // N.B. If you have ever changed these, remember to update 'executive_context.cpp' as well.
    this->open_named_reference(::rocket::sref("__varg")) /*= Reference::S_uninit()*/;
    this->open_named_reference(::rocket::sref("__this")) /*= Reference::S_uninit()*/;
    this->open_named_reference(::rocket::sref("__func")) /*= Reference::S_uninit()*/;
  }

Analytic_Context::
~Analytic_Context()
  {
  }

}  // namespace asteria
