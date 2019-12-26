// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "evaluation_stack.hpp"
#include "analytic_context.hpp"
#include "instantiated_function.hpp"
#include "runtime_error.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

using ParamU      = AVMC_Queue::ParamU;
using Executor    = AVMC_Queue::Executor;
using Enumerator  = AVMC_Queue::Enumerator;

///////////////////////////////////////////////////////////////////////////
// Auxiliary functions
///////////////////////////////////////////////////////////////////////////

bool do_rebind_nodes(bool& dirty, cow_vector<AIR_Node>& code, const Abstract_Context& ctx)
  {
    for(size_t i = 0; i != code.size(); ++i) {
      auto qnode = code.at(i).rebind_opt(ctx);
      if(!qnode) {
        continue;
      }
      dirty |= true;
      code.mut(i) = ::rocket::move(*qnode);
    }
    return dirty;
  }

bool do_rebind_nodes(bool& dirty, cow_vector<cow_vector<AIR_Node>>& code, const Abstract_Context& ctx)
  {
    for(size_t i = 0; i != code.size(); ++i) {
      for(size_t k = 0; k != code.at(i).size(); ++k) {
        auto qnode = code.at(i).at(k).rebind_opt(ctx);
        if(!qnode) {
          continue;
        }
        dirty |= true;
        code.mut(i).mut(k) = ::rocket::move(*qnode);
      }
    }
    return dirty;
  }

template<typename XValT> Reference& do_set_temporary(Evaluation_Stack& stack, bool assign, XValT&& xval)
  {
    auto& ref = stack.open_top();
    if(assign) {
      // Write the value to the top refernce.
      ref.open() = ::rocket::forward<XValT>(xval);
      return ref;
    }
    // Replace the top reference with a temporary reference to the value.
    Reference_Root::S_temporary xref = { ::rocket::forward<XValT>(xval) };
    return ref = ::rocket::move(xref);
  }

AIR_Status do_execute_block(const AVMC_Queue& queue, /*const*/ Executive_Context& ctx)
  {
    if(ROCKET_EXPECT(queue.empty())) {
      // Don't bother creating a context.
      return air_status_next;
    }
    // Execute the queue on a new context.
    Executive_Context ctx_next(::rocket::ref(ctx));
    auto status = queue.execute(ctx_next);
    // Forward the status as is.
    return status;
  }

AIR_Status do_evaluate_branch(const AVMC_Queue& queue, bool assign, /*const*/ Executive_Context& ctx)
  {
    if(ROCKET_EXPECT(queue.empty())) {
      // Leave the condition on the top of the stack.
      return air_status_next;
    }
    if(assign) {
      // Evaluate the branch.
      auto status = queue.execute(ctx);
      ROCKET_ASSERT(status == air_status_next);
      // Read a value from the top reference and write it to the one beneath it.
      ctx.stack().get_top(1).open() = ctx.stack().get_top().read();
      // Discard the reference whose value has just been copied from.
      ctx.stack().pop();
      return air_status_next;
    }
    // Discard the top which will be overwritten anyway.
    ctx.stack().pop();
    // Evaluate the branch.
    // Be advised that you must forward the status code as is, because PTCs may return `air_status_return`.
    return queue.execute(ctx);
  }

AIR_Status do_execute_catch(const AVMC_Queue& queue, const phsh_string& name_except, const Runtime_Error& except, const Executive_Context& ctx)
  {
    if(ROCKET_EXPECT(queue.empty())) {
      return air_status_next;
    }
    // Execute the queue on a new context.
    Executive_Context ctx_next(::rocket::ref(ctx));
    // Set the exception reference.
    {
      Reference_Root::S_temporary xref = { except.value() };
      ctx_next.open_named_reference(name_except) = ::rocket::move(xref);
    }
    // Set backtrace frames.
    {
      G_array backtrace;
      G_object r;
      for(size_t i = 0; i != except.count_frames(); ++i) {
        const auto& frame = except.frame(i);
        // Translate each frame into a human-readable format.
        r.clear();
        r.try_emplace(::rocket::sref("frame"), G_string(::rocket::sref(frame.what_type())));
        r.try_emplace(::rocket::sref("file"), G_string(frame.file()));
        r.try_emplace(::rocket::sref("line"), G_integer(frame.line()));
        r.try_emplace(::rocket::sref("value"), frame.value());
        // Append this frame.
        backtrace.emplace_back(::rocket::move(r));
      }
      Reference_Root::S_constant xref = { ::rocket::move(backtrace) };
      ctx_next.open_named_reference(::rocket::sref("__backtrace")) = ::rocket::move(xref);
    }
    return queue.execute(ctx_next);
  }

template<typename ParamV> inline const ParamV* do_pcast(const void* pv) noexcept
  {
    return static_cast<const ParamV*>(pv);
  }

template<typename ParamV> Variable_Callback& do_penum(Variable_Callback& callback, ParamU /*pu*/, const void* pv)
  {
    return static_cast<const ParamV*>(pv)->enumerate_variables(callback);
  }

// This is the trait struct for parameter types that implement `enumerate_variables()`.
template<typename ParamV, typename = void> struct AVMC_Appender : ParamV
  {
    ParamU pu;

    constexpr AVMC_Appender()
      :
        ParamV(), pu()
      {
      }

    AVMC_Queue& request(AVMC_Queue& queue) const
      {
        return queue.request(sizeof(ParamV));
      }
    template<Executor executorT> AVMC_Queue& output(AVMC_Queue& queue)
      {
        return queue.append<executorT, do_penum<ParamV>>(this->pu, static_cast<ParamV&&>(*this));
      }
  };

// This is the trait struct for parameter types that do not implement `enumerate_variables()`.
template<typename ParamV> struct AVMC_Appender<ParamV, ASTERIA_VOID_T(typename ParamV::nonenumerable)> : ParamV
  {
    ParamU pu;

    constexpr AVMC_Appender()
      :
        ParamV(), pu()
      {
      }

    AVMC_Queue& request(AVMC_Queue& queue) const
      {
        return queue.request(sizeof(ParamV));
      }
    template<Executor executorT> AVMC_Queue& output(AVMC_Queue& queue)
      {
        return queue.append<executorT>(this->pu, static_cast<ParamV&&>(*this));
      }
  };

// This is the trait struct when there is no parameter.
template<> struct AVMC_Appender<void, void>
  {
    ParamU pu;

    constexpr AVMC_Appender()
      :
        pu()
      {
      }

    AVMC_Queue& request(AVMC_Queue& queue) const
      {
        return queue.request(0);
      }
    template<Executor executorT> AVMC_Queue& output(AVMC_Queue& queue)
      {
        return queue.append<executorT>(this->pu);
      }
  };

AVMC_Queue& do_solidify_queue(AVMC_Queue& queue, const cow_vector<AIR_Node>& code)
  {
    ::rocket::for_each(code, [&](const AIR_Node& node) { node.solidify(queue, 0);  });  // 1st pass
    ::rocket::for_each(code, [&](const AIR_Node& node) { node.solidify(queue, 1);  });  // 2nd pass
    return queue;
  }

///////////////////////////////////////////////////////////////////////////
// Parameter structs and variable enumeration callbacks
///////////////////////////////////////////////////////////////////////////

template<size_t nqsT> struct Pv_queues_fixed
  {
    AVMC_Queue queues[nqsT];

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const
      {
        ::rocket::for_each(this->queues, [&](const AVMC_Queue& queue) { queue.enumerate_variables(callback);  });
        return callback;
      }
  };

struct Pv_switch
  {
    cow_vector<AVMC_Queue> queues_labels;
    cow_vector<AVMC_Queue> queues_bodies;
    cow_vector<cow_vector<phsh_string>> names_added;

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const
      {
        ::rocket::for_each(this->queues_labels, [&](const AVMC_Queue& queue) { queue.enumerate_variables(callback);  });
        ::rocket::for_each(this->queues_bodies, [&](const AVMC_Queue& queue) { queue.enumerate_variables(callback);  });
        return callback;
      }
  };

struct Pv_for_each
  {
    phsh_string name_key;
    phsh_string name_mapped;
    AVMC_Queue queue_init;
    AVMC_Queue queue_body;

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const
      {
        this->queue_init.enumerate_variables(callback);
        this->queue_body.enumerate_variables(callback);
        return callback;
      }
  };

struct Pv_try
  {
    AVMC_Queue queue_try;
    Source_Location sloc;
    phsh_string name_except;
    AVMC_Queue queue_catch;

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const
      {
        this->queue_try.enumerate_variables(callback);
        this->queue_catch.enumerate_variables(callback);
        return callback;
      }
  };

struct Pv_func
  {
    Source_Location sloc;
    cow_string func;
    cow_vector<phsh_string> params;
    cow_vector<AIR_Node> code_body;

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const
      {
        ::rocket::for_each(this->code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
  };

struct Pv_name
  {
    phsh_string name;

    using nonenumerable = ::std::true_type;
  };

struct Pv_names
  {
    cow_vector<phsh_string> names;

    using nonenumerable = ::std::true_type;
  };

struct Pv_sloc
  {
    Source_Location sloc;

    using nonenumerable = ::std::true_type;
  };

struct Pv_sloc_name
  {
    Source_Location sloc;
    phsh_string name;

    using nonenumerable = ::std::true_type;
  };

struct Pv_sloc_msg
  {
    Source_Location sloc;
    cow_string msg;

    using nonenumerable = ::std::true_type;
  };

struct Pv_call
  {
    Source_Location sloc;
    cow_vector<bool> args_by_refs;

    using nonenumerable = ::std::true_type;
  };

///////////////////////////////////////////////////////////////////////////
// Executor functions
///////////////////////////////////////////////////////////////////////////

AIR_Status do_clear_stack(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // Clear the stack.
    ctx.stack().clear();
    return air_status_next;
  }

AIR_Status do_execute_block(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& queue_body = do_pcast<Pv_queues_fixed<1>>(pv)->queues[0];

    // Execute the body on a new context.
    return do_execute_block(queue_body, ctx);
  }

AIR_Status do_declare_variable(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc_name>(pv)->sloc;
    const auto& name = do_pcast<Pv_sloc_name>(pv)->name;
    const auto& inside = ctx.zvarg().func();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Allocate a variable and initialize it to `null`.
    auto var = ctx.global().create_variable();
    var->reset(nullptr, true);
    // Call the hook function if any.
    if(qhooks) {
      qhooks->on_variable_declare(sloc, inside, name);
    }
    // Inject the variable into the current context.
    Reference_Root::S_variable xref = { ::rocket::move(var) };
    ctx.open_named_reference(name) = xref;
    // Push a copy of the reference onto the stack.
    ctx.stack().push(::rocket::move(xref));
    return air_status_next;
  }

AIR_Status do_initialize_variable(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& immutable = static_cast<bool>(pu.u8s[0]);

    // Read the value of the initializer.
    // Note that the initializer must not have been empty for this function.
    auto val = ctx.stack().get_top().read();
    ctx.stack().pop();
    // Get the variable back.
    auto var = ctx.stack().get_top().get_variable_opt();
    ROCKET_ASSERT(var);
    ROCKET_ASSERT(var->get_value().is_null());
    ctx.stack().pop();
    // Initialize it.
    var->reset(::rocket::move(val), immutable);
    return air_status_next;
  }

AIR_Status do_if_statement(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& negative = static_cast<bool>(pu.u8s[0]);
    const auto& queue_true = do_pcast<Pv_queues_fixed<2>>(pv)->queues[0];
    const auto& queue_false = do_pcast<Pv_queues_fixed<2>>(pv)->queues[1];

    // Check the value of the condition.
    if(ctx.stack().get_top().read().test() != negative) {
      // Execute the true branch.
      return do_execute_block(queue_true, ctx);
    }
    // Execute the false branch.
    return do_execute_block(queue_false, ctx);
  }

AIR_Status do_switch_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& queues_labels = do_pcast<Pv_switch>(pv)->queues_labels;
    const auto& queues_bodies = do_pcast<Pv_switch>(pv)->queues_bodies;
    const auto& names_added = do_pcast<Pv_switch>(pv)->names_added;

    // Read the value of the condition.
    auto value = ctx.stack().get_top().read();
    // Get the number of clauses.
    auto nclauses = queues_labels.size();
    ROCKET_ASSERT(nclauses == queues_bodies.size());
    ROCKET_ASSERT(nclauses == names_added.size());
    // Find a target clause.
    // This is different from the `switch` statement in C, where `case` labels must have constant operands.
    size_t target = SIZE_MAX;
    for(size_t i = 0; i != nclauses; ++i) {
      if(queues_labels[i].empty()) {
        // This is a `default` label.
        if(target != SIZE_MAX) {
          ASTERIA_THROW("multiple `default` clauses");
        }
        target = i;
        continue;
      }
      // This is a `case` label.
      // Evaluate the operand and check whether it equals `value`.
      auto status = queues_labels[i].execute(ctx);
      ROCKET_ASSERT(status == air_status_next);
      if(ctx.stack().get_top().read().compare(value) == compare_equal) {
        target = i;
        break;
      }
    }
    if(ROCKET_EXPECT(target == SIZE_MAX)) {
      // No matching clause has been found.
      return air_status_next;
    }

    // Jump to the clause denoted by `target`.
    // Note that all clauses share the same context.
    Executive_Context ctx_body(::rocket::ref(ctx));
    // Fly over all clauses that precede `target`.
    for(size_t i = 0; i != target; ++i) {
      ::rocket::for_each(names_added[i], [&](const phsh_string& name) { ctx_body.open_named_reference(name) = Reference_Root::S_null();  });
    }
    // Execute all clauses from `target`.
    for(size_t i = target; i != nclauses; ++i) {
      auto status = queues_bodies[i].execute(ctx_body);
      if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_switch })) {
        break;
      }
      if(status != air_status_next) {
        return status;
      }
    }
    return air_status_next;
  }

