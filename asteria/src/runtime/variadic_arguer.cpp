// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variadic_arguer.hpp"
#include "../library/argument_sentry.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Variadic_Arguer::describe(std::ostream &os) const
  {
    os << "<builtin>.__varg() @ " << this->m_sloc;
  }

void Variadic_Arguer::invoke(Reference &self_io, const Global_Context & /*global*/, CoW_Vector<Reference> &&args) const
  {
    Argument_Sentry sentry(rocket::sref("<builtin>.__varg"), args);
    // `__varg()`
    const auto nvargs = this->get_argument_count();
    if(sentry.start().finish()) {
      // Return the number of variadic arguments.
      Reference_Root::S_constant ref_c = { D_integer(nvargs) };
      self_io = std::move(ref_c);
      return;
    }
    // `__varg(index)`
    D_integer index;
    if(sentry.start().req(index).finish()) {
      // Return the argument at the index specified.
      auto wrap = wrap_index(index, nvargs);
      if(wrap.index >= nvargs) {
        ASTERIA_DEBUG_LOG("Variadic argument index is out of range: index = ", index, ", nvarg = ", nvargs);
        self_io = Reference_Root::S_null();
        return;
      }
      self_io = this->get_argument(wrap.index);
      return;
    }
    // Fail.
    sentry.throw_no_matching_function_call();
  }

void Variadic_Arguer::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    rocket::for_each(this->m_vargs, [&](const Reference &arg) { arg.enumerate_variables(callback);  });
  }

}
