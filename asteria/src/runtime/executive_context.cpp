// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "executive_context.hpp"
#include "variadic_arguer.hpp"
#include "../utilities.hpp"

namespace Asteria {

Executive_Context::~Executive_Context()
  {
  }

void Executive_Context::do_prepare_function(const cow_vector<phsh_string>& params, Reference&& self, cow_vector<Reference>&& args)
  {
    // This is the subscript of the special pameter placeholder `...`.
    opt<size_t> qelps;
    // Set parameters, which are local references.
    for(size_t i = 0; i < params.size(); ++i) {
      const auto& param = params.at(i);
      if(param.empty()) {
        continue;
      }
      if(param == "...") {
        // Nothing is set for the parameter placeholder, but the parameter list terminates here.
        ROCKET_ASSERT(i == params.size() - 1);
        qelps = i;
        break;
      }
      if(param.rdstr().starts_with("__")) {
        ASTERIA_THROW("The function parameter name `", param, "` is reserved and cannot be used.");
      }
      // Set the parameter.
      if(ROCKET_UNEXPECT(i >= args.size()))
        this->open_named_reference(param) = Reference_Root::S_null();
      else
        this->open_named_reference(param) = rocket::move(args.mut(i));
    }
    if(!qelps && (args.size() > params.size())) {
      // Disallow exceess arguments if the function is not variadic.
      ASTERIA_THROW("Too many arguments were provided (expecting no more than `", params.size(), "`, but got `", args.size(), "`).");
    }
    // Prepare `__this` and `__varg`. This is tricky.
    args.erase(0, rocket::min(qelps.value_or(SIZE_MAX), args.size()));
    args.emplace_back(rocket::move(self));
    this->m_args_self = rocket::move(args);
  }

bool Executive_Context::do_is_analytic() const noexcept
  {
    return this->is_analytic();
  }

const Abstract_Context* Executive_Context::do_get_parent_opt() const noexcept
  {
    return this->get_parent_opt();
  }

Reference* Executive_Context::do_lazy_lookup_opt(Reference_Dictionary& named_refs, const phsh_string& name) const
  {
    // Create pre-defined references as needed.
    // N.B. If you have ever changed these, remember to update 'analytic_context.cpp' as well.
    if(name == "__func") {
      auto& func = named_refs.open(rocket::sref("__func"));
      // Create a constant string of the function signature.
      Reference_Root::S_constant xref = { G_string(this->m_zvarg->func()) };
      func = rocket::move(xref);
      return &func;
    }
    if((name == "__this") || (name == "__varg")) {
      auto& self = named_refs.open(rocket::sref("__this"));
      auto& varg = named_refs.open(rocket::sref("__varg"));
      // Unpack the `this` reference.
      ROCKET_ASSERT(!this->m_args_self.empty());
      self = rocket::move(this->m_args_self.mut_back());
      this->m_args_self.pop_back();
      // Initialize the variadic argument getter.
      rcptr<Variadic_Arguer> kvarg;
      if(this->m_args_self.empty()) {
        // Reference the pre-allocated zero-ary argument getter if there are variadic arguments.
        this->m_zvarg->add_reference();
        kvarg.reset(this->m_zvarg.ptr());
      }
      else {
        // Create a new argument getter otherwise.
        kvarg = rocket::make_refcnt<Variadic_Arguer>(this->m_zvarg, rocket::move(this->m_args_self));
        this->m_args_self.clear();
      }
      // Set it.
      Reference_Root::S_constant xref = { G_function(rocket::move(kvarg)) };
      varg = rocket::move(xref);
      // Return a pointer to the reference requested.
      return (name[2] == 't') ? &self : &varg;
    }
    return nullptr;
  }

}  // namespace Asteria
