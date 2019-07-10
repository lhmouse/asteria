// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "../syntax/statement.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Air_Node::enumerate_variables_of(const Abstract_Variable_Callback& callback, const Air_Node::Parameter& param)
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
    default:
      ASTERIA_TERMINATE("An unknown AIR node argument enumeration `", param.index(), "` has been encountered.");
    }
  }

void Air_Node::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    rocket::for_each(this->m_params, [&](const Parameter& param) { Air_Node::enumerate_variables_of(callback, param);  });
  }

}  // namespace Asteria
