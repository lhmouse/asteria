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
    // This hook is called before every statement, expression, iteration of a loop,
    // etc. Be advised that traps require code generation support, which must be
    // enabled by setting `verbose_single_step_traps` in `Compiler_Options` before
    // parsing a script.
    virtual
    void
    on_trap(const Source_Location& sloc, Executive_Context& ctx);

    // This hook is called after a variable or function is declared, and before
    // its initializer is evaluated.
    virtual
    void
    on_declare(const Source_Location& sloc, const phsh_string& name);

    // This hook is called before every function call, be it native or not.
    virtual
    void
    on_call(const Source_Location& sloc, const cow_function& target);

    // This hook is called before returning a result from a function. For proper
    // tail calls, this is called in their 'natural' order, as if proper tail call
    // transformation didn't happen. If the function returns nothing, `result_opt`
    // is a null pointer.
    virtual
    void
    on_return(const Source_Location& sloc, Reference* result_opt);

    // This hook is called before after an exception value has been initialized,
    // and before it is thrown.
    virtual
    void
    on_throw(const Source_Location& sloc, Value& except);

    // This hook is called just after control flow enters a function, and before
    // its body. `fctx` contains all parameters.
    virtual
    void
    on_function_enter(Executive_Context& fctx, const Instantiated_Function& func);

    // This hook is called just before control flow leaves a function, either by
    // returning a result or by throwing an exception. A call to this hook is always
    // paired by a preceding call to `on_enter_function()`. In the case of a proper
    // tail call, this hook is called before entering the target function.
    virtual
    void
    on_function_leave(Executive_Context& fctx) noexcept;
  };

}  // namespace asteria
#endif
