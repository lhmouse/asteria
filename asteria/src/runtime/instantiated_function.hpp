// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_

#include "../fwd.hpp"
#include "abstract_function.hpp"
#include "variadic_arguer.hpp"
#include "../syntax/block.hpp"

namespace Asteria {

class Instantiated_Function : public Abstract_Function
  {
  private:
    RefCnt_Object<Variadic_Arguer> m_zvarg;
    Cow_Vector<PreHashed_String> m_params;
    Block m_body_bnd;

  public:
    Instantiated_Function(const Source_Location &loc, const PreHashed_String &name, const Cow_Vector<PreHashed_String> &params, Block body_bnd)
      : m_zvarg(loc, name), m_params(params), m_body_bnd(std::move(body_bnd))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Instantiated_Function);

  public:
    Cow_String describe() const override;
    void invoke(Reference &self_io, Global_Context &global, Cow_Vector<Reference> &&args) const override;
    void enumerate_variables(const Abstract_Variable_Callback &callback) const override;
  };

}

#endif