AIR_Status do_do_while_statement(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& queue_body = do_pcast<Pv_queues_fixed<2>>(pv)->queues[0];
    const auto& negative = static_cast<bool>(pu.u8s[0]);
    const auto& queue_cond = do_pcast<Pv_queues_fixed<2>>(pv)->queues[1];

    // This is the same as the `do...while` statement in C.
    for(;;) {
      // Execute the body.
      auto status = do_execute_block(queue_body, ctx);
      if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while })) {
        break;
      }
      if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_while })) {
        return status;
      }
      // Check the condition.
      ctx.stack().clear();
      status = queue_cond.execute(ctx);
      ROCKET_ASSERT(status == air_status_next);
      if(ctx.stack().get_top().read().test() == negative) {
        break;
      }
    }
    return air_status_next;
  }

AIR_Status do_while_statement(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& negative = static_cast<bool>(pu.u8s[0]);
    const auto& queue_cond = do_pcast<Pv_queues_fixed<2>>(pv)->queues[0];
    const auto& queue_body = do_pcast<Pv_queues_fixed<2>>(pv)->queues[1];

    // This is the same as the `while` statement in C.
    for(;;) {
      // Check the condition.
      ctx.stack().clear();
      auto status = queue_cond.execute(ctx);
      ROCKET_ASSERT(status == air_status_next);
      if(ctx.stack().get_top().read().test() == negative) {
        break;
      }
      // Execute the body.
      status = do_execute_block(queue_body, ctx);
      if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while })) {
        break;
      }
      if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_while })) {
        return status;
      }
    }
    return air_status_next;
  }

AIR_Status do_for_each_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& name_key = do_pcast<Pv_for_each>(pv)->name_key;
    const auto& name_mapped = do_pcast<Pv_for_each>(pv)->name_mapped;
    const auto& queue_init = do_pcast<Pv_for_each>(pv)->queue_init;
    const auto& queue_body = do_pcast<Pv_for_each>(pv)->queue_body;

    // We have to create an outer context due to the fact that the key and mapped references outlast every iteration.
    Executive_Context ctx_for(::rocket::ref(ctx));
    // Create a variable for the key.
    // Important: As long as this variable is immutable, there cannot be circular reference.
    auto key = ::rocket::make_refcnt<Variable>();
    {
      Reference_Root::S_variable xref = { key };
      ctx_for.open_named_reference(name_key) = ::rocket::move(xref);
    }
    // Create the mapped reference.
    auto& mapped = ctx_for.open_named_reference(name_mapped);
    mapped = Reference_Root::S_null();
    // Evaluate the range initializer.
    ctx_for.stack().clear();
    auto status = queue_init.execute(ctx_for);
    ROCKET_ASSERT(status == air_status_next);
    // Set the range up.
    mapped = ::rocket::move(ctx_for.stack().open_top());
    auto range = mapped.read();

    // The range value has been saved.
    // We are immune to dangling pointers in case the object being iterated is modified by the loop body.
    if(range.is_array()) {
      const auto& array = range.as_array();
      // The key is the subscript of an element of the array.
      for(ptrdiff_t i = 0; i != array.ssize(); ++i) {
        // Set up the key.
        key->reset(G_integer(i), true);
        // Be advised that the mapped parameter is a reference rather than a value.
        {
          Reference_Modifier::S_array_index xmod = { i };
          mapped.zoom_in(::rocket::move(xmod));
        }
        // Execute the loop body.
        status = do_execute_block(queue_body, ctx_for);
        if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
          break;
        }
        if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for })) {
          return status;
        }
        // Restore the mapped reference.
        mapped.zoom_out();
      }
    }
    else if(range.is_object()) {
      const auto& object = range.as_object();
      // The key is a string.
      for(auto q = object.begin(); q != object.end(); ++q) {
        // Set up the key.
        key->reset(G_string(q->first), true);
        // Be advised that the mapped parameter is a reference rather than a value.
        {
          Reference_Modifier::S_object_key xmod = { q->first };
          mapped.zoom_in(::rocket::move(xmod));
        }
        // Execute the loop body.
        status = do_execute_block(queue_body, ctx_for);
        if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
          break;
        }
        if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for })) {
          return status;
        }
        // Restore the mapped reference.
        mapped.zoom_out();
      }
    }
    else {
      ASTERIA_THROW("range value not iterable (range `$1`)", range);
    }
    return air_status_next;
  }

AIR_Status do_for_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& queue_init = do_pcast<Pv_queues_fixed<4>>(pv)->queues[0];
    const auto& queue_cond = do_pcast<Pv_queues_fixed<4>>(pv)->queues[1];
    const auto& queue_step = do_pcast<Pv_queues_fixed<4>>(pv)->queues[2];
    const auto& queue_body = do_pcast<Pv_queues_fixed<4>>(pv)->queues[3];

    // This is the same as the `for` statement in C.
    // We have to create an outer context due to the fact that names declared in the first segment outlast every iteration.
    Executive_Context ctx_for(::rocket::ref(ctx));
    // Execute the loop initializer, which shall only be a definition or an expression statement.
    auto status = queue_init.execute(ctx_for);
    ROCKET_ASSERT(status == air_status_next);
    for(;;) {
      // This is a special case: If the condition is empty then the loop is infinite.
      if(!queue_cond.empty()) {
        // Check the condition.
        ctx_for.stack().clear();
        status = queue_cond.execute(ctx_for);
        ROCKET_ASSERT(status == air_status_next);
        if(ctx_for.stack().get_top().read().test() == false) {
          break;
        }
      }
      // Execute the body.
      status = do_execute_block(queue_body, ctx_for);
      if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
        break;
      }
      if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for })) {
        return status;
      }
      // Execute the increment.
      ctx_for.stack().clear();
      status = queue_step.execute(ctx_for);
      ROCKET_ASSERT(status == air_status_next);
    }
    return air_status_next;
  }

AIR_Status do_try_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& queue_try = do_pcast<Pv_try>(pv)->queue_try;
    const auto& sloc = do_pcast<Pv_try>(pv)->sloc;
    const auto& name_except = do_pcast<Pv_try>(pv)->name_except;
    const auto& queue_catch = do_pcast<Pv_try>(pv)->queue_catch;

    // This is almost identical to JavaScript.
    try {
      // Execute the `try` block. If no exception is thrown, this will have little overhead.
      auto status = do_execute_block(queue_try, ctx);
      if(status == air_status_return) {
        // This cannot be PTC'd, otherwise exceptions thrown from tail calls won't be caught.
        ctx.stack().open_top().finish_call(ctx.global());
      }
      return status;
    }
    catch(Runtime_Error& except) {
      // Reuse the exception object. Don't bother allocating a new one.
      except.push_frame_catch(sloc);
      // This branch must be executed inside this `catch` block.
      // User-provided bindings may obtain the current exception using `::std::current_exception`.
      return do_execute_catch(queue_catch, name_except, except, ctx);
    }
    catch(::std::exception& stdex) {
      // Translate the exception.
      Runtime_Error except(stdex);
      except.push_frame_catch(sloc);
      // This branch must be executed inside this `catch` block.
      // User-provided bindings may obtain the current exception using `::std::current_exception`.
      return do_execute_catch(queue_catch, name_except, except, ctx);
    }
  }

AIR_Status do_throw_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc>(pv)->sloc;

    // Read the value to throw.
    // Note that the operand must not have been empty for this code.
    auto value = ctx.stack().get_top().read();
    try {
      // Unpack nested exceptions, if any.
      auto eptr = ::std::current_exception();
      if(eptr) {
        ::std::rethrow_exception(eptr);
      }
    }
    catch(Runtime_Error& except) {
      // Modify it in place. Don't bother allocating a new one.
      except.push_frame_throw(sloc, ::rocket::move(value));
      throw;
    }
    catch(::std::exception& stdex) {
      // Translate the exception.
      Runtime_Error except(stdex);
      except.push_frame_throw(sloc, ::rocket::move(value));
      throw except;
    }
    // If no nested exception exists, construct a fresh one.
    Runtime_Error except(sloc, ::rocket::move(value));
    throw except;
  }

AIR_Status do_assert_statement(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc_msg>(pv)->sloc;
    const auto& negative = static_cast<bool>(pu.u8s[0]);
    const auto& msg = do_pcast<Pv_sloc_msg>(pv)->msg;

    // Check the value of the condition.
    if(ROCKET_EXPECT(ctx.stack().get_top().read().test() != negative)) {
      // When the assertion succeeds, there is nothing to do.
      return air_status_next;
    }
    // Throw a `runtime_error`.
    ASTERIA_THROW("assertion failure: $1\n[declared at '$2']", msg, sloc);
  }

AIR_Status do_simple_status(Executive_Context& /*ctx*/, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& status = static_cast<AIR_Status>(pu.u8s[0]);

    // Return the status as is.
    return status;
  }

