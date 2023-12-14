// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_HOOKS_
#define ASTERIA_RUNTIME_ABSTRACT_HOOKS_

#include "../fwd.hpp"
namespace asteria {

struct Abstract_Hooks
  :
    public rcfwd<Abstract_Hooks>
  {
    explicit
    Abstract_Hooks() noexcept = default;

    ASTERIA_COPYABLE_DESTRUCTOR(Abstract_Hooks);

    // This hook is called when a variable (mutable or immutable) or function is
    // declared, and before its initializer is evaluated.
    virtual
    void
    on_variable_declare(const Source_Location& sloc, phsh_stringR name)
      {
        (void) sloc;
        (void) name;
      }

    // This hook is called before every function call, whether native or not.
    virtual
    void
    on_function_call(const Source_Location& sloc, const cow_function& target)
      {
        (void) sloc;
        (void) target;
      }

    // This hook is called before every statement, condition, etc.
    // Be advised that single-step traps require code generation support, which must be
    // enabled by setting `verbose_single_step_traps` in `Compiler_Options`.
    virtual
    void
    on_single_step_trap(Executive_Context& ctx, const Source_Location& sloc)
      {
        (void) ctx;
        (void) sloc;
      }

    // This hook is called just after entering a function body.
    virtual
    void
    on_function_enter(Executive_Context& func_ctx, const Instantiated_Function& target,
                      const Source_Location& func_sloc)
      {
        (void) func_ctx;
        (void) target;
        (void) func_sloc;
      }

    // This hook is called just before leaving a function body. In the case of a
    // proper tail call, this precedes entrance of the target function.
    virtual
    void
    on_function_leave(Executive_Context& func_ctx, const Instantiated_Function& target,
                      const Source_Location& func_sloc)
      {
        (void) func_ctx;
        (void) target;
        (void) func_sloc;
      }

    // This hook is called just before a result is returned from a function body.
    virtual
    void
    on_function_return(const Instantiated_Function& target, const Source_Location& func_sloc,
                       Reference& result)
      {
        (void) target;
        (void) func_sloc;
        (void) result;
      }

    // This hook is called just before leaving from a function body because of an
    // exception. The original exception will be rethrown after this hook returns.
    virtual
    void
    on_function_except(const Instantiated_Function& target, const Source_Location& func_sloc,
                       Runtime_Error& except)
      {
        (void) target;
        (void) func_sloc;
        (void) except;
      }
  };

}  // namespace asteria
#endif
