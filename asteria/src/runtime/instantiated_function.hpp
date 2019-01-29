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
    CoW_Vector<PreHashed_String> m_params;
    Block m_body_bnd;

  public:
    Instantiated_Function(const Source_Location &sloc, const PreHashed_String &func, const CoW_Vector<PreHashed_String> &params, Block body_bnd)
      : m_zvarg(sloc, func), m_params(params), m_body_bnd(std::move(body_bnd))
      {
      }
    ~Instantiated_Function() override;

  public:
    const Source_Location & get_source_location() const noexcept
      {
        return this->m_zvarg->get_source_location();
      }
    const PreHashed_String & get_function_name() const noexcept
      {
        return this->m_zvarg->get_function_name();
      }
    const CoW_Vector<PreHashed_String> & get_parameters() const noexcept
      {
        return this->m_params;
      }

    void describe(std::ostream &os) const override;
    void invoke(Reference &self_io, Global_Context &global, CoW_Vector<Reference> &&args) const override;
    void enumerate_variables(const Abstract_Variable_Callback &callback) const override;
  };

}

#endif
