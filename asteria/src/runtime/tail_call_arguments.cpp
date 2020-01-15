// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "tail_call_arguments.hpp"
#include "reference.hpp"
#include "variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

Tail_Call_Arguments::~Tail_Call_Arguments()
  {
  }

Variable_Callback& Tail_Call_Arguments::enumerate_variables(Variable_Callback& callback) const
  {
    this->m_target->enumerate_variables(callback);
    ::rocket::for_each(this->m_args_self, callback);
    return callback;
  }

}  // namespace Asteria
