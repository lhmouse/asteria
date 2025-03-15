// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "analytic_context.hpp"
#include "../utils.hpp"
namespace asteria {

Analytic_Context::
Analytic_Context(Uxtc_function, const Abstract_Context* parent_opt,
                 const cow_vector<phsh_string>& params)
  :
    m_parent_opt(parent_opt)
  {
    // Set parameters, which are local references.
    for(const auto& name : params)
      if(name != "...")
        this->do_mut_named_reference(nullptr, name);

    // Set pre-defined references.
    // N.B. If you have ever changed these, remember to update
    // 'executive_context.cpp' as well.
    this->do_mut_named_reference(nullptr, &"__this");
    this->do_mut_named_reference(nullptr, &"__func");
    this->do_mut_named_reference(nullptr, &"__varg");
  }

Analytic_Context::
~Analytic_Context()
  {
  }

}  // namespace asteria
