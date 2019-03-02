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
    rocket::for_each(this->m_opaque.vars,
      [&](const Variant &var)
        {
          switch(static_cast<Index>(var.index())) {
          case index_string:
            {
              return;
            }
          case index_value:
            {
              const auto &alt = var.as<Value>();
              alt.enumerate_variables(callback);
              return;
            }
          case index_reference:
            {
              const auto &alt = var.as<Reference>();
              alt.enumerate_variables(callback);
              return;
            }
          case index_nodes:
            {
              const auto &alt = var.as<Cow_Vector<Air_Node>>();
              rocket::for_each(alt, [&](const Air_Node &node) { node.enumerate_variables(callback);  });
              return;
            }
          default:
            ASTERIA_TERMINATE("An unknown AIR node argument enumeration `", var.index(), "` has been encountered.");
          }
        }
      );
  }

}
