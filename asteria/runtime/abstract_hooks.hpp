// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

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
    on_declare(const Source_Location& sloc, const phcow_string& name);

    // This hook is called before every function call, be it native or not.
    virtual
    void
    on_call(const Source_Location& sloc, const cow_function& target);

    // This hook is called before returning a result from a function. For proper
    // tail calls where `ptc != ptc_aware_none`, this hook is called in their
    // 'natural' order, and `sloc` denotes the function-call operator to the target
    // function instead of the return statement.
    virtual
    void
    on_return(const Source_Location& sloc, PTC_Aware ptc);

    // This hook is called before after an exception value has been initialized,
    // and before it is thrown.
    virtual
    void
    on_throw(const Source_Location& sloc, Value& except);

    // This hook is called just after control flow enters a function, just before
    // execution of its body. `fctx` is the scope of the function, which contains
    // all parameters. This hook is provided for instrument reasons.
    virtual
    void
    on_function_enter(const Instantiated_Function& func, Executive_Context& fctx);

    // This hook is called just before control flow leaves a function, either
    // because control flow reaches the end of the function body, or because of
    // a return statement or a throw statement. For proper tail calls, this hook is
    // called before entering the target function, so it's hardly ever useful for
    // debugging. This hook is provided for instrument reasons.
    virtual
    void
    on_function_leave(const Instantiated_Function& func, Executive_Context& fctx);
  };

}  // namespace asteria
#endif
