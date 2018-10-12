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
    Formatter fmt;
    ASTERIA_FORMAT(fmt, "variadic argument accessor @@ ", this->m_file, ':', this->m_line);
    return fmt.get_stream().extract_string();
  }

void Variadic_arguer::collect_variables(bool (*callback)(void *, const Rcptr<Variable> &), void *param) const
  {
    for(const auto &varg : this->m_vargs) {
      varg.collect_variables(callback, param);
    }
  }

Reference Variadic_arguer::invoke(Global_context & /*global*/, Reference /*self*/, Vector<Reference> args) const
  {
    switch(args.size()) {
      case 0: {
        // Return the number of variadic arguments.
        Reference_root::S_constant ref_c = { D_integer(this->m_vargs.size()) };
        return std::move(ref_c);
      }
      case 1: {
        // Return the argument at the index specified.
        const auto ivalue = args.at(0).read();
        const auto qindex = ivalue.opt<D_integer>();
        if(!qindex) {
          ASTERIA_THROW_RUNTIME_ERROR("The argument passed to a variadic argument accessor must be of type `integer`.");
        }
        // Return the argument at the given index.
        Uint64 bfill, efill;
        auto rindex = wrap_index(bfill, efill, *qindex, this->m_vargs.size());
        if(rindex >= this->m_vargs.size()) {
          ASTERIA_DEBUG_LOG("Variadic argument index is out of range: index = ", *qindex, ", nvarg = ", this->m_vargs.size());
          return { };
        }
        return this->m_vargs.at(static_cast<Size>(rindex));
      }
      default: {
        ASTERIA_THROW_RUNTIME_ERROR("A variadic argument accessor takes no more than one argument.");
      }
    }
  }

}
