// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variadic_arguer.hpp"
#include "variable_callback.hpp"
#include "../library/argument_reader.hpp"
#include "../utilities.hpp"

namespace Asteria {

tinyfmt& Variadic_Arguer::describe(tinyfmt& fmt) const
  {
    return fmt << "<built-in>.__varg([index]) @ " << this->m_sloc;
  }

Variable_Callback& Variadic_Arguer::enumerate_variables(Variable_Callback& callback) const
  {
    return ::rocket::for_each(this->m_vargs, callback), callback;
  }

Reference& Variadic_Arguer::invoke_ptc_aware(Reference& self, Global_Context& /*global*/,
                                             cow_vector<Reference>&& args) const
  {
    Argument_Reader reader(::rocket::sref("<built-in>.__varg"), ::rocket::ref(args));
    // Extract arguments.
    opt<G_integer> qindex;
    if(reader.I().g(qindex).F()) {
      auto nvargs = this->m_vargs.size();
      if(!qindex) {
        // Return the number of variadic arguments if `index` is `null` or absent.
        Reference_Root::S_constant xref = { G_integer(nvargs) };
        return self = ::rocket::move(xref);
      }
      // Return the argument at `index`.
      auto w = wrap_index(*qindex, nvargs);
      auto nadd = w.nprepend | w.nappend;
      if(nadd != 0) {
        // Return a `null`.
        return self = Reference_Root::S_constant();
      }
      return self = this->m_vargs.at(w.rindex);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }

}  // namespace Asteria
