// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "instantiated_function.hpp"
#include "reference.hpp"
#include "executive_context.hpp"
#include "statement.hpp"
#include "utilities.hpp"

namespace Asteria {

Instantiated_function::~Instantiated_function()
  {
  }

String Instantiated_function::describe() const
  {
    Formatter fmt;
    ASTERIA_FORMAT(fmt, "function `", this->m_name, "(");
    auto it = this->m_params.begin();
    while(this->m_params.end() - it > 1) {
      ASTERIA_FORMAT(fmt, *it, ", ");
      ++it;
    }
    if(it != this->m_params.end()) {
      ASTERIA_FORMAT(fmt, *it);
      ++it;
    }
    ASTERIA_FORMAT(fmt, ")` @@ ", this->m_file, ':', this->m_line);
    return fmt.get_stream().extract_string();
  }

void Instantiated_function::collect_variables(bool (*callback)(void *, const rocket::refcounted_ptr<Variable> &), void *param) const
  {
    this->m_body.collect_variables(callback, param);
  }

Reference Instantiated_function::invoke(Global_context &global, Reference self, Vector<Reference> args) const
  {
    return this->m_body.execute_as_function(global, this->m_file, this->m_line, this->m_name, this->m_params, std::move(self), std::move(args));
  }

}
