// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variadic_arguer.hpp"
#include "../library/argument_sentry.hpp"
#include "../utilities.hpp"

namespace Asteria {

Variadic_Arguer::~Variadic_Arguer()
  {
  }

rocket::cow_string Variadic_Arguer::describe() const
  {
    return ASTERIA_FORMAT_STRING("variadic argument accessor for `", this->m_name, "()` at \'", this->m_loc, "\'");
  }

void Variadic_Arguer::invoke(Reference &self_io, Global_Context & /*global*/, rocket::cow_vector<Reference> &&args) const
  {
    Argument_Sentry sentry(std::ref("<builtin>.__varg"));
    // `__varg()`:
    //   Return the number of variadic arguments.
    sentry.reset(args);
    if(sentry.cut()) {
      Reference_Root::S_constant ref_c = { D_integer(this->get_varg_size()) };
      self_io = std::move(ref_c);
      return;
    }
    // `__varg(number)`:
    //   Return the argument at the index specified.
    sentry.reset(args);
    D_integer index;
    if(sentry.req(index).cut()) {
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
    sentry.throw_no_matching_function_call(
      {
        // Remember to keep this list up-to-date.
        0,
        1, Value::type_integer,
      });
  }

void Variadic_Arguer::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    for(const auto &varg : this->m_vargs) {
      varg.enumerate_variables(callback);
    }
  }

}
