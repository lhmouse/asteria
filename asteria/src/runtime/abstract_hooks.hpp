// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_HOOKS_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_HOOKS_HPP_

#include "../fwd.hpp"
#include "../rcbase.hpp"

namespace Asteria {

class Abstract_Hooks : public virtual Rcbase
  {
  public:
    Abstract_Hooks() noexcept
      {
      }
    ~Abstract_Hooks() override;

  public:
    // This hook is called when a variable (mutable or immutable) or function is declared,
    // before its initializer is evaluated.
    virtual void on_variable_declare(const Source_Location& sloc, const cow_string& inside,
                                     const phsh_string& name);
    // This hook is called before every function call (whether native or not) from Asteria.
    virtual void on_function_call(const Source_Location& sloc, const cow_string& inside,
                                  const rcptr<Abstract_Function>& target);
    // This hook is called after every function call that completes by returning normally.
    virtual void on_function_return(const Source_Location& sloc, const cow_string& inside,
                                    const Reference& result);
    // This hook is called after every function call that completes by throwing an exception.
    // The original exception will be rethrown after the hook returns.
    // N.B. It is suggested that you should not throw exceptions from this hook.
    virtual void on_function_except(const Source_Location& sloc, const cow_string& inside,
                                    const Runtime_Error& except);
    // This hook is called before every statement, condition, etc.
    // Be advised that single-step traps require code generation support, which can be
    // disabled by setting `no_plain_single_step_traps` in `Compiler_Options`.
    virtual void on_single_step_trap(const Source_Location& sloc, const cow_string& inside,
                                     Executive_Context* ctx_opt);
  };

}  // namespace Asteria

#endif
