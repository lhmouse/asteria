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
    return m_parent_opt;
  }

void initialize_executive_function_context(Executive_context &ctx_out, const Vector<String> &params, const String &file, Unsigned line, Reference self, Vector<Reference> args)
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
        ctx_out.set_named_reference(name, std::move(materialize_reference(ref)));
      }
    }
    // Set up system variables.
    ctx_out.set_named_reference(String::shallow("__file"), reference_constant(D_string(file)));
    ctx_out.set_named_reference(String::shallow("__line"), reference_constant(D_integer(line)));
    ctx_out.set_named_reference(String::shallow("__this"), std::move(materialize_reference(self)));
    ctx_out.set_named_reference(String::shallow("__varg"), reference_constant(D_function(rocket::make_refcounted<Variadic_arguer>(file, line, std::move(args)))));
  }

}
