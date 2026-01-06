// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "analytic_context.hpp"
#include "../utils.hpp"
namespace asteria {

Analytic_Context::
Analytic_Context(Uxtc_function, const Abstract_Context* parent_opt,
                 const cow_vector<phcow_string>& params)
  :
    m_parent_opt(parent_opt)
  {
    // Set parameters, which are local references.
    for(const auto& name : params)
      if(is_cmask(name[0], cmask_namei))
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
