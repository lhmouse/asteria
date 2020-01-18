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
    mutable Reference m_self = Reference_Root::S_void();
    mutable cow_vector<Reference> m_args;

  public:
    template<typename ContextT, ASTERIA_SFINAE_CONVERT(ContextT*, const Executive_Context*)>
        explicit Executive_Context(ref_to<ContextT> parent)  // for non-functions
      :
        m_parent_opt(parent.ptr()),
        m_global(parent->m_global), m_stack(parent->m_stack), m_zvarg(parent->m_zvarg)
      {
      }
    Executive_Context(ref_to<Global_Context> xglobal, ref_to<Evaluation_Stack> xstack,
                      const ckptr<Variadic_Arguer> xzvarg, const cow_vector<phsh_string>& params,
                      Reference&& self, cow_vector<Reference>&& args)  // for functions
      :
        m_parent_opt(nullptr),
        m_global(xglobal), m_stack(xstack), m_zvarg(xzvarg)
      {
        this->do_prepare_function(params, ::rocket::move(self), ::rocket::move(args));
      }
    ~Executive_Context() override;

  private:
    void do_prepare_function(const cow_vector<phsh_string>& params, Reference&& self, cow_vector<Reference>&& args);

  protected:
    bool do_is_analytic() const noexcept final;
    const Abstract_Context* do_get_parent_opt() const noexcept override;
    Reference* do_lazy_lookup_opt(Reference_Dictionary& named_refs, const phsh_string& name) const override;

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
    const cow_string& func() const noexcept
      {
        return this->m_zvarg->func();
      }
  };

}  // namespace Asteria

#endif
