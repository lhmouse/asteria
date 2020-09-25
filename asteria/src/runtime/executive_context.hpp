// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "variadic_arguer.hpp"
#include "evaluation_stack.hpp"
#include "../llds/avmc_queue.hpp"

namespace asteria {

class Executive_Context
  : public Abstract_Context
  {
  private:
    const Executive_Context* m_parent_opt;

    // Store some references to the enclosing function,
    // so they are not passed here and there upon each native call.
    ref<Global_Context> m_global;
    ref<Evaluation_Stack> m_stack;

    // These members are used for lazy initialization.
    rcptr<Variadic_Arguer> m_zvarg;
    cow_vector<Reference> m_lazy_args;

    // This stores deferred expressions.
    cow_bivector<Source_Location, AVMC_Queue> m_defer;

  public:
    template<typename ContextT,
    ROCKET_ENABLE_IF(::std::is_base_of<Executive_Context, ContextT>::value)>
    Executive_Context(ref<ContextT> parent)  // for non-functions
      : m_parent_opt(parent.ptr()),
        m_global(parent.get().m_global), m_stack(parent.get().m_stack)
      { }

    Executive_Context(ref<Global_Context> xglobal, ref<Evaluation_Stack> xstack,
                      cow_bivector<Source_Location, AVMC_Queue>&& defer)  // for proper tail calls
      : m_parent_opt(nullptr),
        m_global(xglobal), m_stack(xstack)
      { this->m_defer = ::std::move(defer);  }

    Executive_Context(ref<Global_Context> xglobal, ref<Evaluation_Stack> xstack,
                      const rcptr<Variadic_Arguer>& zvarg, const cow_vector<phsh_string>& params,
                      Reference&& self, cow_vector<Reference>&& args)  // for functions
      : m_parent_opt(nullptr),
        m_global(xglobal), m_stack(xstack)
      { this->do_bind_parameters(zvarg, params, ::std::move(self), ::std::move(args));  }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(Executive_Context);

  private:
    void
    do_bind_parameters(const rcptr<Variadic_Arguer>& zvarg, const cow_vector<phsh_string>& params,
                       Reference&& self, cow_vector<Reference>&& args);

  protected:
    bool
    do_is_analytic()
    const noexcept final
      { return this->is_analytic();  }

    const Abstract_Context*
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

    const Executive_Context*
    get_parent_opt()
    const noexcept
      { return this->m_parent_opt;  }

    Global_Context&
    global()
    const noexcept
      { return this->m_global;  }

    Evaluation_Stack&
    stack()
    const noexcept
      { return this->m_stack;  }

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
