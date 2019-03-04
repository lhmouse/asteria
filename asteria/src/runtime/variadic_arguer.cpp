// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variadic_arguer.hpp"
#include "../library/argument_reader.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Variadic_Arguer::describe(std::ostream &os) const
  {
    os << "<builtin>.__varg() @ " << this->m_sloc;
  }

void Variadic_Arguer::invoke(Reference &self_io, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) const
  {
    Argument_Reader reader(rocket::sref("<builtin>.__varg"), args);
    // `__varg()`
    auto nvargs = this->get_argument_count();
    if(reader.start().finish()) {
      // Return the number of variadic arguments.
      Reference_Root::S_constant ref_c = { D_integer(nvargs) };
      self_io = rocket::move(ref_c);
      return;
    }
    // `__varg(index)`
    D_integer index;
    if(reader.start().req(index).finish()) {
      // Return the argument at the index specified.
      auto wrapped = wrap_subscript(index, nvargs);
      if(wrapped.subscript >= nvargs) {
        ASTERIA_DEBUG_LOG("Variadic argument index is out of range: index = ", index, ", nvarg = ", nvargs);
        self_io = Reference_Root::S_undefined();
        return;
      }
      self_io = this->get_argument(wrapped.subscript);
      return;
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }

void Variadic_Arguer::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    rocket::for_each(this->m_vargs, [&](const Reference &arg) { arg.enumerate_variables(callback);  });
  }

}
