// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "executive_context.hpp"
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

namespace {

  template<typename XvalueT>
    inline Reference do_make_constant(XvalueT &&value)
      {
        Reference_root::S_constant ref_c = { std::forward<XvalueT>(value) };
        return std::move(ref_c);
      }

}

const Reference * Executive_context::get_named_reference_opt(const String &name) const
  {
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
    // Call the overriden function with anything unhandled.
    return Abstract_context::get_named_reference_opt(name);
  }

void Executive_context::initialize_for_function(Global_context &global, String file, Uint32 line, String func, const Vector<String> &params, Reference self, Vector<Reference> args)
  {
    // Materialie parameters.
    for(const auto &param : params) {
      Reference arg;
      if(!args.empty()) {
        arg = std::move(args.mut_front());
        args.erase(args.begin());
      }
      if(!param.empty()) {
        if(this->is_name_reserved(param)) {
          ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", param, "` is reserved and cannot be used.");
        }
        this->set_named_reference(param, std::move(arg.convert_to_variable(global)));
      }
    }
    // Set pre-defined variables.
    this->m_file = do_make_constant(D_string(std::move(file)));
    this->m_line = do_make_constant(D_integer(line));
    this->m_func = do_make_constant(D_string(std::move(func)));
    this->m_self = std::move(self);
    this->m_varg = do_make_constant(D_function(Variadic_arguer(file, line, std::move(args))));
  }

}
