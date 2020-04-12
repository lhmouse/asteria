// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "ptc_arguments.hpp"
#include "reference.hpp"
#include "variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

PTC_Arguments::
~PTC_Arguments()
  {
  }

Variable_Callback&
PTC_Arguments::
enumerate_variables(Variable_Callback& callback)
const
  {
    this->m_target.enumerate_variables(callback);
    ::rocket::for_each(this->m_args_self, callback);
    return callback;
  }

}  // namespace Asteria
