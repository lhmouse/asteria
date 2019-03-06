// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "function_executive_context.hpp"
#include "variadic_arguer.hpp"
#include "../utilities.hpp"

namespace Asteria {

Function_Executive_Context::~Function_Executive_Context()
  {
  }

    namespace {

    template<typename XcontainerT, typename XnameT, typename XvalueT> void do_predefine_constant(XcontainerT &predefs_out, XnameT &&xname, XvalueT &&xvalue)
      {
        Reference_Root::S_constant ref_c = { std::forward<XvalueT>(xvalue) };
        predefs_out.emplace_back(std::forward<XnameT>(xname), rocket::move(ref_c));
      }

    template<typename XcontainerT, typename XnameT, typename XrefT> void do_predefine_reference(XcontainerT &predefs_out, XnameT &&xname, XrefT &&xref)
      {
        predefs_out.emplace_back(std::forward<XnameT>(xname), std::forward<XrefT>(xref));
      }

    inline Rcobj<Abstract_Function> do_make_varg(const Rcobj<Variadic_Arguer> &zvarg, Cow_Vector<Reference> &&args)
      {
        if(ROCKET_EXPECT(args.empty())) {
          return zvarg;
        }
        return Rcobj<Variadic_Arguer>(zvarg.get(), rocket::move(args));
      }

    }

void Function_Executive_Context::do_set_arguments(const Rcobj<Variadic_Arguer> &zvarg, const Cow_Vector<PreHashed_String> &params, Reference &&self, Cow_Vector<Reference> &&args)
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
      if(ROCKET_UNEXPECT(i >= args.size())) {
        this->open_named_reference(param) = Reference_Root::S_null();
      } else {
        this->open_named_reference(param) = rocket::move(args.mut(i));
      }
    }
    if(ROCKET_UNEXPECT(params.size() < args.size())) {
      args.erase(args.begin(), args.begin() + static_cast<std::ptrdiff_t>(params.size()));
    } else {
      args.clear();
    }
    // Set pre-defined variables.
    // N.B. You must keep these elements sorted.
    // N.B. If you have ever changed these, remember to update 'analytic_executive_context.cpp' as well.
    do_predefine_constant(this->m_predef_refs, rocket::sref("__file"), D_string(zvarg->get_source_file()));
    do_predefine_constant(this->m_predef_refs, rocket::sref("__func"), D_string(zvarg->get_function_signature()));
    do_predefine_constant(this->m_predef_refs, rocket::sref("__line"), D_integer(zvarg->get_source_line()));
    do_predefine_reference(this->m_predef_refs, rocket::sref("__this"), rocket::move(self));
    do_predefine_constant(this->m_predef_refs, rocket::sref("__varg"), D_function(do_make_varg(zvarg, rocket::move(args))));
    // Set up them.
    this->do_set_named_reference_templates(this->m_predef_refs.data(), this->m_predef_refs.size());
  }

}
