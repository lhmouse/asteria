// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_hooks.hpp"
#include "../utils.hpp"
namespace asteria {

void
Abstract_Hooks::
on_trap(const Source_Location& sloc, Executive_Context& ctx)
  {
    (void) sloc;
    (void) ctx;
  }

void
Abstract_Hooks::
on_call(const Source_Location& sloc, const cow_function& target)
  {
    (void) sloc;
    (void) target;
  }

void
Abstract_Hooks::
on_declare(const Source_Location& sloc, const phsh_string& name)
  {
    (void) sloc;
    (void) name;
  }

void
Abstract_Hooks::
on_return(const Source_Location& sloc, PTC_Aware ptc)
  {
    (void) sloc;
    (void) ptc;
  }

void
Abstract_Hooks::
on_throw(const Source_Location& sloc, Value& except)
  {
    (void) sloc;
    (void) except;
  }

void
Abstract_Hooks::
on_function_enter(Executive_Context& fctx, const Instantiated_Function& func)
  {
    (void) fctx;
    (void) func;
  }

void
Abstract_Hooks::
on_function_leave(Executive_Context& fctx) noexcept
  {
    (void) fctx;
  }

}  // namespace asteria
