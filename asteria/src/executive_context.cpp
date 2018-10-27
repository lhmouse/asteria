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

const Reference * Executive_context::get_named_reference_opt(const String &name) const
  {
    // Check for overriden references.
    const auto qit = this->m_dict.find(name);
    if(qit != this->m_dict.end()) {
      return &(qit->second);
    }
    // Deal with pre-defined variables.
    if(name.starts_with("__")) {
      // If you add new entries or alter existent entries here, you must update `Executive_context` as well.
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

void Executive_context::set_named_reference(const String &name, Reference ref)
  {
    this->m_dict.insert_or_assign(name, std::move(ref));
  }

    namespace {

    template<typename XvalueT>
      inline void do_set_constant(Reference &ref_out, XvalueT &&value)
      {
        Reference_root::S_constant ref_c = { std::forward<XvalueT>(value) };
        ref_out = std::move(ref_c);
      }

    }

void Executive_context::initialize_for_function(Global_context &global, const Function_header &head, const Shared_function_wrapper *zvarg_opt, Reference self, Vector<Reference> args)
  {
    // Set pre-defined variables.
    do_set_constant(this->m_file, D_string(head.get_file()));
    do_set_constant(this->m_line, D_integer(head.get_line()));
    do_set_constant(this->m_func, D_string(head.get_func()));
    // Set the `this` parameter.
    this->m_self = std::move(self.convert_to_variable(global));
    // Set other parameters.
    for(Size i = 0; i < head.get_param_count(); ++i) {
      const auto &name = head.get_param_name(i);
      if(name.empty()) {
        continue;
      }
      if(name.starts_with("__")) {
        ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", name, "` is reserved and cannot be used.");
      }
      if(i < args.size()) {
        // There is a corresponding argument. Move it.
        this->m_dict.insert_or_assign(name, std::move(args.mut(i)));
      } else {
        // There is no corresponding argument. Default to `null`.
        this->m_dict.insert_or_assign(name, Reference());
      }
    }
    // Set the variadic argument getter.
    if(head.get_param_count() < args.size()) {
      // There are more arguments than parameters. Create an argument getter from those.
      args.erase(args.begin(), args.begin() + static_cast<Diff>(head.get_param_count()));
      do_set_constant(this->m_varg, D_function(Variadic_arguer(head.get_location(), std::move(args))));
    } else if(!zvarg_opt) {
      // There is no variadic arg. Create a zeroary argument getter.
      do_set_constant(this->m_varg, D_function(Variadic_arguer(head.get_location(), { })));
    } else {
      // There is no variadic arg. Copy the existent zeroary argument getter.
      do_set_constant(this->m_varg, D_function(*zvarg_opt));
    }
  }

}
