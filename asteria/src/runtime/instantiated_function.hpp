// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_

#include "../fwd.hpp"
#include "abstract_function.hpp"
#include "variadic_arguer.hpp"

namespace Asteria {

class Instantiated_Function : public Abstract_Function
  {
  private:
    // pre-defined constants and parameters
    Rcobj<Variadic_Arguer> m_zvarg;
    Cow_Vector<PreHashed_String> m_params;
    // the function body
    Cow_Vector<Air_Node> m_code;

  public:
    Instantiated_Function(const Source_Location& sloc, const PreHashed_String& func, const Cow_Vector<PreHashed_String>& params,
                          Cow_Vector<Air_Node>&& code)
      : m_zvarg(Variadic_Arguer(sloc, func)), m_params(params),
        m_code(rocket::move(code))
      {
      }
    ~Instantiated_Function() override;

  public:
    const Source_Location& get_source_location() const noexcept
      {
        return this->m_zvarg->get_source_location();
      }
    const PreHashed_String& get_function_signature() const noexcept
      {
        return this->m_zvarg->get_function_signature();
      }
    const Cow_Vector<PreHashed_String>& get_parameters() const noexcept
      {
        return this->m_params;
      }

    std::ostream& describe(std::ostream& os) const override;
    Reference& invoke(Reference& self, const Global_Context& global, Cow_Vector<Reference>&& args) const override;
    void enumerate_variables(const Abstract_Variable_Callback& callback) const override;
  };

}  // namespace Asteria

#endif
