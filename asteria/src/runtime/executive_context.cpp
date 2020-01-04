// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "executive_context.hpp"
#include "../utilities.hpp"

namespace Asteria {

Executive_Context::~Executive_Context()
  {
  }

void Executive_Context::do_prepare_function(const cow_vector<phsh_string>& params,
                                            Reference&& self, cow_vector<Reference>&& args)
  {
    // This is the subscript of the special parameter placeholder `...`.
    size_t elps = SIZE_MAX;
    // Set parameters, which are local references.
    for(size_t i = 0; i < params.size(); ++i) {
      const auto& name = params.at(i);
      if(name.empty()) {
        continue;
      }
      if(name == "...") {
        // Nothing is set for the parameter placeholder, but the parameter list terminates here.
        ROCKET_ASSERT(i == params.size() - 1);
        elps = i;
        break;
      }
      if(name.rdstr().starts_with("__")) {
        ASTERIA_THROW("reserved name not declarable as parameter (name `$1`)", name);
      }
      // Set the parameter.
      if(ROCKET_UNEXPECT(i >= args.size()))
        this->open_named_reference(name) = Reference_Root::S_void();
      else
        this->open_named_reference(name) = ::rocket::move(args.mut(i));
    }
    if((elps == SIZE_MAX) && (args.size() > params.size())) {
      // Disallow exceess arguments if the function is not variadic.
      ASTERIA_THROW("too many arguments (`$1` > `$2`)", args.size(), params.size());
    }
    args.erase(0, elps);

    // Stash arguments for lazy initialization.
    this->m_self_opt = ::rocket::move(self);
    this->m_args_opt = ::rocket::move(args);
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
      auto& ref = named_refs.open(::rocket::sref("__func"));
      Reference_Root::S_constant xref = { this->m_zvarg->func() };
      ref = ::rocket::move(xref);
      return &ref;
    }
    if(name == "__this") {
      auto& ref = named_refs.open(::rocket::sref("__this"));
      ref = ::rocket::move(*(this->m_self_opt));
      this->m_self_opt.reset();
      return &ref;
    }
    if(name == "__varg") {
      auto& ref = named_refs.open(::rocket::sref("__varg"));
      ckptr<Abstract_Function> varg;
      if(ROCKET_EXPECT(this->m_args_opt->empty()))
        varg = this->m_zvarg;
      else
        varg = ::rocket::make_refcnt<Variadic_Arguer>(*(this->m_zvarg),  // copy `sloc` and `func`
                                                      ::rocket::move(*(this->m_args_opt)));
      this->m_args_opt.reset();
      Reference_Root::S_constant xref = { ::rocket::move(varg) };
      ref = ::rocket::move(xref);
      return &ref;
    }
    return nullptr;
  }

}  // namespace Asteria
