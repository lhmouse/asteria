// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variadic_arguer.hpp"
#include "../library/argument_reader.hpp"
#include "../utilities.hpp"

namespace Asteria {

std::ostream& Variadic_Arguer::describe(std::ostream& os) const
  {
    return os << "<built-in>.__varg([index]) @ " << this->m_sloc;
  }

Reference& Variadic_Arguer::invoke(Reference& self, const Global_Context& /*global*/, cow_vector<Reference>&& args) const
  {
    Argument_Reader reader(rocket::sref("<built-in>.__varg"), args);
    // Extract arguments.
    opt<G_integer> qindex;
    if(reader.start().g(qindex).finish()) {
      auto nvargs = this->m_vargs.size();
      if(!qindex) {
        // Return the number of variadic arguments if `index` is `null` or absent.
        Reference_Root::S_constant xref = { G_integer(nvargs) };
        return self = rocket::move(xref);
      }
      // Return the argument at `index`.
      auto w = wrap_index(*qindex, nvargs);
      auto nadd = w.nprepend | w.nappend;
      if(nadd != 0) {
        ASTERIA_DEBUG_LOG("Variadic argument index is out of range: index = ", *qindex, ", nvarg = ", nvargs);
        return self = Reference_Root::S_null();
      }
      return self = this->m_vargs.at(w.rindex);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }

Variable_Callback& Variadic_Arguer::enumerate_variables(Variable_Callback& callback) const
  {
    return rocket::for_each(this->m_vargs, [&](const Reference& arg) { arg.enumerate_variables(callback);  }), callback;
  }

}  // namespace Asteria