AIR_Status do_return_by_value(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // The result will have been pushed onto the top.
    auto& self = ctx.stack().open_top();
    // Convert the result to an rvalue.
    // PTC wrappers are forwarded as is.
    if(ROCKET_UNEXPECT(self.is_lvalue())) {
      self.convert_to_rvalue();
    }
    return air_status_return;
  }

AIR_Status do_push_immediate(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& val = do_pcast<Value>(pv)[0];

    // Push a constant.
    Reference_Root::S_constant xref = { val };
    ctx.stack().push(::rocket::move(xref));
    return air_status_next;
  }

AIR_Status do_push_global_reference(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& name = do_pcast<Pv_name>(pv)->name;

    // Look for the name in the global context.
    auto qref = ctx.global().get_named_reference_opt(name);
    if(!qref) {
      ASTERIA_THROW("undeclared identifier `$1`", name);
    }
    // Push a copy of it.
    ctx.stack().push(*qref);
    return air_status_next;
  }

AIR_Status do_push_local_reference(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& depth = pu.x32;
    const auto& name = do_pcast<Pv_name>(pv)->name;

    // Get the context.
    const Executive_Context* qctx = ::std::addressof(ctx);
    ::rocket::ranged_for(uint32_t(0), depth, [&](uint32_t) { qctx = qctx->get_parent_opt();  });
    ROCKET_ASSERT(qctx);
    // Look for the name in the context.
    auto qref = qctx->get_named_reference_opt(name);
    if(!qref) {
      ASTERIA_THROW("undeclared identifier `$1`", name);
    }
    // Push a copy of it.
    ctx.stack().push(*qref);
    return air_status_next;
  }

AIR_Status do_push_bound_reference(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& ref = do_pcast<Reference>(pv)[0];

    // Push a copy of the bound reference.
    ctx.stack().push(ref);
    return air_status_next;
  }

AIR_Status do_define_function(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_func>(pv)->sloc;
    const auto& func = do_pcast<Pv_func>(pv)->func;
    const auto& params = do_pcast<Pv_func>(pv)->params;
    const auto& code_body = do_pcast<Pv_func>(pv)->code_body;

    // Create the zero-ary argument getter, which serves two purposes:
    // 0) It is copied as `__varg` whenever its parent function is called with no variadic argument as an optimization.
    // 1) It provides storage for `__file`, `__line` and `__func` for its parent function.
    auto zvarg = ::rocket::make_refcnt<Variadic_Arguer>(sloc, func);
    // Rewrite nodes in the body as necessary.
    // Don't trigger copy-on-write unless a node needs rewriting.
    Analytic_Context ctx_func(::std::addressof(ctx), params);
    bool dirty = false;
    auto code_bound = code_body;
    do_rebind_nodes(dirty, code_bound, ctx_func);
    // Instantiate the function.
    auto qtarget = ::rocket::make_refcnt<Instantiated_Function>(params, ::rocket::move(zvarg), code_bound);
    // Push the function as a temporary.
    Reference_Root::S_temporary xref = { G_function(::rocket::move(qtarget)) };
    ctx.stack().push(::rocket::move(xref));
    return air_status_next;
  }

AIR_Status do_branch_expression(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);
    const auto& queue_true = do_pcast<Pv_queues_fixed<2>>(pv)->queues[0];
    const auto& queue_false = do_pcast<Pv_queues_fixed<2>>(pv)->queues[1];

    // Check the value of the condition.
    if(ctx.stack().get_top().read().test() != false) {
      // Evaluate the true branch.
      return do_evaluate_branch(queue_true, assign, ctx);
    }
    // Evaluate the false branch.
    return do_evaluate_branch(queue_false, assign, ctx);
  }

AIR_Status do_coalescence(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);
    const auto& queue_null = do_pcast<Pv_queues_fixed<1>>(pv)->queues[0];

    // Check the value of the condition.
    if(ctx.stack().get_top().read().is_null() != false) {
      // Evaluate the alternative.
      return do_evaluate_branch(queue_null, assign, ctx);
    }
    // Leave the condition on the stack.
    return air_status_next;
  }

ROCKET_NOINLINE ckptr<Abstract_Function> do_pop_arguments(cow_vector<Reference>& args, Executive_Context& ctx,
                                                          const cow_vector<bool>& args_by_refs)
  {
    // Pop arguments off the stack backwards.
    args.resize(args_by_refs.size());
    for(size_t i = args.size() - 1; i != SIZE_MAX; --i) {
      auto& arg = ctx.stack().open_top();
      // Convert the argument to an rvalue if it shouldn't be passed by reference.
      bool by_ref = args_by_refs[i];
      if(!by_ref) {
        arg.convert_to_rvalue();
      }
      // Ensure it is dereferenceable.
      arg.read();
      // Move and pop this argument.
      args.mut(i) = ::rocket::move(arg);
      ctx.stack().pop();
    }
    // Copy the target value, which shall be of type `function`.
    auto value = ctx.stack().get_top().read();
    if(!value.is_function()) {
      ASTERIA_THROW("attempt to call a non-function (value `$1`)", value);
    }
    return ::rocket::move(value.open_function());
  }

ROCKET_NOINLINE Reference& do_get_this(Executive_Context& ctx)
  {
    return ctx.stack().open_top().zoom_out();
  }

ROCKET_NOINLINE AIR_Status do_wrap_tail_call(Reference& self, const Source_Location& sloc, PTC_Aware ptc_aware,
                                             const cow_string& inside, const ckptr<Abstract_Function>& target,
                                             cow_vector<Reference>&& args)
  {
    // Pack arguments for this proper tail call.
    args.emplace_back(::rocket::move(self));
    // Create a PTC wrapper.
    auto tca = ::rocket::make_refcnt<Tail_Call_Arguments>(sloc, inside, ptc_aware, target, ::rocket::move(args));
    // Return it.
    Reference_Root::S_tail_call xref = { ::rocket::move(tca) };
    self = ::rocket::move(xref);
    // Force `air_status_return` if control flow reaches the end of a function.
    // Otherwise a null reference is returned instead of this PTC wrapper, which can then never be unpacked.
    return air_status_return;
  }

ROCKET_NOINLINE [[noreturn]] void do_xrethrow(Runtime_Error& except, const Source_Location& sloc,
                                              const cow_string& inside, const rcptr<Abstract_Hooks>& qhooks)
  {
    // Append the current frame and rethrow the exception.
    except.push_frame_func(sloc, inside);
    // Call the hook function if any.
    if(qhooks) {
      qhooks->on_function_except(sloc, inside, except);
    }
    throw;
  }

ROCKET_NOINLINE [[noreturn]] void do_xrethrow(::std::exception& stdex, const Source_Location& sloc,
                                              const cow_string& inside, const rcptr<Abstract_Hooks>& qhooks)
  {
    // Translate the exception, append the current frame, and throw the new exception.
    Runtime_Error except(stdex);
    except.push_frame_func(sloc, inside);
    // Call the hook function if any.
    if(qhooks) {
      qhooks->on_function_except(sloc, inside, except);
    }
    throw except;
  }

AIR_Status do_function_call(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_call>(pv)->sloc;
    const auto& args_by_refs = do_pcast<Pv_call>(pv)->args_by_refs;
    const auto& ptc_aware = static_cast<PTC_Aware>(pu.u8s[0]);
    const auto& inside = ctx.zvarg().func();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Check for stack overflows.
    const auto sentry = ctx.global().copy_recursion_sentry();
    // Generate a single-step trap.
    if(qhooks) {
      qhooks->on_single_step_trap(sloc, inside, ::std::addressof(ctx));
    }
    // Prepare arguments.
    cow_vector<Reference> args;
    auto target = do_pop_arguments(args, ctx, args_by_refs);
    // Initialize the `this` reference.
    auto& self = do_get_this(ctx);

    // Call the function now.
    if(ptc_aware != ptc_aware_none) {
      return do_wrap_tail_call(self, sloc, ptc_aware, inside, target, ::rocket::move(args));
    }
    // Call the hook function if any.
    if(qhooks) {
      qhooks->on_function_call(sloc, inside, target);
    }
    // Perform a non-proper call.
    try {
      target->invoke(self, ctx.global(), ::rocket::move(args));
      self.finish_call(ctx.global());
    }
    catch(Runtime_Error& except) {
      do_xrethrow(except, sloc, inside, qhooks);
    }
    catch(::std::exception& stdex) {
      do_xrethrow(stdex, sloc, inside, qhooks);
    }
    // Call the hook function if any.
    if(qhooks) {
      qhooks->on_function_return(sloc, inside, self);
    }
    return air_status_next;
  }

AIR_Status do_member_access(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& name = do_pcast<Pv_name>(pv)->name;

    // Append a modifier to the reference at the top.
    Reference_Modifier::S_object_key xmod = { name };
    ctx.stack().open_top().zoom_in(::rocket::move(xmod));
    return air_status_next;
  }

AIR_Status do_push_unnamed_array(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& nelems = pu.x32;

    // Pop elements from the stack and store them in an array backwards.
    G_array array;
    array.resize(nelems);
    for(auto it = array.mut_rbegin(); it != array.rend(); ++it) {
      // Write elements backwards.
      *it = ctx.stack().get_top().read();
      ctx.stack().pop();
    }
    // Push the array as a temporary.
    Reference_Root::S_temporary xref = { ::rocket::move(array) };
    ctx.stack().push(::rocket::move(xref));
    return air_status_next;
  }

AIR_Status do_push_unnamed_object(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& keys = do_pcast<Pv_names>(pv)->names;

    // Pop elements from the stack and store them in an object backwards.
    G_object object;
    object.reserve(keys.size());
    for(auto it = keys.rbegin(); it != keys.rend(); ++it) {
      // Use `try_emplace()` instead of `insert_or_assign()`. In case of duplicate keys, the last value takes precedence.
      object.try_emplace(*it, ctx.stack().get_top().read());
      ctx.stack().pop();
    }
    // Push the object as a temporary.
    Reference_Root::S_temporary xref = { ::rocket::move(object) };
    ctx.stack().push(::rocket::move(xref));
    return air_status_next;
  }

ROCKET_PURE_FUNCTION G_boolean do_operator_NOT(const G_boolean& rhs)
  {
    return !rhs;
  }

ROCKET_PURE_FUNCTION G_boolean do_operator_AND(const G_boolean& lhs, const G_boolean& rhs)
  {
    return lhs & rhs;
  }

ROCKET_PURE_FUNCTION G_boolean do_operator_OR(const G_boolean& lhs, const G_boolean& rhs)
  {
    return lhs | rhs;
  }

