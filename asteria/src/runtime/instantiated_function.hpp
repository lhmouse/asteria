// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_

#include "../fwd.hpp"
#include "variadic_arguer.hpp"
#include "../llds/avmc_queue.hpp"
#include "../abstract_function.hpp"

namespace Asteria {

class Instantiated_Function final : public Abstract_Function
  {
  private:
    cow_vector<phsh_string> m_params;
    rcptr<Variadic_Arguer> m_zvarg;
    AVMC_Queue m_queue;

  public:
    Instantiated_Function(const cow_vector<phsh_string>& params, rcptr<Variadic_Arguer>&& zvarg,
                          const cow_vector<AIR_Node>& code)
      :
        m_params(params), m_zvarg(::rocket::move(zvarg))
      {
        this->do_solidify_code(code);
      }
    ~Instantiated_Function() override;

  private:
    void do_solidify_code(const cow_vector<AIR_Node>& code);

  public:
    const Source_Location& source_location() const noexcept
      {
        return this->m_zvarg->sloc();
      }
    const cow_string& signature() const noexcept
      {
        return this->m_zvarg->func();
      }
    const cow_vector<phsh_string>& parameters() const noexcept
      {
        return this->m_params;
      }

    tinyfmt& describe(tinyfmt& fmt) const override;
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override;
    Reference& invoke_ptc_aware(Reference& self, Global_Context& global,
                                cow_vector<Reference>&& args) const override;
  };

}  // namespace Asteria

#endif
