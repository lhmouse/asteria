// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variadic_arguer.hpp"
#include "../library/argument_sentry.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Variadic_Arguer::describe(std::ostream &os) const
  {
    os << "<builtin>.__varg() @ " << this->m_loc;
  }

void Variadic_Arguer::invoke(Reference &self_io, Global_Context & /*global*/, Cow_Vector<Reference> &&args) const
  {
    Argument_Sentry sentry(rocket::sref("<builtin>.__varg"), args);
    // `__varg()`:
    if(sentry.reset().cut()) {
      // Return the number of variadic arguments.
      Reference_Root::S_constant ref_c = { D_integer(this->get_varg_size()) };
      self_io = std::move(ref_c);
      return;
    }
    // `__varg(integer)`:
    D_integer index;
    if(sentry.reset().req(index).cut()) {
      // Return the argument at the index specified.
      auto wrap = wrap_index(index, this->get_varg_size());
      if(wrap.index >= this->get_varg_size()) {
        ASTERIA_DEBUG_LOG("Variadic argument index is out of range: index = ", index, ", nvarg = ", this->get_varg_size());
        self_io = Reference_Root::S_null();
        return;
      }
      self_io = this->get_varg(static_cast<std::size_t>(wrap.index));
      return;
    }
    // Fail.
    static constexpr Argument_Sentry::Overload_Parameter s_overloads[] =
      {
        // `__varg()`:
        0,
        // `__varg(integer)`:
        1, type_integer,
      };
    sentry.throw_no_matching_function_call(s_overloads, rocket::countof(s_overloads));
  }

void Variadic_Arguer::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    for(const auto &varg : this->m_vargs) {
      varg.enumerate_variables(callback);
    }
  }

}
