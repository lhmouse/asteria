// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "variadic_arguer.hpp"
namespace asteria {

class Executive_Context
  :
    public Abstract_Context
  {
  private:
    const Executive_Context* m_parent_opt;

    // Store some references to the enclosing function,
    // so they are not passed here and there upon each native call.
    Global_Context* m_global;
    AIR_Status* m_status;
    Reference_Stack* m_stack;
    Reference_Stack* m_alt_stack;  // for nested calls

    cow_bivector<Source_Location, AVM_Rod> m_defer;
    const Instantiated_Function* m_func = nullptr;
    cow_vector<Reference> m_lazy_args;

  public:
    // A plain context must have a parent context.
    // Its parent context shall outlast itself.
    Executive_Context(Uxtc_plain, const Executive_Context& parent)
      :
        m_parent_opt(&parent), m_global(parent.m_global), m_status(parent.m_status),
        m_stack(parent.m_stack), m_alt_stack(parent.m_alt_stack)
      { }

    // A defer context is used to evaluate deferred expressions.
    // They are evaluated in separated contexts, as in case of proper tail calls,
    // contexts of enclosing function will have been destroyed.
    ASTERIA_INCOMPLET(AVM_Rod)
    Executive_Context(Uxtc_defer, Global_Context& xglobal, AIR_Status& xstatus,
                      Reference_Stack& xstack, Reference_Stack& ystack)
      :
        m_parent_opt(nullptr), m_global(&xglobal), m_status(&xstatus),
        m_stack(&xstack), m_alt_stack(&ystack)
      { }

    // A function context has no parent.
    // The caller shall define a global context and evaluation stack, both of which
    // shall outlast this context.
    Executive_Context(Uxtc_function, Global_Context& xglobal, AIR_Status& xstatus,
                      Reference_Stack& xstack, Reference_Stack& ystack,
                      const Instantiated_Function& xfunc, Reference&& xself);

  protected:
    Reference*
    do_create_lazy_reference_opt(Reference* hint_opt, const phcow_string& name)
      const override;

    bool
    do_is_analytic()
      const noexcept override
      { return false;  }

    const Abstract_Context*
    do_get_parent_opt()
      const noexcept override
      { return this->m_parent_opt;  }

    void
    do_on_scope_exit_normal_slow();

    void
    do_on_scope_exit_exceptional_slow(Runtime_Error& except);

  public:
    Executive_Context(const Executive_Context&) = delete;
    Executive_Context& operator=(const Executive_Context&) & = delete;
    ~Executive_Context();

    const Executive_Context*
    get_parent_opt()
      const noexcept
      { return this->m_parent_opt;  }

    Global_Context&
    global()
      const noexcept
      { return *(this->m_global);  }

    AIR_Status&
    status()
      const noexcept
      { return *(this->m_status);  }

    Reference_Stack&
    stack()
      const noexcept
      { return *(this->m_stack);  }

    Reference_Stack&
    alt_stack()
      const noexcept
      { return *(this->m_alt_stack);  }

    void
    swap_stacks()
      noexcept
      { ::std::swap(this->m_stack, this->m_alt_stack);  }

    // Get the defer expression list.
    const cow_bivector<Source_Location, AVM_Rod>&
    defer()
      const noexcept
      { return this->m_defer;  }

    cow_bivector<Source_Location, AVM_Rod>&
    mut_defer()
      noexcept
      { return this->m_defer;  }

    // These functions must be called before exiting a scope.
    // Note that these functions may throw arbitrary exceptions, which
    // is why RAII is inapplicable.
    void
    on_scope_exit_normal()
      {
        if(!this->m_defer.empty())
          this->do_on_scope_exit_normal_slow();
      }

    void
    on_scope_exit_exceptional(Runtime_Error& except)
      {
        if(!this->m_defer.empty())
          this->do_on_scope_exit_exceptional_slow(except);
      }
  };

}  // namespace asteria
#endif
