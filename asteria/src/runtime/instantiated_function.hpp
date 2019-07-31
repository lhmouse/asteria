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
    rcobj<Variadic_Arguer> m_zvarg;
    cow_vector<phsh_string> m_params;

    cow_vector<uptr<Air_Node>> m_code;

  public:
    Instantiated_Function(const Source_Location& sloc, const cow_string& func, const cow_vector<phsh_string>& params,
                          cow_vector<uptr<Air_Node>>&& code)
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
    const cow_string& get_function_signature() const noexcept
      {
        return this->m_zvarg->get_function_signature();
      }
    const cow_vector<phsh_string>& get_parameters() const noexcept
      {
        return this->m_params;
      }

    std::ostream& describe(std::ostream& os) const override;
    Reference& invoke(Reference& self, const Global_Context& global, cow_vector<Reference>&& args) const override;
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override;
  };

}  // namespace Asteria

#endif
