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

void Executive_context::initialize_for_function(Global_context &global, String file, Uint32 line, String func, const Vector<String> &params, Reference self, Vector<Reference> args)
  {
    // Set up parameters.
    auto eparg = args.mut_begin();
    for(const auto &param : params) {
      Reference ref;
      if(eparg != args.end()) {
        ref = std::move(*eparg);
        ++eparg;
      }
      if(param.empty()) {
        continue;
      }
      if(this->is_name_reserved(param)) {
        ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", param, "` is reserved and cannot be used.");
      }
      this->set_named_reference(param, std::move(ref.convert_to_variable(global)));
    }
    args.erase(args.begin(), eparg);
    // Set up system variables.
    this->set_named_reference(String::shallow("__file"), do_make_constant(D_string(std::move(file))));
    this->set_named_reference(String::shallow("__line"), do_make_constant(D_integer(line)));
    this->set_named_reference(String::shallow("__func"), do_make_constant(D_string(std::move(func))));
    this->set_named_reference(String::shallow("__this"), std::move(self.convert_to_variable(global)));
    this->set_named_reference(String::shallow("__varg"), do_make_constant(D_function(Variadic_arguer(file, line, std::move(args)))));
  }

}
