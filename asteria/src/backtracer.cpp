// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "backtracer.hpp"

namespace Asteria {

Backtracer::Backtracer(const Backtracer &) noexcept
  = default;
Backtracer & Backtracer::operator=(const Backtracer &) noexcept
  = default;
Backtracer::Backtracer(Backtracer &&) noexcept
  = default;
Backtracer & Backtracer::operator=(Backtracer &&) noexcept
  = default;
Backtracer::~Backtracer()
  = default;

void unpack_backtrace_and_rethrow(Bivector<String, Unsigned> &btv_out, const std::exception_ptr &etop)
  {
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

}
