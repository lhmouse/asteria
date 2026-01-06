// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_hooks.hpp"
#include "../utils.hpp"
#pragma GCC diagnostic ignored "-Wunused-parameter"
namespace asteria {

void
Abstract_Hooks::
on_trap(const Source_Location& sloc, Executive_Context& ctx)
  {
  }

void
Abstract_Hooks::
on_call(const Source_Location& sloc, const cow_function& target)
  {
  }

void
Abstract_Hooks::
on_declare(const Source_Location& sloc, const phcow_string& name)
  {
  }

void
Abstract_Hooks::
on_return(const Source_Location& sloc, PTC_Aware ptc)
  {
  }

void
Abstract_Hooks::
on_throw(const Source_Location& sloc, Value& except)
  {
  }

void
Abstract_Hooks::
on_function_enter(const Instantiated_Function& func, Executive_Context& fctx)
  {
  }

void
Abstract_Hooks::
on_function_leave(const Instantiated_Function& func, Executive_Context& fctx)
  {
  }

}  // namespace asteria
