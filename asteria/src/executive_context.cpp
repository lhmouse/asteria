// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "executive_context.hpp"
#include "function_header.hpp"
#include "reference.hpp"
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

const Reference * Executive_context::get_named_reference_opt(const rocket::prehashed_string &name) const
  {
    // Check for overriden references.
    const auto qref = this->m_dict.get_opt(name);
    if(qref) {
      return qref;
    }
    // Deal with pre-defined variables.
    if(name.rdstr().starts_with("__")) {
      // If you add new entries or alter existent entries here, remember to update `Analytic_context` as well.
      if(name == "__file") {
        return &(this->m_file);
      }
      if(name == "__line") {
        return &(this->m_line);
      }
      if(name == "__func") {
        return &(this->m_func);
      }
      if(name == "__this") {
        return &(this->m_self);
      }
      if(name == "__varg") {
        return &(this->m_varg);
      }
    }
    return nullptr;
  }

Reference & Executive_context::mutate_named_reference(const rocket::prehashed_string &name)
  {
    return this->m_dict.mut(name);
  }

    namespace {

    template<typename XvalueT>
      inline void do_set_constant(Reference &ref_out, XvalueT &&value)
      {
        Reference_root::S_constant ref_c = { std::forward<XvalueT>(value) };
        ref_out = std::move(ref_c);
      }

    }

void Executive_context::initialize_for_function(Global_context &global, const Function_header &head, const Shared_function_wrapper *zvarg_opt, Reference self, rocket::cow_vector<Reference> args)
  {
    // Set pre-defined variables.
    do_set_constant(this->m_file, D_string(head.get_file()));
    do_set_constant(this->m_line, D_integer(head.get_line()));
    do_set_constant(this->m_func, D_string(head.get_func()));
    // Set the `this` parameter.
    this->m_self = std::move(self.convert_to_variable(global, true));
    // Set other parameters.
    for(std::size_t i = 0; i < head.get_param_count(); ++i) {
      const auto &name = head.get_param_name(i);
      if(name.empty()) {
        continue;
      }
      if(name.rdstr().starts_with("__")) {
        ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", name, "` is reserved and cannot be used.");
      }
      if(i < args.size()) {
        // There is a corresponding argument. Move it.
        this->m_dict.mut(name) = std::move(args.mut(i).convert_to_variable(global, false));
      } else {
        // There is no corresponding argument. Default to `null`.
        this->m_dict.mut(name) = Reference_root::S_constant();
      }
    }
    // Set the variadic argument getter.
    if(ROCKET_EXPECT(head.get_param_count() >= args.size())) {
      args.clear();
    } else {
      args.erase(args.begin(), args.begin() + static_cast<std::ptrdiff_t>(head.get_param_count()));
    }
    if(ROCKET_EXPECT(args.empty() && zvarg_opt)) {
      // Copy the existent zeroary argument getter.
      do_set_constant(this->m_varg, D_function(*zvarg_opt));
    } else {
      // Create an argument getter from those.
      do_set_constant(this->m_varg, D_function(Variadic_arguer(head.get_location(), std::move(args))));
    }
  }

void Executive_context::dispose_variables(Asteria::Global_context &global) noexcept
  {
    this->m_dict.for_each([&](const rocket::prehashed_string /*name*/, const Reference &ref) { ref.dispose_variable(global); });
    this->m_dict.clear();
  }

}
