// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Executive_Context final : public Abstract_Context
  {
  private:
    const Executive_Context* m_parent_opt;
    // Store some references to the enclosing function,
    // so they are not passed here and there upon each native call.
    ref_to<const Global_Context> m_global;
    ref_to<Evaluation_Stack> m_stack;
    ref_to<const rcobj<Variadic_Arguer>> m_zvarg;

  public:
    template<typename ContextT, ASTERIA_SFINAE_CONVERT(ContextT*, const Executive_Context*)> explicit Executive_Context(ref_to<ContextT> parent)  // for non-functions
      : m_parent_opt(parent.ptr()),
        m_global(this->m_parent_opt->m_global), m_stack(this->m_parent_opt->m_stack), m_zvarg(this->m_parent_opt->m_zvarg)
      {
      }
    Executive_Context(ref_to<const Global_Context> xglobal, ref_to<Evaluation_Stack> xstack, ref_to<const rcobj<Variadic_Arguer>> xzvarg,  // for functions
                      const cow_vector<phsh_string>& params, Reference&& self, cow_vector<Reference>&& args)
      : m_parent_opt(nullptr),
        m_global(xglobal), m_stack(xstack), m_zvarg(xzvarg)
      {
        this->do_prepare_function(params, rocket::move(self), rocket::move(args));
      }
    ~Executive_Context() override;

  private:
    void do_prepare_function(const cow_vector<phsh_string>& params, Reference&& self, cow_vector<Reference>&& args);

  public:
    bool is_analytic() const noexcept override
      {
        return false;
      }
    const Executive_Context* get_parent_opt() const noexcept override
      {
        return this->m_parent_opt;
      }

    const Global_Context& global() const noexcept
      {
        return this->m_global;
      }
    Evaluation_Stack& stack() const noexcept
      {
        return this->m_stack;
      }
    const rcobj<Variadic_Arguer>& zvarg() const noexcept
      {
        return this->m_zvarg;
      }
  };

}  // namespace Asteria

#endif
