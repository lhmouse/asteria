// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variadic_arguer.hpp"
#include "../utilities.hpp"

namespace Asteria {

Variadic_arguer::~Variadic_arguer()
  {
  }

rocket::cow_string Variadic_arguer::describe() const
  {
    return ASTERIA_FORMAT_STRING("variadic argument accessor at \'", this->m_loc, "\'");
  }

void Variadic_arguer::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    for(const auto &varg : this->m_vargs) {
      varg.enumerate_variables(callback);
    }
  }

void Variadic_arguer::invoke(Reference &result_out, Global_context & /*global*/, Reference && /*self*/, rocket::cow_vector<Reference> &&args) const
  {
    switch(args.size()) {
      case 0: {
        // Return the number of variadic arguments.
        Reference_root::S_constant ref_c = { D_integer(this->m_vargs.size()) };
        result_out = std::move(ref_c);
        return;
      }
      case 1: {
        // Return the argument at the index specified.
        const auto ivalue = args.at(0).read();
        const auto qindex = ivalue.opt<D_integer>();
        if(!qindex) {
          ASTERIA_THROW_RUNTIME_ERROR("The argument passed to a variadic argument accessor must be of type `integer`.");
        }
        // Return the argument at the given index.
        auto wrap = wrap_index(*qindex, this->m_vargs.size());
        if(wrap.index >= this->m_vargs.size()) {
          ASTERIA_DEBUG_LOG("Variadic argument index is out of range: index = ", *qindex, ", nvarg = ", this->m_vargs.size());
          result_out = Reference_root::S_null();
          return;
        }
        result_out = this->m_vargs.at(static_cast<std::size_t>(wrap.index));
        return;
      }
      default: {
        ASTERIA_THROW_RUNTIME_ERROR("A variadic argument accessor takes no more than one argument.");
      }
    }
  }

}
