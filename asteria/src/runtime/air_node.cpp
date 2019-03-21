// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "../syntax/statement.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Air_Node::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    const auto enumerate_variables_recursively = [&](const Param& param)
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
            param.as<Value>().enumerate_variables(callback);
            return;
          }
        case index_reference:
          {
            param.as<Reference>().enumerate_variables(callback);
            return;
          }
        case index_vector_air_node:
          {
            rocket::for_each(param.as<Cow_Vector<Air_Node>>(), [&](const Air_Node& node) { node.enumerate_variables(callback);  });
            return;
          }
        default:
          ASTERIA_TERMINATE("An unknown AIR node argument enumeration `", param.index(), "` has been encountered.");
         }
      };
    rocket::for_each(this->m_params, enumerate_variables_recursively);
  }

}  // namespace Asteria
