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

    namespace {

    template<typename XvalueT, typename... ParamsT> Reference_Root::S_constant do_make_constant(ParamsT&&... params)
      {
        Reference_Root::S_constant ref_c = { XvalueT(rocket::forward<ParamsT>(params)...) };
        return ref_c;
      }

    }  // namespace

void Executive_Context::prepare_function_arguments(const Rcobj<Variadic_Arguer>& zvarg, const Cow_Vector<PreHashed_String>& params, Reference&& self, Cow_Vector<Reference>&& args)
  {
    // Set parameters, which are local references.
    for(std::size_t i = 0; i < params.size(); ++i) {
      const auto& param = params.at(i);
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
    // Set pre-defined references.
    // N.B. If you have ever changed these, remember to update 'analytic_context.cpp' as well.
    if(ROCKET_UNEXPECT(params.size() < args.size())) {
      this->open_named_reference(rocket::sref("__varg")) = do_make_constant<D_function>(Rcobj<Variadic_Arguer>(zvarg.get(), rocket::move(args.erase(0, params.size()))));
    } else {
      this->open_named_reference(rocket::sref("__varg")) = do_make_constant<D_function>(zvarg);
    }
    this->open_named_reference(rocket::sref("__this")) = rocket::move(self);
    this->open_named_reference(rocket::sref("__func")) = do_make_constant<D_string>(zvarg->get_function_signature());
  }

}  // namespace Asteria
