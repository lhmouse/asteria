// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "variadic_arguer.hpp"

namespace Asteria {

class Executive_Context : public Abstract_Context
  {
  private:
    const Executive_Context* m_parent_opt;

    // Store some references to the enclosing function,
    // so they are not passed here and there upon each native call.
    ref_to<Global_Context> m_global;
    ref_to<Evaluation_Stack> m_stack;
    ckptr<Variadic_Arguer> m_zvarg;

    // These members are used for lazy initialization.
    Reference m_self;
    cow_vector<Reference> m_args;

  public:
    Executive_Context(ref_to<const Executive_Context> parent, nullptr_t)  // for non-functions
      :
        m_parent_opt(parent.ptr()),
        m_global(parent->m_global), m_stack(parent->m_stack), m_zvarg(parent->m_zvarg),
        m_self(Reference_Root::S_void())
      {
      }
    Executive_Context(ref_to<Global_Context> xglobal, ref_to<Evaluation_Stack> xstack,
                      const ckptr<Variadic_Arguer>& xzvarg, Reference&& self,
                      const cow_vector<phsh_string>& params, cow_vector<Reference>&& args)  // for functions
      :
        m_parent_opt(nullptr),
        m_global(xglobal), m_stack(xstack), m_zvarg(xzvarg),
        m_self(::rocket::move(self))
      {
        this->do_bind_parameters(params, ::rocket::move(args));
      }
    ~Executive_Context() override;

  private:
    void do_bind_parameters(const cow_vector<phsh_string>& params, cow_vector<Reference>&& args);

  protected:
    bool do_is_analytic() const noexcept final;
    const Abstract_Context* do_get_parent_opt() const noexcept override;
    Reference* do_lazy_lookup_opt(const phsh_string& name) override;

  public:
    bool is_analytic() const noexcept
      {
        return false;
      }
    const Executive_Context* get_parent_opt() const noexcept
      {
        return this->m_parent_opt;
      }

    Global_Context& global() const noexcept
      {
        return this->m_global;
      }
    Evaluation_Stack& stack() const noexcept
      {
        return this->m_stack;
      }
    const ckptr<Variadic_Arguer>& zvarg() const noexcept
      {
        return this->m_zvarg;
      }
  };

}  // namespace Asteria

#endif
