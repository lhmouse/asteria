// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "../syntax/statement.hpp"
#include "../utilities.hpp"

namespace Asteria {

Air_Node::Status Air_Node::execute(Evaluation_Stack& stack, Executive_Context& ctx, const Cow_String& func, const Global_Context& global) const
  {
    return (*(this->m_fptr))(stack, ctx, this->m_params, func, global);
  }

void Air_Node::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    rocket::for_each(this->m_params,
      [&](const Parameter& param)
        {
          switch(static_cast<Index>(param.index())) {
          case index_int64:
          case index_string:
          case index_vector_string:
          case index_source_location:
          case index_vector_statement:
            {
              return;
            }
          case index_value:
            {
              return param.as<Value>().enumerate_variables(callback);
            }
          case index_reference:
            {
              return param.as<Reference>().enumerate_variables(callback);
            }
          case index_vector_air_node:
            {
              return rocket::for_each(param.as<Cow_Vector<Air_Node>>(), [&](const Air_Node& node) { node.enumerate_variables(callback);  });
            }
          case index_compiler_options:
            {
              return;
            }
          default:
            ASTERIA_TERMINATE("An unknown AIR node argument enumeration `", param.index(), "` has been encountered.");
          }
        }
      );
  }

}  // namespace Asteria
