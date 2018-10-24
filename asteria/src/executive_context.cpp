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
    const auto qbase = this->Abstract_context::get_named_reference_opt(name);
    if(qbase) {
      return qbase;
    }
    // Deal with pre-defined variables.
    // If you add new entries or alter existent entries here, you must update `Analytic_context` as well.
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
    return nullptr;
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
    this->m_self = std::move(self);
    // Materialie other parameters.
    for(const auto &param : head.get_params()) {
      Reference arg;
      if(!args.empty()) {
        arg = std::move(args.mut_front());
        args.erase(args.begin());
      }
      if(!param.empty()) {
        if(this->is_name_reserved(param)) {
          ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", param, "` is reserved and cannot be used.");
        }
        this->Abstract_context::set_named_reference(param, std::move(arg.convert_to_variable(global)));
      }
    }
    if(args.empty() && zvarg_opt) {
      do_set_constant(this->m_varg, D_function(*zvarg_opt));
    } else {
      do_set_constant(this->m_varg, D_function(Variadic_arguer(head.get_location(), std::move(args))));
    }
  }

}
