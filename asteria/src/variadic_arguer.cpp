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
    return ASTERIA_FORMAT_STRING("variadic argument accessor at \'", m_file, ':', m_line, "\'");
  }

Reference Variadic_arguer::invoke(Reference /*self*/, Vector<Reference> args) const
  {
    const auto nvarg = m_vargs.size();
    switch(args.size()) {
    case 1:
      {
        const auto ivalue = read_reference(args.at(0));
        const auto qindex = ivalue.opt<D_integer>();
        if(!qindex) {
          ASTERIA_THROW_RUNTIME_ERROR("The argument passed to a variadic argument accessor must be of type `integer`.");
        }
        // Return the argument at the given index.
        auto rindex = *qindex;
        if(rindex < 0) {
          // Wrap negative indices.
          rindex += static_cast<Signed>(nvarg);
        }
        if(rindex < 0) {
          ASTERIA_DEBUG_LOG("Variadic argument index fell before the front: index = ", *qindex, ", nvarg = ", nvarg);
          return { };
        }
        if(rindex >= static_cast<Signed>(nvarg)) {
          ASTERIA_DEBUG_LOG("Variadic argument index fell after the back: index = ", *qindex, ", nvarg = ", nvarg);
          return { };
        }
        return m_vargs.at(static_cast<std::size_t>(rindex));
      }
    case 0:
      // Return the number of variadic arguments.
      return reference_constant(D_integer(nvarg));
    default:
      ASTERIA_THROW_RUNTIME_ERROR("A variadic argument accessor takes no more than one argument.");
    }
  }

}
