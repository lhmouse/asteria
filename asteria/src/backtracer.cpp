// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "backtracer.hpp"

namespace Asteria {

void Backtracer::unpack_backtrace_and_rethrow(Bivector<String, Unsigned> &btv_out, const std::exception_ptr &etop)
  {
    // Rethrow the exception. If `Backtracer` is caught, a new element is appended to `btv_out` and the nested
    // exception is rethrown. This procedure is repeated until something other than `Backtracer` is thrown.
    auto eptr = etop;
    do {
      try {
        std::rethrow_exception(eptr);
      } catch(Backtracer &e) {
        btv_out.emplace_back(e.get_file(), e.get_line());
        eptr = e.nested_ptr();
      }
    } while(true);
  }

Backtracer::~Backtracer()
  {
  }

}
