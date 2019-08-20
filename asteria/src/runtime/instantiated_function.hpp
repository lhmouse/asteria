// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_

#include "../fwd.hpp"
#include "abstract_function.hpp"
#include "variadic_arguer.hpp"
#include "../llds/avmc_queue.hpp"

namespace Asteria {

class Instantiated_Function final : public Abstract_Function
  {
  private:
    cow_vector<phsh_string> m_params;
    rcobj<Variadic_Arguer> m_zvarg;
    AVMC_Queue m_queue;

  public:
    Instantiated_Function(const Compiler_Options& options, const Source_Location& sloc, const cow_string& name,
                          const Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params, const cow_vector<Statement>& stmts)
      : m_params(params),
        m_zvarg(this->do_create_zvarg(sloc, name)),
        m_queue(this->do_generate_code(options, ctx_opt, stmts))
      {
      }
    ~Instantiated_Function() override;

  private:
    rcobj<Variadic_Arguer> do_create_zvarg(const Source_Location& sloc, const cow_string& name) const;
    AVMC_Queue do_generate_code(const Compiler_Options& options, const Abstract_Context* ctx_opt, const cow_vector<Statement>& stmts) const;

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

    std::ostream& describe(std::ostream& ostrm) const override;
    Reference& invoke(Reference& self, const Global_Context& global, cow_vector<Reference>&& args) const override;
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override;
  };

}  // namespace Asteria

#endif
