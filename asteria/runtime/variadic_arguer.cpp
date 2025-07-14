// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "variadic_arguer.hpp"
#include "../runtime/argument_reader.hpp"
#include "../utils.hpp"
namespace asteria {

tinyfmt&
Variadic_Arguer::
describe(tinyfmt& fmt)
  const
  {
    return fmt << "`__varg([index])` at '" << this->m_sloc << "'";
  }

void
Variadic_Arguer::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp)
  const
  {
    for(const auto& arg : this->m_vargs)
      arg.collect_variables(staged, temp);
  }

void
Variadic_Arguer::
invoke_ptc_aware(Reference& self, Global_Context& /*global*/, Reference_Stack&& stack)
  const
  {
    Argument_Reader reader(&"__varg", move(stack));

    // If an argument is specified, it shall be the index of a variadic
    // argument as an integer, or an explicit `null`.
    optV_integer qindex;

    reader.start_overload();
    reader.optional(qindex);    // [index]
    if(!reader.end_overload())
      reader.throw_no_matching_function_call();

    if(!qindex)
      self.set_temporary(this->m_vargs.ssize());
    else {
      // Wrap the index as necessary. If the index is out of range, return
      // a constant `null`.
      auto w = wrap_array_index(this->m_vargs.ssize(), *qindex);
      if(w.nprepend | w.nappend)
        self.set_temporary(nullopt);
      else
        self = this->m_vargs.at(w.rindex);
    }
  }

}  // namespace asteria
