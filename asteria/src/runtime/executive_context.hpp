// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "variadic_arguer.hpp"

namespace asteria {

class Executive_Context
  : public Abstract_Context
  {
  private:
    Executive_Context* m_parent_opt;

    // Store some references to the enclosing function,
    // so they are not passed here and there upon each native call.
    Global_Context* m_global;
    Reference_Stack* m_stack;

    cow_bivector<Source_Location, AVMC_Queue> m_defer;
    rcptr<Variadic_Arguer> m_zvarg;
    cow_vector<Reference> m_lazy_args;

  public:
    // A plain context must have a parent context.
    // Its parent context shall outlast itself.
    Executive_Context(M_plain, Executive_Context& parent)
      : m_parent_opt(::std::addressof(parent)),
        m_global(parent.m_global), m_stack(parent.m_stack)
      { }

    // A defer context is used to evaluate deferred expressions.
    // They are evaluated in separated contexts, as in case of proper tail calls,
    // contexts of enclosing function will have been destroyed.
    Executive_Context(M_defer, Global_Context& global, Reference_Stack& stack,
                      cow_bivector<Source_Location, AVMC_Queue>&& defer)
      : m_parent_opt(),
        m_global(::std::addressof(global)), m_stack(::std::addressof(stack)),
        m_defer(::std::move(defer))
      { }

    // A function context has no parent.
    // The caller shall define a global context and evaluation stack, both of which
    // shall outlast this context.
    Executive_Context(M_function, Global_Context& global, Reference_Stack& stack,
                      const rcptr<Variadic_Arguer>& zvarg,
                      const cow_vector<phsh_string>& params, Reference&& self);

    ASTERIA_NONCOPYABLE_DESTRUCTOR(Executive_Context);

  protected:
    bool
    do_is_analytic()
      const noexcept final
      { return this->is_analytic();  }

    Abstract_Context*
    do_get_parent_opt()
      const noexcept override
      { return this->get_parent_opt();  }

    Reference*
    do_lazy_lookup_opt(const phsh_string& name)
      override;

  public:
    bool
    is_analytic()
      const noexcept
      { return false;  }

    Executive_Context*
    get_parent_opt()
      const noexcept
      { return this->m_parent_opt;  }

    Global_Context&
    global()
      const noexcept
      { return *(this->m_global);  }

    Reference_Stack&
    stack()
      const noexcept
      { return *(this->m_stack);  }

    // Defer an expression which will be evaluated at scope exit.
    // The result of such expressions are discarded.
    Executive_Context&
    defer_expression(const Source_Location& sloc, AVMC_Queue&& queue);

    // These functions must be called before exiting a scope.
    // Note that these functions may throw arbitrary exceptions, which
    // is why RAII is inapplicable.
    AIR_Status
    on_scope_exit(AIR_Status status);

    Runtime_Error&
    on_scope_exit(Runtime_Error& except);
  };

}  // namespace asteria

#endif
