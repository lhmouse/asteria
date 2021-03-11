// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../llds/avmc_queue.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"

namespace asteria {

Executive_Context::
Executive_Context(M_function, Global_Context& global, Reference_Stack& stack,
                  Reference_Stack& alt_stack, const rcptr<Variadic_Arguer>& zvarg,
                  const cow_vector<phsh_string>& params, Reference&& self)
  : m_parent_opt(),
    m_global(&global), m_stack(&stack), m_alt_stack(&alt_stack),
    m_zvarg(zvarg)
  {
    // Set the `this` reference.
    // If the self reference is void, it is likely that `this` isn't ever referenced in
    // this function, so perform lazy initialization to avoid this overhead.
    if(!self.is_void() && !(self.is_constant() && self.dereference_readonly().is_null()))
      this->open_named_reference(sref("__this")) = ::std::move(self);

    // Prepare iterators to arguments.
    // As function arguments are evaluated from left to right, the reference at the top
    // is the last argument.
    auto bptr = stack.bottom();
    auto eptr = stack.top();

    // Set parameters, which are local references.
    bool variadic = false;
    for(const auto& name : params) {
      if(name.empty())
        continue;

      // Nothing is set for the variadic placeholder, but the parameter list
      // terminates here.
      variadic = (name.size() == 3) && mem_equal(name.c_str(), "...", 4);
      if(variadic)
        break;

      // Try popping an argument from `stack` and assign it to this parameter.
      // If no more arguments follow, declare a constant `null`.
      if(ROCKET_EXPECT(bptr != eptr))
        this->open_named_reference(name) = ::std::move(*(bptr++));
      else
        this->open_named_reference(name) = Reference::S_constant();
    }

    // If the function is not variadic, then all arguments shall have been consumed.
    if(!variadic && (bptr != eptr))
      ASTERIA_THROW("Too many arguments passed to `$1`", zvarg->func());

    // Stash variadic arguments, if any.
    if(ROCKET_UNEXPECT(bptr != eptr))
      this->m_lazy_args.append(::std::make_move_iterator(bptr),
                     ::std::make_move_iterator(eptr));
  }

Executive_Context::
~Executive_Context()
  {
  }

Reference*
Executive_Context::
do_create_lazy_reference(Reference* hint_opt, const phsh_string& name)
  const
  {
    // Create pre-defined references as needed.
    // N.B. If you have ever changed these, remember to update 'analytic_context.cpp'
    // as well.
    if(name == "__func") {
      // Note: This can only happen inside a function context.
      Reference::S_constant xref = { this->m_zvarg->func() };
      return this->do_set_named_reference(hint_opt, name, ::std::move(xref));
    }

    if(name == "__this") {
      // Note: This can only happen inside a function context and the `this` argument
      // is null.
      Reference::S_constant xref = { };
      return this->do_set_named_reference(hint_opt, name, ::std::move(xref));
    }

    if(name == "__varg") {
      // Note: This can only happen inside a function context.
      Reference::S_constant xref;
      if(ROCKET_UNEXPECT(this->m_lazy_args.empty()))
        xref.val = this->m_zvarg;
      else
        xref.val = ::rocket::make_refcnt<Variadic_Arguer>(*(this->m_zvarg),
                                 this->m_lazy_args);
      return this->do_set_named_reference(hint_opt, name, ::std::move(xref));
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
    Reference self = Reference::S_uninit();
    if(status == air_status_return_ref)
      self = ::std::move(this->m_stack->mut_back());

    if(auto ptca = self.get_ptc_args_opt()) {
      // If a PTC wrapper was returned, prepend all deferred expressions to it.
      // These callbacks will be unpacked later, so we just return.
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
