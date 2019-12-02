// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_hooks.hpp"
#include "../utilities.hpp"

namespace Asteria {

Abstract_Hooks::~Abstract_Hooks()
  {
  }

void Abstract_Hooks::on_variable_declare(const Source_Location& /*sloc*/, const phsh_string& /*inside*/,
                                         const phsh_string& /*name*/)
  {
  }

void Abstract_Hooks::on_function_call(const Source_Location& /*sloc*/, const phsh_string& /*inside*/,
                                      const ckptr<Abstract_Function>& /*target*/)
  {
  }

void Abstract_Hooks::on_function_return(const Source_Location& /*sloc*/, const phsh_string& /*inside*/,
                                        const Reference& /*result*/)
  {
  }

void Abstract_Hooks::on_function_except(const Source_Location& /*sloc*/, const phsh_string& /*inside*/,
                                        const Runtime_Error& /*except*/)
  {
  }

}  // namespace Asteria
