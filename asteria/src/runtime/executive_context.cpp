// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../llds/avmc_queue.hpp"
#include "../util.hpp"

namespace asteria {
namespace {

template<typename XRefT>
inline
Reference*
do_set_lazy_reference(Executive_Context& ctx, const phsh_string& name, XRefT&& xref)
  {
    auto& ref = ctx.open_named_reference(name);
    ref = ::std::forward<XRefT>(xref);
    return ::std::addressof(ref);
  }

template<typename ElemT>
inline
cow_vector<ElemT>&
do_concatenate(cow_vector<ElemT>& lhs, cow_vector<ElemT>&& rhs)
  {
    if(lhs.size()) {
      lhs.append(rhs.move_begin(), rhs.move_end());
      rhs.clear();
    }
    else {
      lhs.swap(rhs);
    }
    return lhs;
  }

}  // namespace

Executive_Context::
~Executive_Context()
  {
  }

void
Executive_Context::
do_bind_parameters(const rcptr<Variadic_Arguer>& zvarg, const cow_vector<phsh_string>& params,
                   Reference&& self, cow_vector<Reference>&& args)
  {
    // Set the zero-ary argument getter.
    this->m_zvarg = zvarg;

    // Set the `this` reference.
    // If the self reference is void, it is likely that `this` isn't ever referenced in this function,
    // so perform lazy initialization to avoid this overhead.
    if(!self.is_void() && !(self.is_constant() && self.read().is_null()))
      this->open_named_reference(::rocket::sref("__this")) = ::std::move(self);

    // This is the subscript of the special parameter placeholder `...`.
    size_t elps = SIZE_MAX;

    // Set parameters, which are local references.
    for(size_t i = 0;  i < params.size();  ++i) {
      const auto& name = params.at(i);
      if(name.empty())
        continue;

      if(name.rdstr().starts_with("__"))
        ASTERIA_THROW("Reserved name not declarable as parameter (name `$1`)", name);

      if(name == "...") {
        // Nothing is set for the parameter placeholder, but the parameter list terminates here.
        ROCKET_ASSERT(i == params.size() - 1);
        elps = i;
        break;
      }
      // Set the parameter.
      if(ROCKET_UNEXPECT(i >= args.size()))
        this->open_named_reference(name) = Reference::S_constant();
      else
        this->open_named_reference(name) = ::std::move(args.mut(i));
    }

    // Disallow exceess arguments if the function is not variadic.
    if((elps == SIZE_MAX) && (args.size() > params.size())) {
      ASTERIA_THROW("Too many arguments (`$1` > `$2`)", args.size(), params.size());
    }
    args.erase(0, elps);

    // Stash variadic arguments for lazy initialization.
    // If all arguments are positional, `args` may be reused for the evaluation stack, so don't move it.
    if(args.size())
      this->m_lazy_args = ::std::move(args);
  }

Reference*
Executive_Context::
do_lazy_lookup_opt(const phsh_string& name)
  {
    // Create pre-defined references as needed.
    // N.B. If you have ever changed these, remember to update 'analytic_context.cpp' as well.
    if(name == "__func") {
      // Note: This can only happen inside a function context.
      Reference::S_constant xref = { this->m_zvarg->func() };
      return do_set_lazy_reference(*this, name, ::std::move(xref));
    }

    if(name == "__this") {
      // Note: This can only happen inside a function context and the `this` argument is null.
      Reference::S_constant xref = { };
      return do_set_lazy_reference(*this, name, ::std::move(xref));
    }

    if(name == "__varg") {
      // Note: This can only happen inside a function context.
      cow_function varg;
      if(ROCKET_EXPECT(this->m_lazy_args.size()))
        varg = ::rocket::make_refcnt<Variadic_Arguer>(*(this->m_zvarg), ::std::move(this->m_lazy_args));
      else
        varg = this->m_zvarg;

      Reference::S_constant xref = { ::std::move(varg) };
      return do_set_lazy_reference(*this, name, ::std::move(xref));
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
on_scope_exit(AIR_Status status)
  {
    if(ROCKET_EXPECT(this->m_defer.empty()))
      return status;

    // Stash the returned reference, if any.
    Reference self = Reference::S_uninit();
    if(status == air_status_return_ref)
      self = ::std::move(this->m_stack.get().get_top());

    if(auto ptca = self.get_ptc_args_opt()) {
      // If a PTC wrapper was returned, prepend all deferred expressions to it.
      // These callbacks will be unpacked later, so we just return.
      do_concatenate(ptca->open_defer_stack(), ::std::move(this->m_defer));
      ROCKET_ASSERT(!self.is_uninit());
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

    // Restore the returned reference.
    this->m_stack.get().open_top() = ::std::move(self);
    return status;
  }

Runtime_Error&
Executive_Context::
on_scope_exit(Runtime_Error& except)
  {
    if(ROCKET_EXPECT(this->m_defer.empty()))
      return except;

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
