// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_

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
    Reference_Stack* m_alt_stack;  // for nested calls

    cow_bivector<Source_Location, AVMC_Queue> m_defer;
    refcnt_ptr<Variadic_Arguer> m_zvarg;
    cow_vector<Reference> m_lazy_args;

  public:
    // A plain context must have a parent context.
    // Its parent context shall outlast itself.
    explicit
    Executive_Context(M_plain, Executive_Context& parent)
      : m_parent_opt(&parent),
        m_global(parent.m_global), m_stack(parent.m_stack),
        m_alt_stack(parent.m_alt_stack)  { }

    // A defer context is used to evaluate deferred expressions.
    // They are evaluated in separated contexts, as in case of proper tail calls,
    // contexts of enclosing function will have been destroyed.
    ASTERIA_INCOMPLET(AVMC_Queue)
    explicit
    Executive_Context(M_defer, Global_Context& global, Reference_Stack& stack,
                      Reference_Stack& alt_stack,
                      cow_bivector<Source_Location, AVMC_Queue>&& defer)
      : m_parent_opt(),
        m_global(&global), m_stack(&stack), m_alt_stack(&alt_stack),
        m_defer(::std::move(defer))  { }

    // A function context has no parent.
    // The caller shall define a global context and evaluation stack, both of which
    // shall outlast this context.
    explicit
    Executive_Context(M_function, Global_Context& global, Reference_Stack& stack,
                      Reference_Stack& alt_stack, const refcnt_ptr<Variadic_Arguer>& zvarg,
                      const cow_vector<phsh_string>& params, Reference&& self);

  private:
    bool
    do_is_analytic() const noexcept final
      { return this->is_analytic();  }

    Abstract_Context*
    do_get_parent_opt() const noexcept override
      { return this->get_parent_opt();  }

    Reference*
    do_create_lazy_reference_opt(Reference* hint_opt, phsh_stringR name) const override;

    ROCKET_COLD
    AIR_Status
    do_on_scope_exit_slow(AIR_Status status);

    ROCKET_COLD
    Runtime_Error&
    do_on_scope_exit_slow(Runtime_Error& except);

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Executive_Context);

    bool
    is_analytic() const noexcept
      { return false;  }

    Executive_Context*
    get_parent_opt() const noexcept
      { return this->m_parent_opt;  }

    Global_Context&
    global() const noexcept
      { return *(this->m_global);  }

    Reference_Stack&
    stack() const noexcept
      { return *(this->m_stack);  }

    Reference_Stack&
    alt_stack() const noexcept
      { return *(this->m_alt_stack);  }

    // Defer an expression which will be evaluated at scope exit.
    // The result of such expressions are discarded.
    void
    defer_expression(const Source_Location& sloc, AVMC_Queue&& queue);

    // These functions must be called before exiting a scope.
    // Note that these functions may throw arbitrary exceptions, which
    // is why RAII is inapplicable.
    AIR_Status
    on_scope_exit(AIR_Status status)
      {
        return ROCKET_UNEXPECT(this->m_defer.size())
          ? this->do_on_scope_exit_slow(status)
          : status;
      }

    Runtime_Error&
    on_scope_exit(Runtime_Error& except)
      {
        return ROCKET_UNEXPECT(this->m_defer.size())
          ? this->do_on_scope_exit_slow(except)
          : except;
      }
  };

}  // namespace asteria
#endif
