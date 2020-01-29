// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "executive_context.hpp"
#include "air_node.hpp"
#include "runtime_error.hpp"
#include "tail_call_arguments.hpp"
#include "../llds/avmc_queue.hpp"
#include "../utilities.hpp"

namespace Asteria {

Executive_Context::~Executive_Context()
  {
  }

void Executive_Context::do_bind_parameters(const cow_vector<phsh_string>& params, cow_vector<Reference>&& args)
  {
    // This is the subscript of the special parameter placeholder `...`.
    size_t elps = SIZE_MAX;
    // Set parameters, which are local references.
    for(size_t i = 0;  i < params.size();  ++i) {
      const auto& name = params.at(i);
      if(name.empty()) {
        continue;
      }
      if(name == "...") {
        // Nothing is set for the parameter placeholder, but the parameter list terminates here.
        ROCKET_ASSERT(i == params.size() - 1);
        elps = i;
        break;
      }
      if(name.rdstr().starts_with("__")) {
        ASTERIA_THROW("reserved name not declarable as parameter (name `$1`)", name);
      }
      // Set the parameter.
      if(ROCKET_UNEXPECT(i >= args.size()))
        this->open_named_reference(name) = Reference_Root::S_constant();
      else
        this->open_named_reference(name) = ::rocket::move(args.mut(i));
    }
    if((elps == SIZE_MAX) && (args.size() > params.size())) {
      // Disallow exceess arguments if the function is not variadic.
      ASTERIA_THROW("too many arguments (`$1` > `$2`)", args.size(), params.size());
    }
    args.erase(0, elps);
    // Stash variadic arguments for lazy initialization.
    // If all arguments are positional, `args` may be reused for the evaluation stack, so don't move it at all.
    if(!args.empty())
      this->m_args = ::rocket::move(args);
  }

void Executive_Context::do_defer_expression(const Source_Location& sloc, const cow_vector<AIR_Node>& code)
  {
    AVMC_Queue queue;
    // Solidify the expression.
    ::rocket::for_each(code, [&](const AIR_Node& node) { node.solidify(queue, 0);  });  // 1st pass
    ::rocket::for_each(code, [&](const AIR_Node& node) { node.solidify(queue, 1);  });  // 2nd pass
    // Append it.
    this->m_defer.emplace_back(sloc, ::rocket::move(queue));
  }

void Executive_Context::do_on_scope_exit_void()
  {
    // Execute all deferred expressions backwards.
    while(this->m_defer.size()) {
      // Pop an expression.
      auto pair = ::rocket::move(this->m_defer.mut_back());
      this->m_defer.pop_back();
      // Execute it. If an exception is thrown, append a frame and rethrow it.
      ASTERIA_RUNTIME_TRY {
        auto status = pair.second.execute(*this);
        ROCKET_ASSERT(status == air_status_next);
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
        except.push_frame_defer(pair.first, except.value());
        this->do_on_scope_exit_exception(except);
        throw;
      }
    }
    ROCKET_ASSERT(this->m_defer.empty());
  }

void Executive_Context::do_on_scope_exit_return()
  {
    // Stash the return reference.
    this->m_self = this->m_stack->get_top();
    // If a PTC wrapper is returned, prepend all deferred expressions to it.
    if(auto tca = this->m_self.get_tail_call_opt()) {
      // Take advantage of reference counting.
      auto& defer = tca->open_defer_stack();
      if(defer.empty()) {
        defer.swap(this->m_defer);
      }
      else {
        defer.insert(defer.begin(), ::std::make_move_iterator(this->m_defer.mut_begin()),
                                    ::std::make_move_iterator(this->m_defer.mut_end()));
        this->m_defer.clear();
      }
      return;
    }
    // Evaluate all deferred expressions.
    this->do_on_scope_exit_void();
    // Restore the return reference.
    this->m_stack->clear();
    this->m_stack->push(::rocket::move(this->m_self));
  }

void Executive_Context::do_on_scope_exit_exception(Runtime_Error& except)
  {
    // Execute all deferred expressions backwards.
    while(this->m_defer.size()) {
      // Pop an expression.
      auto pair = ::rocket::move(this->m_defer.mut_back());
      this->m_defer.pop_back();
      // Execute it. If an exception is thrown, replace `except` with it.
      ASTERIA_RUNTIME_TRY {
        auto status = pair.second.execute(*this);
        ROCKET_ASSERT(status == air_status_next);
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& nested) {
        except = nested;
        except.push_frame_defer(pair.first, nested.value());
      }
    }
    ROCKET_ASSERT(this->m_defer.empty());
  }

bool Executive_Context::do_is_analytic() const noexcept
  {
    return this->is_analytic();
  }

const Abstract_Context* Executive_Context::do_get_parent_opt() const noexcept
  {
    return this->get_parent_opt();
  }

Reference* Executive_Context::do_lazy_lookup_opt(const phsh_string& name)
  {
    // Create pre-defined references as needed.
    // N.B. If you have ever changed these, remember to update 'analytic_context.cpp' as well.
    if(name == "__func") {
      auto& ref = this->open_named_reference(::rocket::sref("__func"));
      // Copy the function name as a constant.
      Reference_Root::S_constant xref = { this->m_zvarg.get()->func() };
      ref = ::rocket::move(xref);
      return &ref;
    }
    if(name == "__this") {
      auto& ref = this->open_named_reference(::rocket::sref("__this"));
      // Copy the `this` reference.
      ref = ::rocket::move(this->m_self);
      return &ref;
    }
    if(name == "__varg") {
      auto& ref = this->open_named_reference(::rocket::sref("__varg"));
      // Use the pre-allocated zero-ary variadic argument getter if there is no variadic argument,
      // or allocate a new one if there is.
      auto varg = this->m_args.empty() ? rcptr<Abstract_Function>(this->m_zvarg.get())
                                       : ::rocket::make_refcnt<Variadic_Arguer>(*(this->m_zvarg.get()),
                                                                                ::rocket::move(this->m_args));
      Reference_Root::S_constant xref = { ::rocket::move(varg) };
      ref = ::rocket::move(xref);
      return &ref;
    }
    return nullptr;
  }

}  // namespace Asteria
