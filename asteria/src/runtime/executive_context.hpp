// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Executive_Context : public Abstract_Context
  {
  private:
    const Executive_Context* m_parent_opt;

    std::reference_wrapper<const Global_Context> m_global;
    std::reference_wrapper<Evaluation_Stack> m_stack;
    const Rcobj<Variadic_Arguer>& m_zvarg;

  public:
    Executive_Context(int, const Global_Context& xglobal, Evaluation_Stack& xstack, const Rcobj<Variadic_Arguer>& xzvarg,  // for functions
                           const Cow_Vector<PreHashed_String>& params, Reference&& self, Cow_Vector<Reference>&& args)
      : m_parent_opt(nullptr),
        m_global(xglobal), m_stack(xstack), m_zvarg(xzvarg)
      {
        this->do_prepare_function(params, rocket::move(self), rocket::move(args));
      }
    Executive_Context(int, const Executive_Context& parent)  // for non-functions
      : m_parent_opt(&parent),
        m_global(parent.m_global), m_stack(parent.m_stack), m_zvarg(parent.m_zvarg)
      {
      }
    ~Executive_Context() override;

  private:
    void do_prepare_function(const Cow_Vector<PreHashed_String>& params, Reference&& self, Cow_Vector<Reference>&& args);

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
    const Rcobj<Variadic_Arguer>& zvarg() const noexcept
      {
        return this->m_zvarg;
      }
  };

}  // namespace Asteria

#endif
