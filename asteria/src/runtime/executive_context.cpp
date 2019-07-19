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
        Reference_Root::S_constant xref = { XvalueT(rocket::forward<ParamsT>(params)...) };
        return xref;
      }

    }  // namespace

void Executive_Context::do_prepare_function(const Cow_Vector<PreHashed_String>& params, Reference&& self, Cow_Vector<Reference>&& args)
  {
    // This is the subscript of the special pameter placeholder `...`.
    Opt<std::size_t> qelps;
    // Set parameters, which are local references.
    for(std::size_t i = 0; i < params.size(); ++i) {
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
        ASTERIA_THROW_RUNTIME_ERROR("The function parameter name `", param, "` is reserved and cannot be used.");
      }
      // Set the parameter.
      if(ROCKET_EXPECT(i < args.size())) {
        this->open_named_reference(param) = rocket::move(args.mut(i));
      }
      else {
        this->open_named_reference(param) = Reference_Root::S_null();
      }
    }
    // Set pre-defined references.
    // N.B. If you have ever changed these, remember to update 'analytic_context.cpp' as well.
    if(qelps) {
      if(args.size() > *qelps) {
        // Erase named arguments as well as the ellipsis.
        args.erase(0, *qelps);
        // Create a new argument getter.
        this->open_named_reference(rocket::sref("__varg")) = do_make_constant<G_function>(Rcobj<Variadic_Arguer>(this->m_zvarg.get(), rocket::move(args)));
      }
      else {
        // Reference the pre-allocated zero-ary argument getter.
        this->open_named_reference(rocket::sref("__varg")) = do_make_constant<G_function>(this->m_zvarg);
      }
    }
    else {
      if(args.size() > params.size()) {
        // Disallow exceess arguments if the function is not variadic.
        ASTERIA_THROW_RUNTIME_ERROR("Too many arguments were provided (expecting no more than `", params.size(), "`, "
                                    "but got `", args.size(), "`).");
      }
      else {
        // Reference the pre-allocated zero-ary argument getter.
        this->open_named_reference(rocket::sref("__varg")) = do_make_constant<G_function>(this->m_zvarg);
      }
    }
    this->open_named_reference(rocket::sref("__this")) = rocket::move(self);
    this->open_named_reference(rocket::sref("__func")) = do_make_constant<G_string>(this->m_zvarg->get_function_signature());
  }

}  // namespace Asteria
