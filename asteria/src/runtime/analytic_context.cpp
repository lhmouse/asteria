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
    bool variadic = false;
    for(const auto& name : params) {
      if(name.empty())
        continue;

      // Nothing is set for the variadic placeholder, but the parameter list
      // terminates here.
      variadic = (name.size() == 3) && mem_equal(name.c_str(), "...", 4);
      if(variadic)
        break;

      // Its contents are out of interest.
      this->open_named_reference(name) /*= Reference::S_uninit()*/;
    }

    // Set pre-defined references.
    // N.B. If you have ever changed these, remember to update 'executive_context.cpp'
    // as well.
    this->open_named_reference(::rocket::sref("__varg")) /*= Reference::S_uninit()*/;
    this->open_named_reference(::rocket::sref("__this")) /*= Reference::S_uninit()*/;
    this->open_named_reference(::rocket::sref("__func")) /*= Reference::S_uninit()*/;
  }

Analytic_Context::
~Analytic_Context()
  {
  }

}  // namespace asteria
