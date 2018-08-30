// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variadic_arguer.hpp"
#include "utilities.hpp"

namespace Asteria {

Variadic_arguer::~Variadic_arguer()
  {
  }

String Variadic_arguer::describe() const
  {
    return ASTERIA_FORMAT_STRING("variadic argument accessor at \'", this->m_file, ':', this->m_line, "\'");
  }

Reference Variadic_arguer::invoke(Reference /*self*/, Vector<Reference> args) const
  {
    const auto nvarg = static_cast<Signed>(this->m_vargs.size());
    switch(args.size()) {
    case 1:
      {
        // Return the argument at the index specified.
        const auto ivalue = args.at(0).read();
        const auto qindex = ivalue.opt<D_integer>();
        if(!qindex) {
          ASTERIA_THROW_RUNTIME_ERROR("The argument passed to a variadic argument accessor must be of type `integer`.");
        }
        // Return the argument at the given index.
        auto rindex = *qindex;
        if(rindex < 0) {
          // Wrap negative indices.
          rindex += nvarg;
        }
        if(rindex < 0) {
          ASTERIA_DEBUG_LOG("Variadic argument index fell before the front: index = ", *qindex, ", nvarg = ", nvarg);
          return { };
        }
        if(rindex >= nvarg) {
          ASTERIA_DEBUG_LOG("Variadic argument index fell after the back: index = ", *qindex, ", nvarg = ", nvarg);
          return { };
        }
        return this->m_vargs.at(static_cast<std::size_t>(rindex));
      }
    case 0:
      {
        // Return the number of variadic arguments.
        Reference_root::S_constant ref_c = { D_integer(nvarg) };
        return std::move(ref_c);
      }
    default:
      ASTERIA_THROW_RUNTIME_ERROR("A variadic argument accessor takes no more than one argument.");
    }
  }

}
