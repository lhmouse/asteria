// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "variable.hpp"
#include "instantiated_function.hpp"
#include "../llds/avm_rod.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"
namespace asteria {

Executive_Context::
Executive_Context(Uxtc_function, Global_Context& xglobal, Reference_Stack& xstack, Reference_Stack& ystack,
                  const Instantiated_Function& xfunc, Reference&& xself)
  :
    m_parent_opt(nullptr), m_global(&xglobal), m_stack(&xstack), m_alt_stack(&ystack), m_func(&xfunc)
  {
    // Set the `this` reference, but only if it is a variable or non-null. When
    // `this` is null, it is likely that it is never referenced in the function,
    // so lazy initialization is performed to avoid the overhead here.
    if(!xself.is_invalid())
      this->do_mut_named_reference(nullptr, sref("__this")) = move(xself);

    // Set arguments. Because arguments are evaluated from left to right, the
    // reference at the top is the last argument.
    uint32_t nargs = this->m_stack->size();
    bool has_ellipsis = false;

    for(const auto& name : this->m_func->params())
      if(name != sref("...")) {
        // Try popping an argument and assign it.
        auto& param = this->do_mut_named_reference(nullptr, name);
        if(nargs == 0)
          param.set_temporary(nullopt);
        else
          param = move(this->m_stack->mut_top(--nargs));
      }
      else
        has_ellipsis = true;

    if(!has_ellipsis && (nargs != 0))
      throw Runtime_Error(xtc_format,
               "Too many arguments passed to `$1`", this->m_func->func());

    // Move all arguments into the variadic argument getter.
    while(nargs != 0)
      this->m_lazy_args.emplace_back(move(this->m_stack->mut_top(--nargs)));
  }

Executive_Context::
~Executive_Context()
  {
  }

void
Executive_Context::
vtable_key_function_sLBHstEX() noexcept
  {
  }

Reference*
Executive_Context::
do_create_lazy_reference_opt(Reference* hint_opt, phsh_stringR name) const
  {
    // Create pre-defined references as needed.
    // N.B. If you have ever changed these, remember to update
    // 'analytic_context.cpp' as well.
    if(name == sref("__this"))
      return &(this->do_mut_named_reference(hint_opt, name));

    if(name == sref("__func")) {
      ROCKET_ASSERT(this->m_func);
      auto& ref = this->do_mut_named_reference(hint_opt, name);
      ref.set_temporary(this->m_func->func());
      return &ref;
    }

    if(name == sref("__varg")) {
      ROCKET_ASSERT(this->m_func);
      auto& ref = this->do_mut_named_reference(hint_opt, name);
      ref.set_temporary(::rocket::make_refcnt<Variadic_Arguer>(*(this->m_func), this->m_lazy_args));
      return &ref;
    }

    return nullptr;
  }

void
Executive_Context::
do_on_scope_exit_normal_slow(AIR_Status status)
  {
    Reference self;
    if(status == air_status_return_ref) {
      // If a PTC wrapper was returned, append all deferred expressions to it.
      // These callbacks will be unpacked later, so we just return.
      if(this->m_stack->top().is_ptc()) {
        const auto ptc = this->m_stack->top().unphase_ptc_opt();
        ROCKET_ASSERT(ptc);
        ptc->mut_defer().append(this->m_defer.move_begin(), this->m_defer.move_end());
        return;
      }

      // Stash the result reference.
      self = move(this->m_stack->mut_top());
    }

    // Execute all deferred expressions backwards.
    while(!this->m_defer.empty()) {
      auto pair = move(this->m_defer.mut_back());
      this->m_defer.pop_back();

      // Execute it.
      // If an exception is thrown, append a frame and rethrow it.
      try {
        pair.second.execute(*this);
      }
      catch(Runtime_Error& except) {
        except.push_frame_defer(pair.first);
        this->do_on_scope_exit_exceptional_slow(except);
        throw;
      }
    }

    // Restore the result reference.
    if(!self.is_invalid())
      this->m_stack->push() = move(self);
  }

void
Executive_Context::
do_on_scope_exit_exceptional_slow(Runtime_Error& except)
  {
    // Execute all deferred expressions backwards.
    while(!this->m_defer.empty()) {
      auto pair = move(this->m_defer.mut_back());
      this->m_defer.pop_back();

      // Execute it.
      // If an exception is thrown, replace `except` with it.
      try {
        pair.second.execute(*this);
      }
      catch(Runtime_Error& nested) {
        except = nested;
        except.push_frame_defer(pair.first);
      }
    }
  }

}  // namespace asteria
