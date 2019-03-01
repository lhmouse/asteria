// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "reference.hpp"
#include "../utilities.hpp"

namespace Asteria {

Air_Node::Status Air_Node::execute(Reference_Stack &stack_io, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global) const
  {
    return (*(this->m_fptr))(stack_io, ctx_io, this->m_opaque, func, global);
  }

void Air_Node::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    rocket::for_each(this->m_refs, [&](const Reference &ref) { ref.enumerate_variables(callback);  });
  }

}
