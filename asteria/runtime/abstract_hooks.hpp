// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_HOOKS_
#define ASTERIA_RUNTIME_ABSTRACT_HOOKS_

#include "../fwd.hpp"
namespace asteria {

struct Abstract_Hooks
  : public rcfwd<Abstract_Hooks>
  {
    explicit
    Abstract_Hooks() noexcept = default;

    ASTERIA_COPYABLE_DESTRUCTOR(Abstract_Hooks);

    // This hook is called when a variable (mutable or immutable) or function is declared,
    // and before its initializer is evaluated.
    virtual
    void
    on_variable_declare(const Source_Location& sloc, phsh_stringR name)
      {
        (void)sloc;
        (void)name;
      }

    // This hook is called before every function call (whether native or not) from Asteria.
    virtual
    void
    on_function_call(const Source_Location& sloc, const cow_function& target)
      {
        (void)sloc;
        (void)target;
      }

    // This hook is called after every function call that completes by returning normally.
    virtual
    void
    on_function_return(const Source_Location& sloc, const cow_function& target,
                       const Reference& result)
      {
        (void)sloc;
        (void)target;
        (void)result;
      }

    // This hook is called after every function call that completes by throwing an
    // exception. The original exception will be rethrown after the hook returns.
    // N.B. It is suggested that you should not throw exceptions from this hook.
    virtual
    void
    on_function_except(const Source_Location& sloc, const cow_function& target,
                       const Runtime_Error& except)
      {
        (void)sloc;
        (void)target;
        (void)except;
      }

    // This hook is called before every statement, condition, etc.
    // Be advised that single-step traps require code generation support, which must be
    // enabled by setting `verbose_single_step_traps` in `Compiler_Options`.
    virtual
    void
    on_single_step_trap(const Source_Location& sloc)
      {
        (void)sloc;
      }
  };

}  // namespace asteria
#endif
