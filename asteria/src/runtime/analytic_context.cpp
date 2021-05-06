// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "analytic_context.hpp"
#include "../utils.hpp"

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

      // Nothing is set for the variadic placeholder, but the parameter
      // list terminates here.
      variadic = name == sref("...");
      if(variadic)
        break;

      // Its contents are out of interest.
      this->do_open_named_reference(nullptr, name).set_uninit();
    }

    // Set pre-defined references.
    // N.B. If you have ever changed these, remember to update
    // 'executive_context.cpp' as well.
    this->do_open_named_reference(nullptr, sref("__varg")).set_uninit();
    this->do_open_named_reference(nullptr, sref("__this")).set_uninit();
    this->do_open_named_reference(nullptr, sref("__func")).set_uninit();
  }

Analytic_Context::
~Analytic_Context()
  {
  }

}  // namespace asteria
