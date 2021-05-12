// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "variable.hpp"
#include "../llds/avmc_queue.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"

namespace asteria {

ROCKET_FLATTEN
Executive_Context::
Executive_Context(M_function, Global_Context& global, Reference_Stack& stack,
                  Reference_Stack& alt_stack, const rcptr<Variadic_Arguer>& zvarg,
                  const cow_vector<phsh_string>& params, Reference&& self)
  : m_parent_opt(),
    m_global(&global), m_stack(&stack), m_alt_stack(&alt_stack),
    m_zvarg(zvarg)
  {
    // Set the `this` reference.
    if(self.is_temporary()) {
      // If the self reference is null, it is likely that `this` isn't ever
      // referenced in this function, so perform lazy initialization to avoid
      // this overhead.
      const auto& val = self.dereference_readonly();
      if(!val.is_null())
        this->do_open_named_reference(nullptr, sref("__this")).set_temporary(val);
    }
    else if(self.is_variable()) {
      // If the self reference points to a variable, copy it because it is
      // always an lvalue.
      auto var = self.get_variable_opt();
      ROCKET_ASSERT(var);
      this->do_open_named_reference(nullptr, sref("__this")).set_variable(::std::move(var));
    }
    else
      ASTERIA_THROW("invalid `this` reference passed to `$1`", zvarg->func());

    // Set arguments. As arguments are evaluated from left to right, the
    // reference at the top is the last argument.
    auto bptr = ::std::make_move_iterator(stack.bottom());
    auto eptr = ::std::make_move_iterator(stack.top());
    bool variadic = false;

    for(const auto& name : params) {
      if(name.empty())
        continue;

      // Nothing is set for the variadic placeholder, but the parameter
      // list terminates here.
      variadic = name == sref("...");
      if(variadic)
        break;

      // Try popping an argument from `stack` and assign it to this parameter.
      // If no more arguments follow, declare a constant `null`.
      if(bptr != eptr)
        this->do_open_named_reference(nullptr, name) = *(bptr++);
      else
        this->do_open_named_reference(nullptr, name).set_temporary(nullopt);
    }

    // If the function is not variadic, then all arguments must have been consumed.
    if(!variadic && (bptr != eptr))
      ASTERIA_THROW("too many arguments passed to `$1`", zvarg->func());

    // Stash variadic arguments, if any.
    if(bptr != eptr)
      this->m_lazy_args.append(bptr, eptr);
  }

Executive_Context::
~Executive_Context()
  {
  }

Reference*
Executive_Context::
do_create_lazy_reference(Reference* hint_opt, const phsh_string& name) const
  {
    // Create pre-defined references as needed.
    // N.B. If you have ever changed these, remember to update
    // 'analytic_context.cpp' as well.
    if(name == "__func") {
      // Note: This can only happen inside a function context.
      auto& ref = this->do_open_named_reference(hint_opt, name);
      ref.set_temporary(this->m_zvarg->func());
      return &ref;
    }

    if(name == "__this") {
      // Note: This can only happen inside a function context and the `this`
      // argument is null.
      auto& ref = this->do_open_named_reference(hint_opt, name);
      ref.set_temporary(nullopt);
      return &ref;
    }

    if(name == "__varg") {
      // Note: This can only happen inside a function context.
      auto& ref = this->do_open_named_reference(hint_opt, name);
      ref.set_temporary(
            this->m_lazy_args.empty()
              ? this->m_zvarg
              : ::rocket::make_refcnt<Variadic_Arguer>(
                          *(this->m_zvarg), this->m_lazy_args));
      return &ref;
    }

    return nullptr;
  }

Executive_Context&
Executive_Context::
defer_expression(const Source_Location& sloc, AVMC_Queue&& queue)
  {
    this->m_defer.emplace_back(sloc, ::std::move(queue));
    return *this;
  }

AIR_Status
Executive_Context::
do_on_scope_exit_slow(AIR_Status status)
  {
    // Stash the returned reference, if any.
    Reference self;
    if(status == air_status_return_ref)
      self = ::std::move(this->m_stack->mut_back());

    if(auto ptca = self.get_ptc_args_opt()) {
      // If a PTC wrapper was returned, prepend all deferred expressions
      // to it. These callbacks will be unpacked later, so we just return.
      if(ptca->get_defer().empty())
        ptca->open_defer().swap(this->m_defer);
      else
        ptca->open_defer().append(
                this->m_defer.move_begin(), this->m_defer.move_end());
    }
    else {
      // Execute all deferred expressions backwards.
      while(this->m_defer.size()) {
        auto pair = ::std::move(this->m_defer.mut_back());
        this->m_defer.pop_back();

        // Execute it.
        // If an exception is thrown, append a frame and rethrow it.
        ASTERIA_RUNTIME_TRY {
          auto status_def = pair.second.execute(*this);
          ROCKET_ASSERT(status_def == air_status_next);
        }
        ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
          except.push_frame_defer(pair.first);
          this->on_scope_exit(except);
          throw;
        }
      }
      if(self.is_uninit())
        return status;
    }
    ROCKET_ASSERT(!self.is_uninit());

    // Restore the returned reference.
    this->m_stack->mut_back() = ::std::move(self);
    return status;
  }

Runtime_Error&
Executive_Context::
do_on_scope_exit_slow(Runtime_Error& except)
  {
    // Execute all deferred expressions backwards.
    while(this->m_defer.size()) {
      auto pair = ::std::move(this->m_defer.mut_back());
      this->m_defer.pop_back();

      // Execute it.
      // If an exception is thrown, replace `except` with it.
      ASTERIA_RUNTIME_TRY {
        auto status_def = pair.second.execute(*this);
        ROCKET_ASSERT(status_def == air_status_next);
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& nested) {
        except = nested;
        except.push_frame_defer(pair.first);
      }
    }
    return except;
  }

}  // namespace asteria