ROCKET_PURE_FUNCTION G_boolean do_operator_XOR(const G_boolean& lhs, const G_boolean& rhs)
  {
    return lhs ^ rhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_NEG(const G_integer& rhs)
  {
    if(rhs == INT64_MIN) {
      ASTERIA_THROW("integer negation overflow (operand was `$1`)", rhs);
    }
    return -rhs;
  }

ROCKET_PURE_FUNCTION G_real do_operator_SQRT(const G_integer& rhs)
  {
    return ::std::sqrt(G_real(rhs));
  }

ROCKET_PURE_FUNCTION G_boolean do_operator_ISINF(const G_integer& /*rhs*/)
  {
    return false;
  }

ROCKET_PURE_FUNCTION G_boolean do_operator_ISNAN(const G_integer& /*rhs*/)
  {
    return false;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_ABS(const G_integer& rhs)
  {
    if(rhs == INT64_MIN) {
      ASTERIA_THROW("integer absolute value overflow (operand was `$1`)", rhs);
    }
    return ::std::abs(rhs);
  }

ROCKET_PURE_FUNCTION G_integer do_operator_SIGN(const G_integer& rhs)
  {
    return rhs >> 63;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_ADD(const G_integer& lhs, const G_integer& rhs)
  {
    if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
      ASTERIA_THROW("integer addition overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return lhs + rhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_SUB(const G_integer& lhs, const G_integer& rhs)
  {
    if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
      ASTERIA_THROW("integer subtraction overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return lhs - rhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_MUL(const G_integer& lhs, const G_integer& rhs)
  {
    if((lhs == 0) || (rhs == 0)) {
      return 0;
    }
    if((lhs == 1) || (rhs == 1)) {
      return (lhs ^ rhs) ^ 1;
    }
    if((lhs == INT64_MIN) || (rhs == INT64_MIN)) {
      ASTERIA_THROW("integer multiplication overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    if((lhs == -1) || (rhs == -1)) {
      return (lhs ^ rhs) + 1;
    }
    // absolute lhs and signed rhs
    auto m = lhs >> 63;
    auto alhs = (lhs ^ m) - m;
    auto srhs = (rhs ^ m) - m;
    // `alhs` may only be positive here.
    if((srhs >= 0) ? (alhs > INT64_MAX / srhs) : (alhs > INT64_MIN / srhs)) {
      ASTERIA_THROW("integer multiplication overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return alhs * srhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_DIV(const G_integer& lhs, const G_integer& rhs)
  {
    if(rhs == 0) {
      ASTERIA_THROW("integer divided by zero (operands were `$1` and `$2`)", lhs, rhs);
    }
    if((lhs == INT64_MIN) && (rhs == -1)) {
      ASTERIA_THROW("integer division overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return lhs / rhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_MOD(const G_integer& lhs, const G_integer& rhs)
  {
    if(rhs == 0) {
      ASTERIA_THROW("integer divided by zero (operands were `$1` and `$2`)", lhs, rhs);
    }
    if((lhs == INT64_MIN) && (rhs == -1)) {
      ASTERIA_THROW("integer division overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return lhs % rhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_SLL(const G_integer& lhs, const G_integer& rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    if(rhs >= 64) {
      return 0;
    }
    return G_integer(static_cast<uint64_t>(lhs) << rhs);
  }

ROCKET_PURE_FUNCTION G_integer do_operator_SRL(const G_integer& lhs, const G_integer& rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    if(rhs >= 64) {
      return 0;
    }
    return G_integer(static_cast<uint64_t>(lhs) >> rhs);
  }

ROCKET_PURE_FUNCTION G_integer do_operator_SLA(const G_integer& lhs, const G_integer& rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    if(lhs == 0) {
      return 0;
    }
    if(rhs >= 64) {
      ASTERIA_THROW("integer left shift overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    auto bc = static_cast<int>(63 - rhs);
    auto mask_out = static_cast<uint64_t>(lhs) >> bc << bc;
    auto mask_sbt = static_cast<uint64_t>(lhs >> 63) << bc;
    if(mask_out != mask_sbt) {
      ASTERIA_THROW("integer left shift overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return G_integer(static_cast<uint64_t>(lhs) << rhs);
  }

ROCKET_PURE_FUNCTION G_integer do_operator_SRA(const G_integer& lhs, const G_integer& rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    if(rhs >= 64) {
      return lhs >> 63;
    }
    return lhs >> rhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_NOT(const G_integer& rhs)
  {
    return ~rhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_AND(const G_integer& lhs, const G_integer& rhs)
  {
    return lhs & rhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_OR(const G_integer& lhs, const G_integer& rhs)
  {
    return lhs | rhs;
  }

ROCKET_PURE_FUNCTION G_integer do_operator_XOR(const G_integer& lhs, const G_integer& rhs)
  {
    return lhs ^ rhs;
  }

ROCKET_PURE_FUNCTION G_real do_operator_NEG(const G_real& rhs)
  {
    return -rhs;
  }

ROCKET_PURE_FUNCTION G_real do_operator_SQRT(const G_real& rhs)
  {
    return ::std::sqrt(rhs);
  }

ROCKET_PURE_FUNCTION G_boolean do_operator_ISINF(const G_real& rhs)
  {
    return ::std::isinf(rhs);
  }

ROCKET_PURE_FUNCTION G_boolean do_operator_ISNAN(const G_real& rhs)
  {
    return ::std::isnan(rhs);
  }

ROCKET_PURE_FUNCTION G_real do_operator_ABS(const G_real& rhs)
  {
    return ::std::fabs(rhs);
  }

ROCKET_PURE_FUNCTION G_integer do_operator_SIGN(const G_real& rhs)
  {
    return ::std::signbit(rhs) ? -1 : 0;
  }

ROCKET_PURE_FUNCTION G_real do_operator_ROUND(const G_real& rhs)
  {
    return ::std::round(rhs);
  }

ROCKET_PURE_FUNCTION G_real do_operator_FLOOR(const G_real& rhs)
  {
    return ::std::floor(rhs);
  }

ROCKET_PURE_FUNCTION G_real do_operator_CEIL(const G_real& rhs)
  {
    return ::std::ceil(rhs);
  }

ROCKET_PURE_FUNCTION G_real do_operator_TRUNC(const G_real& rhs)
  {
    return ::std::trunc(rhs);
  }

G_integer do_cast_to_integer(const G_real& value)
  {
    if(!::std::islessequal(-0x1p63, value) || !::std::islessequal(value, 0x1p63 - 0x1p10)) {
      ASTERIA_THROW("value not representable as an `integer` (operand was `$1`)", value);
    }
    return G_integer(value);
  }

ROCKET_PURE_FUNCTION G_integer do_operator_IROUND(const G_real& rhs)
  {
    return do_cast_to_integer(::std::round(rhs));
  }

ROCKET_PURE_FUNCTION G_integer do_operator_IFLOOR(const G_real& rhs)
  {
    return do_cast_to_integer(::std::floor(rhs));
  }

ROCKET_PURE_FUNCTION G_integer do_operator_ICEIL(const G_real& rhs)
  {
    return do_cast_to_integer(::std::ceil(rhs));
  }

ROCKET_PURE_FUNCTION G_integer do_operator_ITRUNC(const G_real& rhs)
  {
    return do_cast_to_integer(::std::trunc(rhs));
  }

ROCKET_PURE_FUNCTION G_real do_operator_ADD(const G_real& lhs, const G_real& rhs)
  {
    return lhs + rhs;
  }

ROCKET_PURE_FUNCTION G_real do_operator_SUB(const G_real& lhs, const G_real& rhs)
  {
    return lhs - rhs;
  }

ROCKET_PURE_FUNCTION G_real do_operator_MUL(const G_real& lhs, const G_real& rhs)
  {
    return lhs * rhs;
  }

ROCKET_PURE_FUNCTION G_real do_operator_DIV(const G_real& lhs, const G_real& rhs)
  {
    return lhs / rhs;
  }

ROCKET_PURE_FUNCTION G_real do_operator_MOD(const G_real& lhs, const G_real& rhs)
  {
    return ::std::fmod(lhs, rhs);
  }

ROCKET_PURE_FUNCTION G_string do_operator_ADD(const G_string& lhs, const G_string& rhs)
  {
    return lhs + rhs;
  }

G_string do_duplicate_string(const G_string& source, uint64_t count)
  {
    G_string res;
    auto nchars = source.size();
    if((nchars == 0) || (count == 0)) {
      return res;
    }
    if(nchars > res.max_size() / count) {
      ASTERIA_THROW("string length overflow (`$1` * `$2` > `$3`)", nchars, count, res.max_size());
    }
    auto times = static_cast<size_t>(count);
    if(nchars == 1) {
      // Fast fill.
      res.assign(times, source.front());
      return res;
    }
    // Reserve space for the result string.
    char* ptr = res.assign(nchars * times, '*').mut_data();
    // Copy the source string once.
    ::std::memcpy(ptr, source.data(), nchars);
    // Append the result string to itself, doubling its length, until more than half of the result string has been populated.
    while(nchars <= res.size() / 2) {
      ::std::memcpy(ptr + nchars, ptr, nchars);
      nchars *= 2;
    }
    // Copy remaining characters, if any.
    if(nchars < res.size()) {
      ::std::memcpy(ptr + nchars, ptr, res.size() - nchars);
      nchars = res.size();
    }
    return res;
  }

ROCKET_PURE_FUNCTION G_string do_operator_MUL(const G_string& lhs, const G_integer& rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative duplicate count (operands were `$1` and `$2`)", lhs, rhs);
    }
    return do_duplicate_string(lhs, static_cast<uint64_t>(rhs));
  }

ROCKET_PURE_FUNCTION G_string do_operator_MUL(const G_integer& lhs, const G_string& rhs)
  {
    if(lhs < 0) {
      ASTERIA_THROW("negative duplicate count (operands were `$1` and `$2`)", lhs, rhs);
    }
    return do_duplicate_string(rhs, static_cast<uint64_t>(lhs));
  }

ROCKET_PURE_FUNCTION G_string do_operator_SLL(const G_string& lhs, const G_integer& rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    G_string res;
    // Reserve space for the result string.
    char* ptr = &*(res.insert(res.begin(), lhs.size(), ' '));
    if(static_cast<uint64_t>(rhs) >= lhs.size()) {
      return res;
    }
    // Copy the substring in the right.
    size_t count = static_cast<size_t>(rhs);
    ::std::memcpy(ptr, lhs.data() + count, lhs.size() - count);
    return res;
  }

ROCKET_PURE_FUNCTION G_string do_operator_SRL(const G_string& lhs, const G_integer& rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    G_string res;
    // Reserve space for the result string.
    char* ptr = &*(res.insert(res.begin(), lhs.size(), ' '));
    if(static_cast<uint64_t>(rhs) >= lhs.size()) {
      return res;
    }
    // Copy the substring in the left.
    size_t count = static_cast<size_t>(rhs);
    ::std::memcpy(ptr + count, lhs.data(), lhs.size() - count);
    return res;
  }

ROCKET_PURE_FUNCTION G_string do_operator_SLA(const G_string& lhs, const G_integer& rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    G_string res;
    if(static_cast<uint64_t>(rhs) >= res.max_size() - lhs.size()) {
      ASTERIA_THROW("string length overflow (`$1` + `$2` > `$3`)", lhs.size(), rhs, res.max_size());
    }
    // Append spaces in the right and return the result.
    size_t count = static_cast<size_t>(rhs);
    res.assign(::rocket::sref(lhs));
    res.append(count, ' ');
    return res;
  }

ROCKET_PURE_FUNCTION G_string do_operator_SRA(const G_string& lhs, const G_integer& rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    G_string res;
    if(static_cast<uint64_t>(rhs) >= lhs.size()) {
      return res;
    }
    // Return the substring in the left.
    size_t count = static_cast<size_t>(rhs);
    res.append(lhs.data(), lhs.size() - count);
    return res;
  }

AIR_Status do_apply_xop_INC_POST(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& lhs = ctx.stack().get_top().open();
    // Increment the operand and return the old value. `assign` is ignored.
    if(lhs.is_integer()) {
      auto& reg = lhs.open_integer();
      do_set_temporary(ctx.stack(), false, ::rocket::move(lhs));
      reg = do_operator_ADD(reg, G_integer(1));
    }
    else if(lhs.is_real()) {
      auto& reg = lhs.open_real();
      do_set_temporary(ctx.stack(), false, ::rocket::move(lhs));
      reg = do_operator_ADD(reg, G_real(1));
    }
    else {
      ASTERIA_THROW("postfix increment not applicable (operand was `$1`)", lhs);
    }
    return air_status_next;
  }

AIR_Status do_apply_xop_DEC_POST(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& lhs = ctx.stack().get_top().open();
    // Decrement the operand and return the old value. `assign` is ignored.
    if(lhs.is_integer()) {
      auto& reg = lhs.open_integer();
      do_set_temporary(ctx.stack(), false, ::rocket::move(lhs));
      reg = do_operator_SUB(reg, G_integer(1));
    }
    else if(lhs.is_real()) {
      auto& reg = lhs.open_real();
      do_set_temporary(ctx.stack(), false, ::rocket::move(lhs));
      reg = do_operator_SUB(reg, G_real(1));
    }
    else {
      ASTERIA_THROW("postfix decrement not applicable (operand was `$1`)", lhs);
    }
    return air_status_next;
  }

AIR_Status do_apply_xop_SUBSCR(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    auto& lref = ctx.stack().open_top();
    // Append a reference modifier. `assign` is ignored.
    if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      Reference_Modifier::S_array_index xmod = { ::rocket::move(reg) };
      lref.zoom_in(::rocket::move(xmod));
    }
    else if(rhs.is_string()) {
      auto& reg = rhs.open_string();
      Reference_Modifier::S_object_key xmod = { ::rocket::move(reg) };
      lref.zoom_in(::rocket::move(xmod));
    }
    else {
      ASTERIA_THROW("subscript value not valid (subscript was `$1`)", rhs);
    }
    return air_status_next;
  }

AIR_Status do_apply_xop_POS(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Copy the operand to create a temporary value, then return it.
    // N.B. This is one of the few operators that work on all types.
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_NEG(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Get the opposite of the operand as a temporary value, then return it.
    if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      reg = do_operator_NEG(reg);
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_NEG(reg);
    }
    else {
      ASTERIA_THROW("prefix negation not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_NOTB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Perform bitwise NOT operation on the operand to create a temporary value, then return it.
    if(rhs.is_boolean()) {
      auto& reg = rhs.open_boolean();
      reg = do_operator_NOT(reg);
    }
    else if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      reg = do_operator_NOT(reg);
    }
    else {
      ASTERIA_THROW("prefix bitwise NOT not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_NOTL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    const auto& rhs = ctx.stack().get_top().read();
    // Perform logical NOT operation on the operand to create a temporary value, then return it.
    // N.B. This is one of the few operators that work on all types.
    do_set_temporary(ctx.stack(), assign, do_operator_NOT(rhs.test()));
    return air_status_next;
  }

AIR_Status do_apply_xop_INC_PRE(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& rhs = ctx.stack().get_top().open();
    // Increment the operand and return it. `assign` is ignored.
    if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      reg = do_operator_ADD(reg, G_integer(1));
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_ADD(reg, G_real(1));
    }
    else {
      ASTERIA_THROW("prefix increment not applicable (operand was `$1`)", rhs);
    }
    return air_status_next;
  }

AIR_Status do_apply_xop_DEC_PRE(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& rhs = ctx.stack().get_top().open();
    // Decrement the operand and return it. `assign` is ignored.
    if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      reg = do_operator_SUB(reg, G_integer(1));
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_SUB(reg, G_real(1));
    }
    else {
      ASTERIA_THROW("prefix decrement not applicable (operand was `$1`)", rhs);
    }
    return air_status_next;
  }

AIR_Status do_apply_xop_UNSET(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().unset();
    // Unset the reference and return the old value.
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_LENGTHOF(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    const auto& rhs = ctx.stack().get_top().read();
    // Return the number of elements in the operand.
    size_t nelems;
    switch(::rocket::weaken_enum(rhs.gtype())) {
    case gtype_null: {
        nelems = 0;
        break;
      }
    case gtype_string: {
        nelems = rhs.as_string().size();
        break;
      }
    case gtype_array: {
        nelems = rhs.as_array().size();
        break;
      }
    case gtype_object: {
        nelems = rhs.as_object().size();
        break;
      }
    default:
      ASTERIA_THROW("prefix `lengthof` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, G_integer(nelems));
    return air_status_next;
  }

AIR_Status do_apply_xop_TYPEOF(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    const auto& rhs = ctx.stack().get_top().read();
    // Return the type name of the operand.
    // N.B. This is one of the few operators that work on all types.
    do_set_temporary(ctx.stack(), assign, G_string(::rocket::sref(rhs.what_gtype())));
    return air_status_next;
  }

AIR_Status do_apply_xop_SQRT(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Get the square root of the operand as a temporary value, then return it.
    if(rhs.is_integer()) {
      // Note that `rhs` does not have type `G_real`, thus this branch can't be optimized.
      rhs = do_operator_SQRT(rhs.as_integer());
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_SQRT(reg);
    }
    else {
      ASTERIA_THROW("prefix `__sqrt` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_ISNAN(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Check whether the operand is a NaN, store the result in a temporary value, then return it.
    if(rhs.is_integer()) {
      // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
      rhs = do_operator_ISNAN(rhs.as_integer());
    }
    else if(rhs.is_real()) {
      // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
      rhs = do_operator_ISNAN(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__isnan` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_ISINF(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Check whether the operand is an infinity, store the result in a temporary value, then return it.
    if(rhs.is_integer()) {
      // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
      rhs = do_operator_ISINF(rhs.as_integer());
    }
    else if(rhs.is_real()) {
      // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
      rhs = do_operator_ISINF(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__isinf` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_ABS(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Get the absolute value of the operand as a temporary value, then return it.
    if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      reg = do_operator_ABS(reg);
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_ABS(reg);
    }
    else {
      ASTERIA_THROW("prefix `__abs` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_SIGN(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Get the sign bit of the operand as a temporary value, then return it.
    if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      reg = do_operator_SIGN(reg);
    }
    else if(rhs.is_real()) {
      // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
      rhs = do_operator_SIGN(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__sign` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_ROUND(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Round the operand to the nearest integer as a temporary value, then return it.
    if(rhs.is_integer()) {
      // No conversion is required.
      // Return `rhs` as is.
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_ROUND(reg);
    }
    else {
      ASTERIA_THROW("prefix `__round` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_FLOOR(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Round the operand towards negative infinity as a temporary value, then return it.
    if(rhs.is_integer()) {
      // No conversion is required.
      // Return `rhs` as is.
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_FLOOR(reg);
    }
    else {
      ASTERIA_THROW("prefix `__floor` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_CEIL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Round the operand towards negative infinity as a temporary value, then return it.
    if(rhs.is_integer()) {
      // No conversion is required.
      // Return `rhs` as is.
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_CEIL(reg);
    }
    else {
      ASTERIA_THROW("prefix `__ceil` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_TRUNC(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Round the operand towards negative infinity as a temporary value, then return it.
    if(rhs.is_integer()) {
      // No conversion is required.
      // Return `rhs` as is.
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_TRUNC(reg);
    }
    else {
      ASTERIA_THROW("prefix `__trunc` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_IROUND(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Round the operand to the nearest integer as a temporary value, then return it as an `integer`.
    if(rhs.is_integer()) {
      // No conversion is required.
      // Return `rhs` as is.
    }
    else if(rhs.is_real()) {
      // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
      rhs = do_operator_IROUND(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__iround` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_IFLOOR(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
    if(rhs.is_integer()) {
      // No conversion is required.
      // Return `rhs` as is.
    }
    else if(rhs.is_real()) {
      // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
      rhs = do_operator_IFLOOR(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__ifloor` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_ICEIL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
    if(rhs.is_integer()) {
      // No conversion is required.
      // Return `rhs` as is.
    }
    else if(rhs.is_real()) {
      // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
      rhs = do_operator_ICEIL(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__iceil` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_ITRUNC(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
    if(rhs.is_integer()) {
      // No conversion is required.
      // Return `rhs` as is.
    }
    else if(rhs.is_real()) {
      // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
      rhs = do_operator_ITRUNC(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__itrunc` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_CMP_XEQ(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);
    const auto& expect = static_cast<Compare>(pu.u8s[1]);
    const auto& negative = static_cast<bool>(pu.u8s[2]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    // Report unordered operands as being unequal.
    // N.B. This is one of the few operators that work on all types.
    auto comp = lhs.compare(rhs);
    do_set_temporary(ctx.stack(), assign, G_boolean((comp == expect) ^ negative));
    return air_status_next;
  }

AIR_Status do_apply_xop_CMP_XREL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);
    const auto& expect = static_cast<Compare>(pu.u8s[1]);
    const auto& negative = static_cast<bool>(pu.u8s[2]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    // Report unordered operands as being unequal.
    // N.B. This is one of the few operators that work on all types.
    auto comp = lhs.compare(rhs);
    if(comp == compare_unordered) {
      ASTERIA_THROW("values not comparable (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, G_boolean((comp == expect) ^ negative));
    return air_status_next;
  }

AIR_Status do_apply_xop_CMP_3WAY(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    // Report unordered operands as being unequal.
    // N.B. This is one of the few operators that work on all types.
    auto comp = lhs.compare(rhs);
    switch(comp) {
    case compare_greater: {
        do_set_temporary(ctx.stack(), assign, G_integer(+1));
        break;
      }
    case compare_less: {
        do_set_temporary(ctx.stack(), assign, G_integer(-1));
        break;
      }
    case compare_equal: {
        do_set_temporary(ctx.stack(), assign, G_integer(0));
        break;
      }
    case compare_unordered: {
        do_set_temporary(ctx.stack(), assign, G_string(::rocket::sref("<unordered>")));
        break;
      }
    default:
      ROCKET_ASSERT(false);
    }
    return air_status_next;
  }

AIR_Status do_apply_xop_ADD(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_boolean() && rhs.is_boolean()) {
      // For the `boolean` type, return the logical OR'd result of both operands.
      auto& reg = rhs.open_boolean();
      reg = do_operator_OR(lhs.as_boolean(), reg);
    }
    else if(lhs.is_integer() && rhs.is_integer()) {
      // For the `integer` and `real` types, return the sum of both operands.
      auto& reg = rhs.open_integer();
      reg = do_operator_ADD(lhs.as_integer(), reg);
    }
    else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
      // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
      rhs = do_operator_ADD(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else if(lhs.is_string() && rhs.is_string()) {
      // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
      auto& reg = rhs.open_string();
      reg = do_operator_ADD(lhs.as_string(), reg);
    }
    else {
      ASTERIA_THROW("infix addition not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_SUB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_boolean() && rhs.is_boolean()) {
      // For the `boolean` type, return the logical XOR'd result of both operands.
      auto& reg = rhs.open_boolean();
      reg = do_operator_XOR(lhs.as_boolean(), reg);
    }
    else if(lhs.is_integer() && rhs.is_integer()) {
      // For the `integer` and `real` types, return the difference of both operands.
      auto& reg = rhs.open_integer();
      reg = do_operator_SUB(lhs.as_integer(), reg);
    }
    else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
      // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
      rhs = do_operator_SUB(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else {
      ASTERIA_THROW("infix subtraction not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_MUL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_boolean() && rhs.is_boolean()) {
      // For the `boolean` type, return the logical AND'd result of both operands.
      auto& reg = rhs.open_boolean();
      reg = do_operator_AND(lhs.as_boolean(), reg);
    }
    else if(lhs.is_integer() && rhs.is_integer()) {
      // For the `integer` and `real` types, return the product of both operands.
      auto& reg = rhs.open_integer();
      reg = do_operator_MUL(lhs.as_integer(), reg);
    }
    else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
      // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
      rhs = do_operator_MUL(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If either operand has type `string` and the other has type `integer`, duplicate the string up to
      // the specified number of times and return the result.
      // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
      rhs = do_operator_MUL(lhs.as_string(), rhs.as_integer());
    }
    else if(lhs.is_integer() && rhs.is_string()) {
      auto& reg = rhs.open_string();
      reg = do_operator_MUL(lhs.as_integer(), reg);
    }
    else {
      ASTERIA_THROW("infix multiplication not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_DIV(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // For the `integer` and `real` types, return the quotient of both operands.
      auto& reg = rhs.open_integer();
      reg = do_operator_DIV(lhs.as_integer(), reg);
    }
    else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
      // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
      rhs = do_operator_DIV(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else {
      ASTERIA_THROW("infix division not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_MOD(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // For the `integer` and `real` types, return the remainder of both operands.
      auto& reg = rhs.open_integer();
      reg = do_operator_MOD(lhs.as_integer(), reg);
    }
    else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
      // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
      rhs = do_operator_MOD(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else {
      ASTERIA_THROW("infix modulo not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_SLL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // If the LHS operand has type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
      // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
      auto& reg = rhs.open_integer();
      reg = do_operator_SLL(lhs.as_integer(), reg);
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If the LHS operand has type `string`, fill space characters in the right and discard characters from the left.
      // The number of bytes in the LHS operand will be preserved.
      // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
      rhs = do_operator_SLL(lhs.as_string(), rhs.as_integer());
    }
    else {
      ASTERIA_THROW("infix logical left shift not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_SRL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // If the LHS operand has type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
      // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
      auto& reg = rhs.open_integer();
      reg = do_operator_SRL(lhs.as_integer(), reg);
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If the LHS operand has type `string`, fill space characters in the left and discard characters from the right.
      // The number of bytes in the LHS operand will be preserved.
      // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
      rhs = do_operator_SRL(lhs.as_string(), rhs.as_integer());
    }
    else {
      ASTERIA_THROW("infix logical right shift not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_SLA(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // If the LHS operand is of type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
      // Bits shifted out that are equal to the sign bit are discarded. Bits shifted in are filled with zeroes.
      // If any bits that are different from the sign bit would be shifted out, an exception is thrown.
      auto& reg = rhs.open_integer();
      reg = do_operator_SLA(lhs.as_integer(), reg);
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If the LHS operand has type `string`, fill space characters in the right.
      // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
      rhs = do_operator_SLA(lhs.as_string(), rhs.as_integer());
    }
    else {
      ASTERIA_THROW("infix arithmetic left shift not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_SRA(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // If the LHS operand is of type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
      // Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
      auto& reg = rhs.open_integer();
      reg = do_operator_SRA(lhs.as_integer(), reg);
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If the LHS operand has type `string`, discard characters from the right.
      // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
      rhs = do_operator_SRA(lhs.as_string(), rhs.as_integer());
    }
    else {
      ASTERIA_THROW("infix arithmetic right shift not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_ANDB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_boolean() && rhs.is_boolean()) {
      // For the `boolean` type, return the logical AND'd result of both operands.
      auto& reg = rhs.open_boolean();
      reg = do_operator_AND(lhs.as_boolean(), reg);
    }
    else if(lhs.is_integer() && rhs.is_integer()) {
      // For the `integer` type, return bitwise AND'd result of both operands.
      auto& reg = rhs.open_integer();
      reg = do_operator_AND(lhs.as_integer(), reg);
    }
    else {
      ASTERIA_THROW("infix bitwise AND not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_ORB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_boolean() && rhs.is_boolean()) {
      // For the `boolean` type, return the logical OR'd result of both operands.
      auto& reg = rhs.open_boolean();
      reg = do_operator_OR(lhs.as_boolean(), reg);
    }
    else if(lhs.is_integer() && rhs.is_integer()) {
      // For the `integer` type, return bitwise OR'd result of both operands.
      auto& reg = rhs.open_integer();
      reg = do_operator_OR(lhs.as_integer(), reg);
    }
    else {
      ASTERIA_THROW("infix bitwise OR not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_XORB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_boolean() && rhs.is_boolean()) {
      // For the `boolean` type, return the logical XOR'd result of both operands.
      auto& reg = rhs.open_boolean();
      reg = do_operator_XOR(lhs.as_boolean(), reg);
    }
    else if(lhs.is_integer() && rhs.is_integer()) {
      // For the `integer` type, return bitwise XOR'd result of both operands.
      auto& reg = rhs.open_integer();
      reg = do_operator_XOR(lhs.as_integer(), reg);
    }
    else {
      ASTERIA_THROW("infix bitwise XOR not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_ASSIGN(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // Pop the RHS operand.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    // Copy the value to the LHS operand which is write-only. `assign` is ignored.
    do_set_temporary(ctx.stack(), true, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_FMA(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is ternary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    auto mid = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_convertible_to_real() && mid.is_convertible_to_real() && rhs.is_convertible_to_real()) {
      // Calculate the fused multiply-add result of the operands.
      // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
      rhs = ::std::fma(lhs.convert_to_real(), mid.convert_to_real(), rhs.convert_to_real());
    }
    else {
      ASTERIA_THROW("fused multiply-add not defined (operands were `$1`, `$2` and `$3`)", lhs, mid, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
    return air_status_next;
  }

AIR_Status do_apply_xop_TAIL(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& lref = ctx.stack().open_top();
    Reference_Modifier::S_array_tail xmod = { };
    lref.zoom_in(::rocket::move(xmod));
    return air_status_next;
  }

AIR_Status do_unpack_struct_array(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& immutable = static_cast<bool>(pu.y8s[0]);
    const auto& nelems = pu.y32;

    // Read the value of the initializer.
    // Note that the initializer must not have been empty for this function.
    auto val = ctx.stack().get_top().read();
    ctx.stack().pop();
    // Make sure it is really an `array`.
    if(val.is_null()) {
      val = G_array();
    }
    if(!val.is_array()) {
      ASTERIA_THROW("attempt to unpack a non-`array` as an `array` (initializer was `$1`)", val);
    }
    auto& arr = val.open_array();
    // Pop and initialize variables from right to left.
    for(size_t i = 0; i != nelems; ++i) {
      // Get the variable back.
      auto var = ctx.stack().get_top().get_variable_opt();
      ROCKET_ASSERT(var);
      ROCKET_ASSERT(var->get_value().is_null());
      ctx.stack().pop();
      // Initialize it.
      auto qelem = arr.mut_ptr(nelems - i - 1);
      if(!qelem) {
        continue;
      }
      var->reset(::rocket::move(*qelem), immutable);
    }
    return air_status_next;
  }

AIR_Status do_unpack_struct_object(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& immutable = static_cast<bool>(pu.u8s[0]);
    const auto& keys = do_pcast<Pv_names>(pv)->names;

    // Read the value of the initializer.
    // Note that the initializer must not have been empty for this function.
    auto val = ctx.stack().get_top().read();
    ctx.stack().pop();
    // Make sure it is really an `object`.
    if(val.is_null()) {
      val = G_object();
    }
    if(!val.is_object()) {
      ASTERIA_THROW("attempt to unpack a non-`object` as an `object` (initializer was `$1`)", val);
    }
    auto& obj = val.open_object();
    // Pop and initialize variables from right to left.
    for(auto it = keys.rbegin(); it != keys.rend(); ++it) {
      // Get the variable back.
      auto var = ctx.stack().get_top().get_variable_opt();
      ROCKET_ASSERT(var);
      ROCKET_ASSERT(var->get_value().is_null());
      ctx.stack().pop();
      // Initialize it.
      auto qelem = obj.mut_ptr(*it);
      if(!qelem) {
        var->set_immutable(immutable);
        continue;
      }
      var->reset(::rocket::move(*qelem), immutable);
    }
    return air_status_next;
  }

AIR_Status do_define_null_variable(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& immutable = static_cast<bool>(pu.u8s[0]);
    const auto& sloc = do_pcast<Pv_sloc_name>(pv)->sloc;
    const auto& name = do_pcast<Pv_sloc_name>(pv)->name;
    const auto& inside = ctx.zvarg().func();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Allocate a variable and initialize it to `null`.
    auto var = ctx.global().create_variable();
    var->reset(nullptr, immutable);
    // Call the hook function if any.
    if(qhooks) {
      qhooks->on_variable_declare(sloc, inside, name);
    }
    // Inject the variable into the current context.
    Reference_Root::S_variable xref = { ::rocket::move(var) };
    ctx.open_named_reference(name) = ::rocket::move(xref);
    return air_status_next;
  }

AIR_Status do_single_step_trap(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc>(pv)->sloc;
    const auto& inside = ctx.zvarg().func();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Call the hook function if any.
    if(qhooks) {
      qhooks->on_single_step_trap(sloc, inside, ::std::addressof(ctx));
    }
    return air_status_next;
  }

}  // namespace

opt<AIR_Node> AIR_Node::rebind_opt(const Abstract_Context& ctx) const
  {
    switch(this->index()) {
    case index_clear_stack: {
        // There is nothing to bind.
        return ::rocket::clear;
      }

    case index_execute_block: {
        const auto& altr = this->m_stor.as<index_execute_block>();
        // Check for rebinds recursively.
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_body, ctx_body);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_declare_variable: {
    case index_initialize_variable:
        // There is nothing to bind.
        return ::rocket::clear;
      }

    case index_if_statement: {
        const auto& altr = this->m_stor.as<index_if_statement>();
        // Check for rebinds recursively.
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_true, ctx_body);
        do_rebind_nodes(dirty, xaltr.code_false, ctx_body);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_switch_statement: {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        // Check for rebinds recursively.
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_labels, ctx_body);
        do_rebind_nodes(dirty, xaltr.code_bodies, ctx_body);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_do_while_statement: {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        // Check for rebinds recursively.
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_body, ctx_body);
        do_rebind_nodes(dirty, xaltr.code_cond, ctx_body);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_while_statement: {
        const auto& altr = this->m_stor.as<index_while_statement>();
        // Check for rebinds recursively.
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_cond, ctx_body);
        do_rebind_nodes(dirty, xaltr.code_body, ctx_body);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_for_each_statement: {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        // Check for rebinds recursively.
        Analytic_Context ctx_for(::rocket::ref(ctx));
        Analytic_Context ctx_body(::rocket::ref(ctx_for));
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_init, ctx_for);
        do_rebind_nodes(dirty, xaltr.code_body, ctx_body);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_for_statement: {
        const auto& altr = this->m_stor.as<index_for_statement>();
        // Check for rebinds recursively.
        Analytic_Context ctx_for(::rocket::ref(ctx));
        Analytic_Context ctx_body(::rocket::ref(ctx_for));
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_init, ctx_for);
        do_rebind_nodes(dirty, xaltr.code_cond, ctx_for);
        do_rebind_nodes(dirty, xaltr.code_step, ctx_for);
        do_rebind_nodes(dirty, xaltr.code_body, ctx_body);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_try_statement: {
        const auto& altr = this->m_stor.as<index_try_statement>();
        // Check for rebinds recursively.
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_try, ctx_body);
        do_rebind_nodes(dirty, xaltr.code_catch, ctx_body);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_throw_statement: {
    case index_assert_statement:
    case index_simple_status:
    case index_return_by_value:
    case index_push_immediate:
    case index_push_global_reference:
        // There is nothing to bind.
        return ::rocket::clear;
      }

    case index_push_local_reference: {
        const auto& altr = this->m_stor.as<index_push_local_reference>();
        // Get the context.
        const Abstract_Context* qctx = ::std::addressof(ctx);
        ::rocket::ranged_for(uint32_t(0), altr.depth, [&](uint32_t) { qctx = qctx->get_parent_opt();  });
        ROCKET_ASSERT(qctx);
        // Don't bind references in analytic contexts.
        if(qctx->is_analytic()) {
          return ::rocket::clear;
        }
        // Look for the name in the context.
        auto qref = qctx->get_named_reference_opt(altr.name);
        if(!qref) {
          return ::rocket::clear;
        }
        // Bind it now.
        S_push_bound_reference xaltr = { *qref };
        return ::rocket::move(xaltr);
      }

    case index_push_bound_reference: {
        // There is nothing to bind.
        return ::rocket::clear;
      }

    case index_define_function: {
        const auto& altr = this->m_stor.as<index_define_function>();
        // Check for rebinds recursively.
        Analytic_Context ctx_func(::std::addressof(ctx), altr.params);
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_body, ctx_func);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_branch_expression: {
        const auto& altr = this->m_stor.as<index_branch_expression>();
        // Check for rebinds recursively.
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_true, ctx);
        do_rebind_nodes(dirty, xaltr.code_false, ctx);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // Check for rebinds recursively.
        bool dirty = false;
        auto xaltr = altr;
        do_rebind_nodes(dirty, xaltr.code_null, ctx);
        if(!dirty) {
          return ::rocket::clear;
        }
        // Return the bound node.
        return ::rocket::move(xaltr);
      }

    case index_function_call: {
    case index_member_access:
    case index_push_unnamed_array:
    case index_push_unnamed_object:
    case index_apply_operator:
    case index_unpack_struct_array:
    case index_unpack_struct_object:
    case index_define_null_variable:
    case index_single_step_trap:
        // There is nothing to bind.
        return ::rocket::clear;
      }

    default:
      ASTERIA_TERMINATE("invalid AIR node type (index `$1`)", this->index());
    }
  }

AVMC_Queue& AIR_Node::solidify(AVMC_Queue& queue, uint8_t ipass) const
  {
    switch(this->index()) {
    case index_clear_stack: {
        const auto& altr = this->m_stor.as<index_clear_stack>();
        // There is no parameter.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        (void)altr;
        // Push a new node.
        return avmcp.output<do_clear_stack>(queue);
      }

    case index_execute_block: {
        const auto& altr = this->m_stor.as<index_execute_block>();
        // `pu` is unused.
        // `pv` points to the body.
        AVMC_Appender<Pv_queues_fixed<1>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queues[0], altr.code_body);
        // Push a new node.
        return avmcp.output<do_execute_block>(queue);
      }

    case index_declare_variable: {
        const auto& altr = this->m_stor.as<index_declare_variable>();
        // `pu` is unused.
        // `pv` points to the source location and name.
        AVMC_Appender<Pv_sloc_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_declare_variable>(queue);
      }

    case index_initialize_variable: {
        const auto& altr = this->m_stor.as<index_initialize_variable>();
        // `pu.u8s[0]` is `immutable`.
        // `pv` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.pu.u8s[0] = altr.immutable;
        // Push a new node.
        return avmcp.output<do_initialize_variable>(queue);
      }

    case index_if_statement: {
        const auto& altr = this->m_stor.as<index_if_statement>();
        // `pu.u8s[0]` is `negative`.
        // `pv` points to the two branches.
        AVMC_Appender<Pv_queues_fixed<2>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.pu.u8s[0] = altr.negative;
        do_solidify_queue(avmcp.queues[0], altr.code_true);
        do_solidify_queue(avmcp.queues[1], altr.code_false);
        // Push a new node.
        return avmcp.output<do_if_statement>(queue);
      }

    case index_switch_statement: {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        // `pu` is unused.
        // `pv` points to all clauses.
        AVMC_Appender<Pv_switch> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        for(size_t i = 0; i != altr.code_bodies.size(); ++i) {
          do_solidify_queue(avmcp.queues_labels.emplace_back(), altr.code_labels.at(i));
          do_solidify_queue(avmcp.queues_bodies.emplace_back(), altr.code_bodies.at(i));
        }
        avmcp.names_added = altr.names_added;
        // Push a new node.
        return avmcp.output<do_switch_statement>(queue);
      }

    case index_do_while_statement: {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        // `pu.u8s[0]` is `negative`.
        // `pv` points to the body and the condition.
        AVMC_Appender<Pv_queues_fixed<2>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queues[0], altr.code_body);
        avmcp.pu.u8s[0] = altr.negative;
        do_solidify_queue(avmcp.queues[1], altr.code_cond);
        // Push a new node.
        return avmcp.output<do_do_while_statement>(queue);
      }

    case index_while_statement: {
        const auto& altr = this->m_stor.as<index_while_statement>();
        // `pu.u8s[0]` is `negative`.
        // `pv` points to the condition and the body.
        AVMC_Appender<Pv_queues_fixed<2>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.pu.u8s[0] = altr.negative;
        do_solidify_queue(avmcp.queues[0], altr.code_cond);
        do_solidify_queue(avmcp.queues[1], altr.code_body);
        // Push a new node.
        return avmcp.output<do_while_statement>(queue);
      }

    case index_for_each_statement: {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        // `pu` is unused.
        // `pv` points to the range initializer and the body.
        AVMC_Appender<Pv_for_each> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.name_key = altr.name_key;
        avmcp.name_mapped = altr.name_mapped;
        do_solidify_queue(avmcp.queue_init, altr.code_init);
        do_solidify_queue(avmcp.queue_body, altr.code_body);
        // Push a new node.
        return avmcp.output<do_for_each_statement>(queue);
      }

    case index_for_statement: {
        const auto& altr = this->m_stor.as<index_for_statement>();
        // `pu` is unused.
        // `pv` points to the triplet and the body.
        AVMC_Appender<Pv_queues_fixed<4>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queues[0], altr.code_init);
        do_solidify_queue(avmcp.queues[1], altr.code_cond);
        do_solidify_queue(avmcp.queues[2], altr.code_step);
        do_solidify_queue(avmcp.queues[3], altr.code_body);
        // Push a new node.
        return avmcp.output<do_for_statement>(queue);
      }

    case index_try_statement: {
        const auto& altr = this->m_stor.as<index_try_statement>();
        // `pu` is unused.
        // `pv` points to the clauses.
        AVMC_Appender<Pv_try> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queue_try, altr.code_try);
        avmcp.sloc = altr.sloc;
        avmcp.name_except = altr.name_except;
        do_solidify_queue(avmcp.queue_catch, altr.code_catch);
        // Push a new node.
        return avmcp.output<do_try_statement>(queue);
      }

    case index_throw_statement: {
        const auto& altr = this->m_stor.as<index_throw_statement>();
        // `pu` is unused.
        // `pv` points to the source location.
        AVMC_Appender<Pv_sloc> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        // Push a new node.
        return avmcp.output<do_throw_statement>(queue);
      }

    case index_assert_statement: {
        const auto& altr = this->m_stor.as<index_assert_statement>();
        // `pu.u8s[0]` is `negative`.
        // `pv` points to the source location and the message.
        AVMC_Appender<Pv_sloc_msg> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.pu.u8s[0] = altr.negative;
        avmcp.msg = altr.msg;
        // Push a new node.
        return avmcp.output<do_assert_statement>(queue);
      }

    case index_simple_status: {
        const auto& altr = this->m_stor.as<index_simple_status>();
        // `pu.u8s[0] ` is `status`.
        // `pv` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.pu.u8s[0] = altr.status;
        // Push a new node.
        return avmcp.output<do_simple_status>(queue);
      }

    case index_return_by_value: {
        const auto& altr = this->m_stor.as<index_return_by_value>();
        // There is no parameter.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        (void)altr;
        // Push a new node.
        return avmcp.output<do_return_by_value>(queue);
      }

    case index_push_immediate: {
        const auto& altr = this->m_stor.as<index_push_immediate>();
        // `pu` is unused.
        // `pv` points to a copy of `val`.
        AVMC_Appender<Value> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        static_cast<Value&>(avmcp) = altr.val;
        // Push a new node.
        return avmcp.output<do_push_immediate>(queue);
      }

    case index_push_global_reference: {
        const auto& altr = this->m_stor.as<index_push_global_reference>();
        // `pu` is unused.
        // `pv` points to the name.
        AVMC_Appender<Pv_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_push_global_reference>(queue);
      }

    case index_push_local_reference: {
        const auto& altr = this->m_stor.as<index_push_local_reference>();
        // `pu.x32` is `depth`.
        // `pv` points to the name.
        AVMC_Appender<Pv_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.pu.x32 = altr.depth;
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_push_local_reference>(queue);
      }

    case index_push_bound_reference: {
        const auto& altr = this->m_stor.as<index_push_bound_reference>();
        // `pu` is unused.
        // `pv` points to a copy of `ref`.
        AVMC_Appender<Reference> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        static_cast<Reference&>(avmcp) = altr.ref;
        // Push a new node.
        return avmcp.output<do_push_bound_reference>(queue);
      }

    case index_define_function: {
        const auto& altr = this->m_stor.as<index_define_function>();
        // `pu` is unused.
        // `pv` points to the name, the parameter list, and the body of the function.
        AVMC_Appender<Pv_func> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.func = altr.func;
        avmcp.params = altr.params;
        avmcp.code_body = altr.code_body;
        // Push a new node.
        return avmcp.output<do_define_function>(queue);
      }

    case index_branch_expression: {
        const auto& altr = this->m_stor.as<index_branch_expression>();
        // `pu.u8s[0]` is `assign`.
        // `pv` points to the two branches.
        AVMC_Appender<Pv_queues_fixed<2>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queues[0], altr.code_true);
        do_solidify_queue(avmcp.queues[1], altr.code_false);
        avmcp.pu.u8s[0] = altr.assign;
        // Push a new node.
        return avmcp.output<do_branch_expression>(queue);
      }

    case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // `pu.u8s[0]` is `assign`.
        // `pv` points to the alternative.
        AVMC_Appender<Pv_queues_fixed<1>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queues[0], altr.code_null);
        avmcp.pu.u8s[0] = altr.assign;
        // Push a new node.
        return avmcp.output<do_coalescence>(queue);
      }

    case index_function_call: {
        const auto& altr = this->m_stor.as<index_function_call>();
        // `pu.u8s[0]` is `ptc_aware`.
        // `pv` points to the source location and the argument avmcp.cifier vector.
        AVMC_Appender<Pv_call> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.args_by_refs = altr.args_by_refs;
        avmcp.pu.u8s[0] = altr.ptc_aware;
        // Push a new node.
        return avmcp.output<do_function_call>(queue);
      }

    case index_member_access: {
        const auto& altr = this->m_stor.as<index_member_access>();
        // `pu` is unused.
        // `pv` points to the name.
        AVMC_Appender<Pv_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_member_access>(queue);
      }

    case index_push_unnamed_array: {
        const auto& altr = this->m_stor.as<index_push_unnamed_array>();
        // `pu.x32` is `nelems`.
        // `pv` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.pu.x32 = altr.nelems;
        // Push a new node.
        return avmcp.output<do_push_unnamed_array>(queue);
      }

    case index_push_unnamed_object: {
        const auto& altr = this->m_stor.as<index_push_unnamed_object>();
        // `pu` is unused.
        // `pv` points to the keys.
        AVMC_Appender<Pv_names> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.names = altr.keys;
        // Push a new node.
        return avmcp.output<do_push_unnamed_object>(queue);
      }

    case index_apply_operator: {
        const auto& altr = this->m_stor.as<index_apply_operator>();
        // `pu.u8s[0]` is `assign`. Other fields may be used depending on the operator.
        // `pv` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        switch(::rocket::weaken_enum(altr.xop)) {
        case xop_cmp_eq: {
            avmcp.pu.u8s[0] = altr.assign;
            avmcp.pu.u8s[1] = compare_equal;
            avmcp.pu.u8s[2] = false;
            break;
          }
        case xop_cmp_ne: {
            avmcp.pu.u8s[0] = altr.assign;
            avmcp.pu.u8s[1] = compare_equal;
            avmcp.pu.u8s[2] = true;
            break;
          }
        case xop_cmp_lt: {
            avmcp.pu.u8s[0] = altr.assign;
            avmcp.pu.u8s[1] = compare_less;
            avmcp.pu.u8s[2] = false;
            break;
          }
        case xop_cmp_gt: {
            avmcp.pu.u8s[0] = altr.assign;
            avmcp.pu.u8s[1] = compare_greater;
            avmcp.pu.u8s[2] = false;
            break;
          }
        case xop_cmp_lte: {
            avmcp.pu.u8s[0] = altr.assign;
            avmcp.pu.u8s[1] = compare_greater;
            avmcp.pu.u8s[2] = true;
            break;
          }
        case xop_cmp_gte: {
            avmcp.pu.u8s[0] = altr.assign;
            avmcp.pu.u8s[1] = compare_less;
            avmcp.pu.u8s[2] = true;
            break;
          }
        default: {
            avmcp.pu.u8s[0] = altr.assign;
            break;
          }
        }
        // Push a new node.
        switch(altr.xop) {
        case xop_inc_post: {
            return avmcp.output<do_apply_xop_INC_POST>(queue);
          }
        case xop_dec_post: {
            return avmcp.output<do_apply_xop_DEC_POST>(queue);
          }
        case xop_subscr: {
            return avmcp.output<do_apply_xop_SUBSCR>(queue);
          }
        case xop_pos: {
            return avmcp.output<do_apply_xop_POS>(queue);
          }
        case xop_neg: {
            return avmcp.output<do_apply_xop_NEG>(queue);
          }
        case xop_notb: {
            return avmcp.output<do_apply_xop_NOTB>(queue);
          }
        case xop_notl: {
            return avmcp.output<do_apply_xop_NOTL>(queue);
          }
        case xop_inc_pre: {
            return avmcp.output<do_apply_xop_INC_PRE>(queue);
          }
        case xop_dec_pre: {
            return avmcp.output<do_apply_xop_DEC_PRE>(queue);
          }
        case xop_unset: {
            return avmcp.output<do_apply_xop_UNSET>(queue);
          }
        case xop_lengthof: {
            return avmcp.output<do_apply_xop_LENGTHOF>(queue);
          }
        case xop_typeof: {
            return avmcp.output<do_apply_xop_TYPEOF>(queue);
          }
        case xop_sqrt: {
            return avmcp.output<do_apply_xop_SQRT>(queue);
          }
        case xop_isnan: {
            return avmcp.output<do_apply_xop_ISNAN>(queue);
          }
        case xop_isinf: {
            return avmcp.output<do_apply_xop_ISINF>(queue);
          }
        case xop_abs: {
            return avmcp.output<do_apply_xop_ABS>(queue);
          }
        case xop_sign: {
            return avmcp.output<do_apply_xop_SIGN>(queue);
          }
        case xop_round: {
            return avmcp.output<do_apply_xop_ROUND>(queue);
          }
        case xop_floor: {
            return avmcp.output<do_apply_xop_FLOOR>(queue);
          }
        case xop_ceil: {
            return avmcp.output<do_apply_xop_CEIL>(queue);
          }
        case xop_trunc: {
            return avmcp.output<do_apply_xop_TRUNC>(queue);
          }
        case xop_iround: {
            return avmcp.output<do_apply_xop_IROUND>(queue);
          }
        case xop_ifloor: {
            return avmcp.output<do_apply_xop_IFLOOR>(queue);
          }
        case xop_iceil: {
            return avmcp.output<do_apply_xop_ICEIL>(queue);
          }
        case xop_itrunc: {
            return avmcp.output<do_apply_xop_ITRUNC>(queue);
          }
        case xop_cmp_eq: {
            return avmcp.output<do_apply_xop_CMP_XEQ>(queue);
          }
        case xop_cmp_ne: {
            return avmcp.output<do_apply_xop_CMP_XEQ>(queue);
          }
        case xop_cmp_lt: {
            return avmcp.output<do_apply_xop_CMP_XREL>(queue);
          }
        case xop_cmp_gt: {
            return avmcp.output<do_apply_xop_CMP_XREL>(queue);
          }
        case xop_cmp_lte: {
            return avmcp.output<do_apply_xop_CMP_XREL>(queue);
          }
        case xop_cmp_gte: {
            return avmcp.output<do_apply_xop_CMP_XREL>(queue);
          }
        case xop_cmp_3way: {
            return avmcp.output<do_apply_xop_CMP_3WAY>(queue);
          }
        case xop_add: {
            return avmcp.output<do_apply_xop_ADD>(queue);
          }
        case xop_sub: {
            return avmcp.output<do_apply_xop_SUB>(queue);
          }
        case xop_mul: {
            return avmcp.output<do_apply_xop_MUL>(queue);
          }
        case xop_div: {
            return avmcp.output<do_apply_xop_DIV>(queue);
          }
        case xop_mod: {
            return avmcp.output<do_apply_xop_MOD>(queue);
          }
        case xop_sll: {
            return avmcp.output<do_apply_xop_SLL>(queue);
          }
        case xop_srl: {
            return avmcp.output<do_apply_xop_SRL>(queue);
          }
        case xop_sla: {
            return avmcp.output<do_apply_xop_SLA>(queue);
          }
        case xop_sra: {
            return avmcp.output<do_apply_xop_SRA>(queue);
          }
        case xop_andb: {
            return avmcp.output<do_apply_xop_ANDB>(queue);
          }
        case xop_orb: {
            return avmcp.output<do_apply_xop_ORB>(queue);
          }
        case xop_xorb: {
            return avmcp.output<do_apply_xop_XORB>(queue);
          }
        case xop_assign: {
            return avmcp.output<do_apply_xop_ASSIGN>(queue);
          }
        case xop_fma_3: {
            return avmcp.output<do_apply_xop_FMA>(queue);
          }
        case xop_tail: {
            return avmcp.output<do_apply_xop_TAIL>(queue);
          }
        default:
          ASTERIA_TERMINATE("invalid operator type (xop `$1`)", altr.xop);
        }
      }

    case index_unpack_struct_array: {
        const auto& altr = this->m_stor.as<index_unpack_struct_array>();
        // `pu.y8s[0]` is `immutable`.
        // `pu.y32` is `nelems`.
        // `pv` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.pu.y8s[0] = altr.immutable;
        avmcp.pu.y32 = altr.nelems;
        // Push a new node.
        return avmcp.output<do_unpack_struct_array>(queue);
      }

    case index_unpack_struct_object: {
        const auto& altr = this->m_stor.as<index_unpack_struct_object>();
        // `pu.u8s[0]` is `immutable`.
        // `pv` points to the keys.
        AVMC_Appender<Pv_names> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.pu.u8s[0] = altr.immutable;
        avmcp.names = altr.keys;
        // Push a new node.
        return avmcp.output<do_unpack_struct_object>(queue);
      }

    case index_define_null_variable: {
        const auto& altr = this->m_stor.as<index_define_null_variable>();
        // `pu.u8s[0]` is `immutable`.
        // `pv` points to the source location and name.
        AVMC_Appender<Pv_sloc_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.pu.u8s[0] = altr.immutable;
        avmcp.sloc = altr.sloc;
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_define_null_variable>(queue);
      }

    case index_single_step_trap: {
        const auto& altr = this->m_stor.as<index_single_step_trap>();
        // `pu.u8s[0]` is unused.
        // `pv` points to the source location.
        AVMC_Appender<Pv_sloc> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        // Push a new node.
        return avmcp.output<do_single_step_trap>(queue);
      }

    default:
      ASTERIA_TERMINATE("invalid AIR node type (index `$1`)", this->index());
    }
  }

Variable_Callback& AIR_Node::enumerate_variables(Variable_Callback& callback) const
  {
    switch(this->index()) {
    case index_clear_stack: {
        return callback;
      }

    case index_execute_block: {
        const auto& altr = this->m_stor.as<index_execute_block>();
        ::rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_declare_variable: {
    case index_initialize_variable:
        return callback;
      }

    case index_if_statement: {
        const auto& altr = this->m_stor.as<index_if_statement>();
        ::rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        ::rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_switch_statement: {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        for(size_t i = 0; i != altr.code_labels.size(); ++i) {
          ::rocket::for_each(altr.code_labels.at(i), [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
          ::rocket::for_each(altr.code_bodies.at(i), [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        }
        return callback;
      }

    case index_do_while_statement: {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        ::rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        ::rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_while_statement: {
        const auto& altr = this->m_stor.as<index_while_statement>();
        ::rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        ::rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_for_each_statement: {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        ::rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        ::rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_for_statement: {
        const auto& altr = this->m_stor.as<index_for_statement>();
        ::rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        ::rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        ::rocket::for_each(altr.code_step, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        ::rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_try_statement: {
        const auto& altr = this->m_stor.as<index_try_statement>();
        ::rocket::for_each(altr.code_try, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        ::rocket::for_each(altr.code_catch, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_throw_statement: {
    case index_assert_statement:
    case index_simple_status:
    case index_return_by_value:
        return callback;
      }

    case index_push_immediate: {
        const auto& altr = this->m_stor.as<index_push_immediate>();
        altr.val.enumerate_variables(callback);
        return callback;
      }

    case index_push_global_reference: {
    case index_push_local_reference:
        return callback;
      }

    case index_push_bound_reference: {
        const auto& altr = this->m_stor.as<index_push_bound_reference>();
        altr.ref.enumerate_variables(callback);
        return callback;
      }

    case index_define_function: {
        const auto& altr = this->m_stor.as<index_define_function>();
        ::rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_branch_expression: {
        const auto& altr = this->m_stor.as<index_branch_expression>();
        ::rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        ::rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();
        ::rocket::for_each(altr.code_null, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }

    case index_function_call: {
    case index_member_access:
    case index_push_unnamed_array:
    case index_push_unnamed_object:
    case index_apply_operator:
    case index_unpack_struct_array:
    case index_unpack_struct_object:
    case index_define_null_variable:
    case index_single_step_trap:
        return callback;
      }

    default:
      ASTERIA_TERMINATE("invalid AIR node type (index `$1`)", this->index());
    }
  }

}  // namespace Asteria
