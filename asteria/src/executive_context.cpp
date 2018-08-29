// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "executive_context.hpp"
#include "variadic_arguer.hpp"
#include "utilities.hpp"

namespace Asteria {

Executive_context::~Executive_context()
  {
  }

bool Executive_context::is_analytic() const noexcept
  {
    return false;
  }
const Executive_context * Executive_context::get_parent_opt() const noexcept
  {
    return this->m_parent_opt;
  }

void Executive_context::initialize_for_function(const Vector<String> &params, const String &file, Unsigned line, Reference self, Vector<Reference> args)
  {
    // Set up parameters.
    for(const auto &name : params) {
      Reference ref;
      if(args.empty() == false) {
        ref = std::move(args.mut_front());
        args.erase(args.begin());
      }
      if(name.empty() == false) {
        if(is_name_reserved(name)) {
          ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", name, "` is reserved and cannot be used.");
        }
        this->set_named_reference(name, std::move(ref.materialize()));
      }
    }
    // Set up system variables.
    this->set_named_reference(String::shallow("__file"), Reference::make_constant(D_string(file)));
    this->set_named_reference(String::shallow("__line"), Reference::make_constant(D_integer(line)));
    this->set_named_reference(String::shallow("__this"), std::move(self.materialize()));
    this->set_named_reference(String::shallow("__varg"), Reference::make_constant(D_function(rocket::make_refcounted<Variadic_arguer>(file, line, std::move(args)))));
  }

}
