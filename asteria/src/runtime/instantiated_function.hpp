// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_

#include "../fwd.hpp"
#include "abstract_function.hpp"
#include "variadic_arguer.hpp"
#include "../syntax/block.hpp"
#include "../rocket/refcounted_object.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Instantiated_Function : public Abstract_Function
  {
  private:
    rocket::refcounted_object<Variadic_Arguer> m_zvarg;
    rocket::cow_vector<rocket::prehashed_string> m_params;
    Block m_body_bnd;

  public:
    Instantiated_Function(const Source_Location &loc, const rocket::prehashed_string &name, const rocket::cow_vector<rocket::prehashed_string> &params, Block body_bnd)
      : m_zvarg(loc, name), m_params(params), m_body_bnd(std::move(body_bnd))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Instantiated_Function);

  public:
    rocket::cow_string describe() const override;
    void invoke(Reference &self_io, Global_Context &global, rocket::cow_vector<Reference> &&args) const override;
    void enumerate_variables(const Abstract_Variable_Callback &callback) const override;
  };

}

#endif
