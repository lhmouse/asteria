// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variadic_arguer.hpp"
#include "../library/argument_sentry.hpp"
#include "../utilities.hpp"

namespace Asteria {

Variadic_arguer::~Variadic_arguer()
  {
  }

rocket::cow_string Variadic_arguer::describe() const
  {
    return ASTERIA_FORMAT_STRING("variadic argument accessor for `", this->m_name, "()` at \'", this->m_loc, "\'");
  }

void Variadic_arguer::invoke(Reference &self_io, Global_context & /*global*/, rocket::cow_vector<Reference> &&args) const
  {
    static constexpr std::initializer_list<const char *> overload_list =
      {
        "__varg()",
        "__varg(integer)",
      };
    Argument_sentry sentry(std::ref("<builtin>.__varg"));
    // `__varg()`:
    //   Return the number of variadic arguments.
    sentry.reset(args);
    if(sentry.cut()) {
      Reference_root::S_constant ref_c = { D_integer(this->get_varg_size()) };
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
        self_io = Reference_root::S_null();
        return;
      }
      self_io = this->get_varg(static_cast<std::size_t>(wrap.index));
      return;
    }
    // Fail.
    sentry.throw_no_matching_function_call(overload_list);
  }

void Variadic_arguer::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    for(const auto &varg : this->m_vargs) {
      varg.enumerate_variables(callback);
    }
  }

}
