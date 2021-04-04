// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variadic_arguer.hpp"
#include "variable_callback.hpp"
#include "argument_reader.hpp"
#include "../utils.hpp"

namespace asteria {

tinyfmt&
Variadic_Arguer::
describe(tinyfmt& fmt)
  const
  {
    return fmt << "`__varg([index])` at '" << this->m_sloc << "'";
  }

Variable_Callback&
Variadic_Arguer::
enumerate_variables(Variable_Callback& callback)
  const
  {
    return ::rocket::for_each(this->m_vargs, callback), callback;
  }

Reference&
Variadic_Arguer::
invoke_ptc_aware(Reference& self, Global_Context& /*global*/, Reference_Stack&& stack)
  const
  {
    Argument_Reader reader(sref("__varg"), ::std::move(stack));

    // If an argument is specified, it shall be the index of a variadic argument as
    // an integer, or an explicit `null`.
    Opt_integer qindex;

    reader.start_overload();
    reader.optional(qindex);    // [index]
    if(!reader.end_overload())
      reader.throw_no_matching_function_call();

    // If no argument is given, return the number of variadic arguments.
    if(!qindex)
      return self.set_temporary(this->m_vargs.ssize());

    // Wrap the index as necessary.
    // If the index is out of range, return a constant `null`.
    auto w = wrap_index(*qindex, this->m_vargs.size());
    if(w.nprepend | w.nappend)
      return self.set_temporary(nullopt);

    // Copy the reference at `*qindex`.
    return self = this->m_vargs.at(w.rindex);
  }

}  // namespace asteria
