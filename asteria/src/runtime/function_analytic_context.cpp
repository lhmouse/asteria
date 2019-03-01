// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "function_analytic_context.hpp"
#include "variadic_arguer.hpp"
#include "../utilities.hpp"

namespace Asteria {

Function_Analytic_Context::~Function_Analytic_Context()
  {
  }

    namespace {

    template<typename XcontainerT, typename XnameT> void do_predefine(XcontainerT &predefs_out, XnameT &&xname)
      {
        predefs_out.emplace_back(std::forward<XnameT>(xname), Reference_Root::S_undefined());
      }

    }

void Function_Analytic_Context::do_set_parameters(const Cow_Vector<PreHashed_String> &params)
  {
    // Set parameters, which are local variables.
    for(const auto &param : params) {
      if(param.empty()) {
        continue;
      }
      if(param.rdstr().starts_with("__")) {
        ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", param, "` is reserved and cannot be used.");
      }
      // Its contents are out of interest.
      this->open_named_reference(param) = Reference_Root::S_undefined();
    }
    // Set pre-defined variables.
    // N.B. You must keep these elements sorted.
    // N.B. If you have ever changed these, remember to update 'function_executive_context.cpp' as well.
    do_predefine(this->m_predef_refs,
                 rocket::sref("__file"));
    do_predefine(this->m_predef_refs,
                 rocket::sref("__func"));
    do_predefine(this->m_predef_refs,
                 rocket::sref("__line"));
    do_predefine(this->m_predef_refs,
                 rocket::sref("__this"));
    do_predefine(this->m_predef_refs,
                 rocket::sref("__varg"));
    // Set up them.
    this->do_set_named_reference_templates(this->m_predef_refs.data(), this->m_predef_refs.size());
  }

}
