// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "backtracer.hpp"

namespace Asteria {

void Backtracer::unpack_and_rethrow(Vector<Backtracer> &btv_out, const std::exception_ptr &etop)
  {
    // Rethrow the exception. If `Backtracer` is caught, a new element is appended to `btv_out` and the nested
    // exception is rethrown. This procedure is repeated until something other than `Backtracer` is thrown.
    auto eptr = etop;
    do {
      try {
        std::rethrow_exception(eptr);
      } catch(Backtracer &e) {
        eptr = e.nested_ptr();
        btv_out.emplace_back(std::move(e));
      }
    } while(true);
  }

Backtracer::~Backtracer()
  {
  }

}
