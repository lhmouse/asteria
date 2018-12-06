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

const Reference * Executive_context::do_get_predefined_reference_opt(const rocket::prehashed_string &name) const
  {
    if(!name.rdstr().starts_with("__")) {
      return nullptr;
    }
    // If you add new entries or alter existent entries here, remember to update `Executive_context` as well.
    if(name == "__this") {
      return &(this->m_self);
    }
    if(name == "__file") {
      return &(this->m_file);
    }
    if(name == "__line") {
      return &(this->m_line);
    }
    if(name == "__func") {
      return &(this->m_func);
    }
    if(name == "__varg") {
      return &(this->m_varg);
    }
    return nullptr;
  }

const Reference * Executive_context::get_named_reference_opt(const rocket::prehashed_string &name) const
  {
    auto qref = this->m_dict.get_opt(name);
    if(ROCKET_UNEXPECT(!qref)) {
      qref = this->do_get_predefined_reference_opt(name);
    }
    return qref;
  }

Reference & Executive_context::mutate_named_reference(const rocket::prehashed_string &name)
  {
    return this->m_dict.mut(name);
  }

void Executive_context::initialize_for_function(const Source_location &loc, const rocket::prehashed_string &name, const rocket::cow_vector<rocket::prehashed_string> &params, const Shared_function_wrapper *zvarg_opt, Reference &&self, rocket::cow_vector<Reference> &&args)
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
      if(i < args.size()) {
        // There is a corresponding argument. Move it.
        this->m_dict.mut(param) = std::move(args.mut(i));
      } else {
        // There is no corresponding argument. Default to `null`.
        this->m_dict.mut(param) = Reference_root::S_null();
      }
    }
    // Set `this`.
    this->m_self = std::move(self);
    // Set `__file`.
    Reference_root::S_constant ref_c = { D_string(loc.get_file()) };
    this->m_file = std::move(ref_c);
    // Set `__line`.
    ref_c.source = D_integer(loc.get_line());
    this->m_line = std::move(ref_c);
    // Set `__func`.
    ref_c.source = D_string(name);
    this->m_func = std::move(ref_c);
    // Set `__varg`.
    if(ROCKET_EXPECT(params.size() >= args.size())) {
      args.clear();
    } else {
      args.erase(args.begin(), args.begin() + static_cast<std::ptrdiff_t>(params.size()));
    }
    if(ROCKET_EXPECT(args.empty() && zvarg_opt)) {
      // Copy the existent zeroary argument getter.
      ref_c.source = D_function(*zvarg_opt);
    } else {
      // Create an argument getter from those.
      ref_c.source = D_function(Variadic_arguer(loc, name, std::move(args)));
    }
    this->m_varg = std::move(ref_c);
  }

void Executive_context::dispose(Asteria::Global_context &global) noexcept
  {
    // Wipe out local variables.
    this->m_dict.for_each([&](const rocket::prehashed_string /*name*/, const Reference &ref) { ref.dispose_variable(global); });
    this->m_dict.clear();
    // Wipe out `this`.
    this->m_self.dispose_variable(global);
    this->m_self = Reference_root::S_null();
    // Wipe out `__file`.
    this->m_file = Reference_root::S_null();
    // Wipe out `__line`.
    this->m_line = Reference_root::S_null();
    // Wipe out `__func`.
    this->m_func = Reference_root::S_null();
    // Wipe out `__varg`.
    this->m_varg.dispose_variable(global);
    this->m_varg = Reference_root::S_null();
  }

}
