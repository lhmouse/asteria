// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "executive_context.hpp"
#include "reference.hpp"
#include "variadic_arguer.hpp"
#include "../utilities.hpp"

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

void Executive_context::initialize_for_function(const Source_location &loc, const rocket::prehashed_string &name, const rocket::cow_vector<rocket::prehashed_string> &params, Reference &&self, rocket::cow_vector<Reference> &&args)
  {
    // Set parameters, which are local variables.
    for(std::size_t i = 0; i < params.size(); ++i) {
      const auto &param = params.at(i);
      if(param.empty()) {
        continue;
      }
      if(param.rdstr().starts_with("__")) {
        ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", param, "` is reserved and cannot be used.");
      }
      if(ROCKET_EXPECT(i < args.size())) {
        this->open_named_reference(param) = std::move(args.mut(i));
      } else {
        this->open_named_reference(param) /*= Reference_root::S_null()*/;
      }
    }
    // Set `this`.
    {
      this->open_named_reference(rocket::cow_string::shallow("__this")) = std::move(self);
    }
    // Set `__file`.
    {
      Reference_root::S_constant ref_c = { D_string(loc.get_file()) };
      this->open_named_reference(rocket::cow_string::shallow("__file")) = std::move(ref_c);
    }
    // Set `__line`.
    {
      Reference_root::S_constant ref_c = { D_integer(loc.get_line()) };
      this->open_named_reference(rocket::cow_string::shallow("__line")) = std::move(ref_c);
    }
    // Set `__func`.
    {
      Reference_root::S_constant ref_c = { D_string(name) };
      this->open_named_reference(rocket::cow_string::shallow("__func")) = std::move(ref_c);
    }
    // Set `__varg`.
    {
      args.erase(args.begin(), args.begin() + static_cast<std::ptrdiff_t>(params.size()));
      Reference_root::S_constant ref_c = { D_function(Variadic_arguer(loc, name, std::move(args))) };
      this->open_named_reference(rocket::cow_string::shallow("__varg")) = std::move(ref_c);
    }
  }

}
