// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "enums.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "analytic_context.hpp"
#include "genius_collector.hpp"
#include "runtime_error.hpp"
#include "variable_callback.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "loader_lock.hpp"
#include "air_optimizer.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../llds/avmc_queue.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

using ParamU      = AVMC_Queue::ParamU;
using Symbols     = AVMC_Queue::Symbols;
using Executor    = AVMC_Queue::Executor;
using Enumerator  = AVMC_Queue::Enumerator;

///////////////////////////////////////////////////////////////////////////
// Auxiliary functions
///////////////////////////////////////////////////////////////////////////

bool&
do_rebind_nodes(bool& dirty, cow_vector<AIR_Node>& code, const Abstract_Context& ctx)
  {
    for(size_t i = 0;  i < code.size();  ++i) {
      auto qnode = code[i].rebind_opt(ctx);
      if(!qnode)
        continue;
      dirty |= true;
      code.mut(i) = ::std::move(*qnode);
    }
    return dirty;
  }

bool&
do_rebind_nodes(bool& dirty, cow_vector<cow_vector<AIR_Node>>& code, const Abstract_Context& ctx)
  {
    for(size_t k = 0;  k < code.size();  ++k) {
      for(size_t i = 0;  i < code[k].size();  ++i) {
        auto qnode = code[k][i].rebind_opt(ctx);
        if(!qnode)
          continue;
        dirty |= true;
        code.mut(k).mut(i) = ::std::move(*qnode);
      }
    }
    return dirty;
  }

template<typename XNodeT>
opt<AIR_Node>
do_forward_if_opt(bool dirty, XNodeT&& xnode)
  {
    if(dirty)
      return ::std::forward<XNodeT>(xnode);
    else
      return nullopt;
  }

template<typename XValT>
Reference&
do_set_temporary(Evaluation_Stack& stack, bool assign, XValT&& xval)
  {
    auto& ref = stack.open_top();
    if(assign) {
      // Write the value to the top refernce.
      ref.open() = ::std::forward<XValT>(xval);
      return ref;
    }
    // Replace the top reference with a temporary reference to the value.
    Reference_root::S_temporary xref = { ::std::forward<XValT>(xval) };
    return ref = ::std::move(xref);
  }

Reference&
do_declare(Executive_Context& ctx, const phsh_string& name)
  {
    return ctx.open_named_reference(name) = Reference_root::S_void();
  }

AIR_Status
do_execute_block(const AVMC_Queue& queue, Executive_Context& ctx)
  {
    // Execute the body on a new context.
    Executive_Context ctx_next(::rocket::ref(ctx), nullptr);
    AIR_Status status;
    ASTERIA_RUNTIME_TRY {
      status = queue.execute(ctx_next);
    }
    ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
      ctx_next.on_scope_exit(except);
      throw;
    }
    ctx_next.on_scope_exit(status);
    return status;
  }

AIR_Status
do_evaluate_branch(const AVMC_Queue& queue, bool assign, Executive_Context& ctx)
  {
    if(ROCKET_EXPECT(queue.empty()))
      // Leave the condition on the top of the stack.
      return air_status_next;

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
    // You must forward the status code as is, because PTCs may return `air_status_return_ref`.
    return queue.execute(ctx);
  }

template<typename ParamV>
inline
const ParamV*
do_pcast(const void* pv)
noexcept
  {
    return static_cast<const ParamV*>(pv);
  }

template<typename ParamV>
Variable_Callback&
do_penum(Variable_Callback& callback, ParamU /*pu*/, const void* pv)
  {
    return static_cast<const ParamV*>(pv)->enumerate_variables(callback);
  }

// This defines common members of AVMC nodes.
struct AVMC_Appender_common
  {
    ParamU pu = { };
    opt<Symbols> syms;

    void
    set_symbols(const Source_Location& sloc)
      { this->syms = { sloc };  }
  };

// This is the trait struct for parameter types that implement `enumerate_variables()`.
template<typename ParamV, typename = void>
struct AVMC_Appender
  : AVMC_Appender_common, ParamV
  {
    constexpr
    AVMC_Appender()
      : ParamV()
      { }

    AVMC_Queue&
    request(AVMC_Queue& queue)
    const
      { return queue.request(sizeof(ParamV), this->syms.value_ptr());  }

    template<Executor executorT>
    AVMC_Queue&
    output(AVMC_Queue& queue)
      { return queue.append<executorT, do_penum<ParamV>>(this->pu, this->syms.value_ptr(),
                                                         static_cast<ParamV&&>(*this));  }
  };

// This is the trait struct for parameter types that do not implement `enumerate_variables()`.
template<typename ParamV>
struct AVMC_Appender<ParamV, ROCKET_VOID_T(typename ParamV::nonenumerable)>
  : AVMC_Appender_common, ParamV
  {
    constexpr
    AVMC_Appender()
      : ParamV()
      { }

    AVMC_Queue&
    request(AVMC_Queue& queue)
    const
      { return queue.request(sizeof(ParamV), this->syms.value_ptr());  }

    template<Executor executorT>
    AVMC_Queue&
    output(AVMC_Queue& queue)
      { return queue.append<executorT>(this->pu, this->syms.value_ptr(), static_cast<ParamV&&>(*this));  }
  };

// This is the trait struct when there is no parameter.
template<>
struct AVMC_Appender<void, void>
  : AVMC_Appender_common
  {
    constexpr
    AVMC_Appender()
      { }

    AVMC_Queue&
    request(AVMC_Queue& queue)
    const
      { return queue.request(0, this->syms.value_ptr());  }

    template<Executor executorT>
    AVMC_Queue&
    output(AVMC_Queue& queue)
      { return queue.append<executorT>(this->pu, this->syms.value_ptr());  }
  };

///////////////////////////////////////////////////////////////////////////
// Parameter structs and variable enumeration callbacks
///////////////////////////////////////////////////////////////////////////

template<size_t nqsT>
struct Pv_queues_fixed
  {
    AVMC_Queue queues[nqsT];

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const
      {
        ::rocket::for_each(this->queues, callback);
        return callback;
      }
  };

struct Pv_switch
  {
    cow_vector<AVMC_Queue> queues_labels;
    cow_vector<AVMC_Queue> queues_bodies;
    cow_vector<cow_vector<phsh_string>> names_added;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const
      {
        ::rocket::for_each(this->queues_labels, callback);
        ::rocket::for_each(this->queues_bodies, callback);
        return callback;
      }
  };

struct Pv_for_each
  {
    phsh_string name_key;
    phsh_string name_mapped;
    AVMC_Queue queue_init;
    AVMC_Queue queue_body;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const
      {
        this->queue_init.enumerate_variables(callback);
        this->queue_body.enumerate_variables(callback);
        return callback;
      }
  };

struct Pv_try
  {
    AVMC_Queue queue_try;
    Source_Location sloc_catch;
    phsh_string name_except;
    AVMC_Queue queue_catch;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const
      {
        this->queue_try.enumerate_variables(callback);
        this->queue_catch.enumerate_variables(callback);
        return callback;
      }
  };

struct Pv_func
  {
    Compiler_Options opts;
    Source_Location sloc;
    cow_string func;
    cow_vector<phsh_string> params;
    cow_vector<AIR_Node> code_body;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const
      {
        ::rocket::for_each(this->code_body, callback);
        return callback;
      }
  };

struct Pv_defer
  {
    Source_Location sloc;
    cow_vector<AIR_Node> code_body;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const
      {
        ::rocket::for_each(this->code_body, callback);
        return callback;
      }
  };

struct Pv_ref
  {
    Reference ref = Reference_root::S_void();

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const
      {
        this->ref.enumerate_variables(callback);
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

struct Pv_opts_sloc
  {
    Compiler_Options opts;
    Source_Location sloc;

    using nonenumerable = ::std::true_type;
  };

///////////////////////////////////////////////////////////////////////////
// Executor functions
///////////////////////////////////////////////////////////////////////////

AIR_Status
do_clear_stack(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // Clear the stack.
    ctx.stack().clear();
    return air_status_next;
  }

AIR_Status
do_execute_block(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& queue_body = do_pcast<Pv_queues_fixed<1>>(pv)->queues[0];

    // Execute the body on a new context.
    return do_execute_block(queue_body, ctx);
  }

AIR_Status
do_declare_variable(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc_name>(pv)->sloc;
    const auto& name = do_pcast<Pv_sloc_name>(pv)->name;
    const auto& inside = ctx.zvarg()->func();
    const auto& gcoll = ctx.global().genius_collector();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Allocate an uninitialized variable.
    auto var = gcoll->create_variable();

    // Inject the variable into the current context.
    Reference_root::S_variable xref = { ::std::move(var) };
    ctx.open_named_reference(name) = xref;

    // Call the hook function if any.
    if(qhooks)
      qhooks->on_variable_declare(sloc, inside, name);

    // Push a copy of the reference onto the stack.
    ctx.stack().push(::std::move(xref));
    return air_status_next;
  }

AIR_Status
do_initialize_variable(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& immutable = static_cast<bool>(pu.u8s[0]);

    // Read the value of the initializer.
    // Note that the initializer must not have been empty for this function.
    auto val = ctx.stack().get_top().read();
    ctx.stack().pop();

    // Get the variable back.
    auto var = ctx.stack().get_top().get_variable_opt();
    ctx.stack().pop();

    // Initialize it.
    ROCKET_ASSERT(var && !var->is_initialized());
    var->initialize(::std::move(val), immutable);
    return air_status_next;
  }

AIR_Status
do_if_statement(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& negative = static_cast<bool>(pu.u8s[0]);
    const auto& queue_true = do_pcast<Pv_queues_fixed<2>>(pv)->queues[0];
    const auto& queue_false = do_pcast<Pv_queues_fixed<2>>(pv)->queues[1];

    // Check the value of the condition.
    if(ctx.stack().get_top().read().test() != negative)
      return do_execute_block(queue_true, ctx);
    else
      return do_execute_block(queue_false, ctx);
  }

AIR_Status
do_switch_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& queues_labels = do_pcast<Pv_switch>(pv)->queues_labels;
    const auto& queues_bodies = do_pcast<Pv_switch>(pv)->queues_bodies;
    const auto& names_added = do_pcast<Pv_switch>(pv)->names_added;

    // Get the number of clauses.
    auto nclauses = queues_labels.size();
    ROCKET_ASSERT(nclauses == queues_bodies.size());
    ROCKET_ASSERT(nclauses == names_added.size());

    // Read the value of the condition.
    auto cond = ctx.stack().get_top().read();

    // Find a target clause.
    // This is different from the `switch` statement in C, where `case` labels must have constant operands.
    opt<size_t> qtarget;
    for(size_t i = 0;  i < nclauses;  ++i) {
      // This is a `default` clause if the condition is empty, and a `case` clause otherwise.
      if(queues_labels[i].empty()) {
        if(qtarget)
          ASTERIA_THROW("multiple `default` clauses");
        qtarget = i;
        continue;
      }
      // Evaluate the operand and check whether it equals `cond`.
      auto status = queues_labels[i].execute(ctx);
      ROCKET_ASSERT(status == air_status_next);
      if(ctx.stack().get_top().read().compare(cond) == compare_equal) {
        qtarget = i;
        break;
      }
    }
    // Skip this statement if no matching clause has been found.
    if(!qtarget)
      return air_status_next;

    // Jump to the clause denoted by `qtarget`.
    // Note that all clauses share the same context.
    Executive_Context ctx_body(::rocket::ref(ctx), nullptr);
    AIR_Status status;
    ASTERIA_RUNTIME_TRY {
      // Fly over all clauses that precede `qtarget`.
      size_t k = SIZE_MAX;
      while(++k < *qtarget)
        ::rocket::for_each(names_added[k], [&](const phsh_string& name) { do_declare(ctx_body, name);  });

      // Execute all clauses from `qtarget`.
      do {
        status = queues_bodies[k].execute(ctx_body);
        if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_switch })) {
          status = air_status_next;
          break;
        }
        if(status != air_status_next)
          break;
      } while(++k < nclauses);
    }
    ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
      ctx_body.on_scope_exit(except);
      throw;
    }
    ctx_body.on_scope_exit(status);
    return status;
  }

AIR_Status
do_do_while_statement(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& queue_body = do_pcast<Pv_queues_fixed<2>>(pv)->queues[0];
    const auto& negative = static_cast<bool>(pu.u8s[0]);
    const auto& queue_cond = do_pcast<Pv_queues_fixed<2>>(pv)->queues[1];

    // This is the same as the `do...while` statement in C.
    for(;;) {
      // Execute the body.
      auto status = do_execute_block(queue_body, ctx);
      if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while }))
        break;
      if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_while }))
        return status;

      // Check the condition.
      status = queue_cond.execute(ctx);
      ROCKET_ASSERT(status == air_status_next);
      if(ctx.stack().get_top().read().test() == negative)
        break;
    }
    return air_status_next;
  }

AIR_Status
do_while_statement(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& negative = static_cast<bool>(pu.u8s[0]);
    const auto& queue_cond = do_pcast<Pv_queues_fixed<2>>(pv)->queues[0];
    const auto& queue_body = do_pcast<Pv_queues_fixed<2>>(pv)->queues[1];

    // This is the same as the `while` statement in C.
    for(;;) {
      // Check the condition.
      auto status = queue_cond.execute(ctx);
      ROCKET_ASSERT(status == air_status_next);
      if(ctx.stack().get_top().read().test() == negative)
        break;

      // Execute the body.
      status = do_execute_block(queue_body, ctx);
      if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while }))
        break;
      if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_while }))
        return status;
    }
    return air_status_next;
  }

AIR_Status
do_for_each_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& name_key = do_pcast<Pv_for_each>(pv)->name_key;
    const auto& name_mapped = do_pcast<Pv_for_each>(pv)->name_mapped;
    const auto& queue_init = do_pcast<Pv_for_each>(pv)->queue_init;
    const auto& queue_body = do_pcast<Pv_for_each>(pv)->queue_body;
    const auto& gcoll = ctx.global().genius_collector();

    // We have to create an outer context due to the fact that the key and mapped
    // references outlast every iteration.
    Executive_Context ctx_for(::rocket::ref(ctx), nullptr);

    // Allocate an uninitialized variable for the key.
    const auto vkey = gcoll->create_variable();
    // Inject the variable into the current context.
    Reference_root::S_variable xref = { vkey };
    ctx_for.open_named_reference(name_key) = xref;

    // Create the mapped reference.
    auto& mapped = do_declare(ctx_for, name_mapped);
    // Evaluate the range initializer.
    auto status = queue_init.execute(ctx_for);
    ROCKET_ASSERT(status == air_status_next);

    // Set the range up, which isn't going to change even if the argument got modified by the loop body.
    mapped = ::std::move(ctx_for.stack().open_top());
    const auto range = mapped.read();
    if(range.is_null()) {
      // Do nothing.
    }
    else if(range.is_array()) {
      const auto& arr = range.as_array();
      for(int64_t i = 0;  i < arr.ssize();  ++i) {
        // Set the key which is the subscript of the mapped element in the array.
        vkey->initialize(i, true);
        // Set the mapped reference.
        Reference_modifier::S_array_index xmod = { i };
        mapped.zoom_in(::std::move(xmod));

        // Execute the loop body.
        status = do_execute_block(queue_body, ctx_for);
        if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for }))
          break;
        if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for }))
          return status;
        // Restore the mapped reference.
        mapped.zoom_out();
      }
    }
    else if(range.is_object()) {
      const auto& obj = range.as_object();
      for(auto it = obj.begin();  it != obj.end();  ++it) {
        // Set the key which is the key of this element in the object.
        vkey->initialize(it->first.rdstr(), true);
        // Set the mapped reference.
        Reference_modifier::S_object_key xmod = { it->first.rdstr() };
        mapped.zoom_in(::std::move(xmod));

        // Execute the loop body.
        status = do_execute_block(queue_body, ctx_for);
        if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for }))
          break;
        if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for }))
          return status;
        // Restore the mapped reference.
        mapped.zoom_out();
      }
    }
    else {
      ASTERIA_THROW("range value not iterable (range `$1`)", range);
    }
    return air_status_next;
  }

AIR_Status
do_for_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& queue_init = do_pcast<Pv_queues_fixed<4>>(pv)->queues[0];
    const auto& queue_cond = do_pcast<Pv_queues_fixed<4>>(pv)->queues[1];
    const auto& queue_step = do_pcast<Pv_queues_fixed<4>>(pv)->queues[2];
    const auto& queue_body = do_pcast<Pv_queues_fixed<4>>(pv)->queues[3];

    // This is the same as the `for` statement in C.
    // We have to create an outer context due to the fact that names declared in the first segment
    // outlast every iteration.
    Executive_Context ctx_for(::rocket::ref(ctx), nullptr);

    // Execute the loop initializer, which shall only be a definition or an expression statement.
    auto status = queue_init.execute(ctx_for);
    ROCKET_ASSERT(status == air_status_next);
    for(;;) {
      // Check the condition.
      status = queue_cond.execute(ctx_for);
      ROCKET_ASSERT(status == air_status_next);
      // This is a special case: If the condition is empty then the loop is infinite.
      if(!(ctx_for.stack().empty() || ctx_for.stack().get_top().read().test()))
        break;

      // Execute the body.
      status = do_execute_block(queue_body, ctx_for);
      if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for }))
        break;
      if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for }))
        return status;

      // Execute the increment.
      status = queue_step.execute(ctx_for);
      ROCKET_ASSERT(status == air_status_next);
    }
    return air_status_next;
  }

AIR_Status
do_try_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& queue_try = do_pcast<Pv_try>(pv)->queue_try;
    const auto& sloc_catch = do_pcast<Pv_try>(pv)->sloc_catch;
    const auto& name_except = do_pcast<Pv_try>(pv)->name_except;
    const auto& queue_catch = do_pcast<Pv_try>(pv)->queue_catch;

    // This is almost identical to JavaScript.
    AIR_Status status;
    ASTERIA_RUNTIME_TRY {
      // Execute the `try` block. If no exception is thrown, this will have little overhead.
      status = do_execute_block(queue_try, ctx);
      // This must not be PTC'd, otherwise exceptions thrown from tail calls won't be caught.
      if(status == air_status_return_ref)
        ctx.stack().open_top().finish_call(ctx.global());
    }
    ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
      // Reuse the exception object. Don't bother allocating a new one.
      except.push_frame_catch(sloc_catch);

      // This branch must be executed inside this `catch` block.
      // User-provided bindings may obtain the current exception using `::std::current_exception`.
      Executive_Context ctx_catch(::rocket::ref(ctx), nullptr);
      ASTERIA_RUNTIME_TRY {
        // Set the exception reference.
        Reference_root::S_temporary xref_except = { except.value() };
        ctx_catch.open_named_reference(name_except) = ::std::move(xref_except);

        // Set backtrace frames.
        V_array backtrace;
        for(size_t i = 0;  i < except.count_frames();  ++i) {
          const auto& f = except.frame(i);
          // Translate each frame into a human-readable format.
          V_object r;
          r.try_emplace(::rocket::sref("frame"), ::rocket::sref(f.what_type()));
          r.try_emplace(::rocket::sref("file"), f.file());
          r.try_emplace(::rocket::sref("line"), f.line());
          r.try_emplace(::rocket::sref("value"), f.value());
          // Append this frame.
          backtrace.emplace_back(::std::move(r));
        }
        Reference_root::S_constant xref = { ::std::move(backtrace) };
        ctx_catch.open_named_reference(::rocket::sref("__backtrace")) = ::std::move(xref);

        // Execute the `catch` clause.
        status = queue_catch.execute(ctx_catch);
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& nested) {
        ctx_catch.on_scope_exit(nested);
        throw;
      }
      ctx_catch.on_scope_exit(status);
    }
    return status;
  }

[[noreturn]]
AIR_Status
do_throw_statement(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc>(pv)->sloc;

    // Read the value to throw.
    // Note that the operand must not have been empty for this code.
    throw Runtime_Error(Runtime_Error::F_throw(), ctx.stack().get_top().read(), sloc);
  }

AIR_Status
do_assert_statement(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc_msg>(pv)->sloc;
    const auto& negative = static_cast<bool>(pu.u8s[0]);
    const auto& msg = do_pcast<Pv_sloc_msg>(pv)->msg;

    // Check the value of the condition.
    if(ROCKET_EXPECT(ctx.stack().get_top().read().test() != negative))
      // When the assertion succeeds, there is nothing to do.
      return air_status_next;

    // Throw a `Runtime_Error`.
    throw Runtime_Error(Runtime_Error::F_assert(), sloc, msg);
  }

AIR_Status
do_simple_status(Executive_Context& /*ctx*/, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& status = static_cast<AIR_Status>(pu.u8s[0]);

    // Return the status as is.
    return status;
  }

AIR_Status
do_glvalue_to_prvalue(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // Check for glvalues only. Void references and PTC wrappers are forwarded as is.
    auto& self = ctx.stack().open_top();
    if(self.is_prvalue())
      return air_status_next;

    // Convert the result to an rvalue.
    Reference_root::S_temporary xref = { self.read() };
    self = ::std::move(xref);
    return air_status_next;
  }

AIR_Status
do_push_immediate(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& val = do_pcast<Value>(pv)[0];

    // Push a constant.
    Reference_root::S_constant xref = { val };
    ctx.stack().push(::std::move(xref));
    return air_status_next;
  }

AIR_Status
do_push_global_reference(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& name = do_pcast<Pv_name>(pv)->name;

    // Look for the name in the global context.
    auto qref = ctx.global().get_named_reference_opt(name);
    if(!qref)
      ASTERIA_THROW("undeclared identifier `$1`", name);

    // Push a copy of it.
    ctx.stack().push(*qref);
    return air_status_next;
  }

AIR_Status
do_push_local_reference(Executive_Context& ctx, ParamU pu, const void* pv)
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
    if(!qref)
      ASTERIA_THROW("undeclared identifier `$1`", name);

    // Push a copy of it.
    ctx.stack().push(*qref);
    return air_status_next;
  }

AIR_Status
do_push_bound_reference(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& ref = do_pcast<Pv_ref>(pv)->ref;

    // Push a copy of the bound reference.
    ctx.stack().push(ref);
    return air_status_next;
  }

AIR_Status
do_define_function(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& opts = do_pcast<Pv_func>(pv)->opts;
    const auto& sloc = do_pcast<Pv_func>(pv)->sloc;
    const auto& func = do_pcast<Pv_func>(pv)->func;
    const auto& params = do_pcast<Pv_func>(pv)->params;
    const auto& code_body = do_pcast<Pv_func>(pv)->code_body;

    // Rewrite nodes in the body as necessary.
    AIR_Optimizer optmz(opts);
    optmz.rebind(&ctx, params, code_body);
    auto qtarget = optmz.create_function(sloc, func);

    // Push the function as a temporary.
    Reference_root::S_temporary xref = { ::std::move(qtarget) };
    ctx.stack().push(::std::move(xref));
    return air_status_next;
  }

AIR_Status
do_branch_expression(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);
    const auto& queue_true = do_pcast<Pv_queues_fixed<2>>(pv)->queues[0];
    const auto& queue_false = do_pcast<Pv_queues_fixed<2>>(pv)->queues[1];

    // Check the value of the condition.
    if(ctx.stack().get_top().read().test() != false)
      return do_evaluate_branch(queue_true, assign, ctx);
    else
      return do_evaluate_branch(queue_false, assign, ctx);
  }

AIR_Status
do_coalescence(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);
    const auto& queue_null = do_pcast<Pv_queues_fixed<1>>(pv)->queues[0];

    // Check the value of the condition.
    if(ctx.stack().get_top().read().is_null() != false)
      return do_evaluate_branch(queue_null, assign, ctx);
    else
      return air_status_next;
  }

ROCKET_NOINLINE
Reference&
do_invoke_nontail(Reference& self, const Source_Location& sloc, Executive_Context& ctx,
                  const cow_function& target, cow_vector<Reference>&& args)
  {
    const auto& inside = ctx.zvarg()->func();
    const auto& qhooks = ctx.global().get_hooks_opt();
    // Note exceptions thrown here are not caught.
    if(qhooks)
      qhooks->on_function_call(sloc, inside, target);

    ASTERIA_RUNTIME_TRY {
      target.invoke(self, ctx.global(), ::std::move(args));
    }
    ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
      if(qhooks)
        qhooks->on_function_except(sloc, inside, except);
      throw;
    }
    if(qhooks)
      qhooks->on_function_return(sloc, inside, self);
    return self;
  }

AIR_Status
do_function_call_common(Reference& self, const Source_Location& sloc, Executive_Context& ctx,
                        const cow_function& target, PTC_Aware ptc, cow_vector<Reference>&& args)
  {
    if(ROCKET_EXPECT(ptc == ptc_aware_none)) {
      // Perform plain calls.
      do_invoke_nontail(self, sloc, ctx, target, ::std::move(args));
      // The result will have been stored into `self`
      return air_status_next;
    }

    // Pack arguments for this proper tail call.
    args.emplace_back(::std::move(self));
    auto tca = ::rocket::make_refcnt<PTC_Arguments>(sloc, ctx.zvarg(), ptc, target, ::std::move(args));

    // Set the result, which will be unpacked outside this scope.
    Reference_root::S_tail_call xref = { ::std::move(tca) };
    self = ::std::move(xref);

    // Force `air_status_return_ref` if control flow reaches the end of a function.
    // Otherwise a null reference is returned instead of this PTC wrapper, which can then never be unpacked.
    return air_status_return_ref;
  }

cow_vector<Reference>
do_pop_positional_arguments(Executive_Context& ctx, size_t nargs)
  {
    cow_vector<Reference> args;
    args.resize(nargs, Reference_root::S_void());
    for(size_t i = args.size() - 1;  i != SIZE_MAX;  --i) {
      // Get an argument. Ensure it is dereferenceable.
      auto& arg = ctx.stack().open_top();
      static_cast<void>(arg.read());
      // Set the argument as is.
      args.mut(i) = ::std::move(arg);
      args.mut(i) = ::std::move(ctx.stack().open_top());
      ctx.stack().pop();
    }
    return args;
  }

AIR_Status
do_function_call(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc>(pv)->sloc;
    const auto& nargs = static_cast<size_t>(pu.y32);
    const auto& ptc = static_cast<PTC_Aware>(pu.y8s[0]);
    const auto& inside = ctx.zvarg()->func();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Check for stack overflows.
    const auto sentry = ctx.global().copy_recursion_sentry();
    // Generate a single-step trap.
    if(qhooks)
      qhooks->on_single_step_trap(sloc, inside, ::std::addressof(ctx));

    // Pop arguments off the stack backwards.
    auto args = do_pop_positional_arguments(ctx, nargs);

    // Copy the target, which shall be of type `function`.
    auto value = ctx.stack().get_top().read();
    if(!value.is_function()) {
      ASTERIA_THROW("attempt to call a non-function (value `$1`)", value);
    }
    auto& self = ctx.stack().open_top().zoom_out();

    return do_function_call_common(self, sloc, ctx, value.as_function(), ptc, ::std::move(args));
  }

AIR_Status
do_member_access(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& name = do_pcast<Pv_name>(pv)->name;

    // Append a modifier to the reference at the top.
    Reference_modifier::S_object_key xmod = { name };
    ctx.stack().open_top().zoom_in(::std::move(xmod));
    return air_status_next;
  }

AIR_Status
do_push_unnamed_array(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& nelems = static_cast<size_t>(pu.x32);

    // Pop elements from the stack and store them in an array backwards.
    V_array array;
    array.resize(nelems);
    for(auto it = array.mut_rbegin();  it != array.rend();  ++it) {
      // Write elements backwards.
      *it = ctx.stack().get_top().read();
      ctx.stack().pop();
    }
    // Push the array as a temporary.
    Reference_root::S_temporary xref = { ::std::move(array) };
    ctx.stack().push(::std::move(xref));
    return air_status_next;
  }

AIR_Status
do_push_unnamed_object(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& keys = do_pcast<Pv_names>(pv)->names;

    // Pop elements from the stack and store them in an object backwards.
    V_object object;
    object.reserve(keys.size());
    for(auto it = keys.rbegin();  it != keys.rend();  ++it) {
      // Use `try_emplace()` instead of `insert_or_assign()`. In case of duplicate keys,
      // the last value takes precedence.
      object.try_emplace(*it, ctx.stack().get_top().read());
      ctx.stack().pop();
    }
    // Push the object as a temporary.
    Reference_root::S_temporary xref = { ::std::move(object) };
    ctx.stack().push(::std::move(xref));
    return air_status_next;
  }

ROCKET_PURE_FUNCTION
V_boolean
do_operator_NOT(bool rhs)
  {
    return !rhs;
  }

ROCKET_PURE_FUNCTION
V_boolean
do_operator_AND(bool lhs, bool rhs)
  {
    return lhs & rhs;
  }

ROCKET_PURE_FUNCTION
V_boolean
do_operator_OR(bool lhs, bool rhs)
  {
    return lhs | rhs;
  }

ROCKET_PURE_FUNCTION
V_boolean
do_operator_XOR(bool lhs, bool rhs)
  {
    return lhs ^ rhs;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_NEG(int64_t rhs)
  {
    if(rhs == INT64_MIN) {
      ASTERIA_THROW("integer negation overflow (operand was `$1`)", rhs);
    }
    return -rhs;
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_SQRT(int64_t rhs)
  {
    return ::std::sqrt(double(rhs));
  }

ROCKET_PURE_FUNCTION
V_boolean
do_operator_ISINF(int64_t /*rhs*/)
  {
    return false;
  }

ROCKET_PURE_FUNCTION
V_boolean
do_operator_ISNAN(int64_t /*rhs*/)
  {
    return false;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_ABS(int64_t rhs)
  {
    if(rhs == INT64_MIN) {
      ASTERIA_THROW("integer absolute value overflow (operand was `$1`)", rhs);
    }
    return ::std::abs(rhs);
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_SIGN(int64_t rhs)
  {
    return rhs >> 63;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_ADD(int64_t lhs, int64_t rhs)
  {
    if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
      ASTERIA_THROW("integer addition overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return lhs + rhs;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_SUB(int64_t lhs, int64_t rhs)
  {
    if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
      ASTERIA_THROW("integer subtraction overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return lhs - rhs;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_MUL(int64_t lhs, int64_t rhs)
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

ROCKET_PURE_FUNCTION
V_integer
do_operator_DIV(int64_t lhs, int64_t rhs)
  {
    if(rhs == 0) {
      ASTERIA_THROW("integer divided by zero (operands were `$1` and `$2`)", lhs, rhs);
    }
    if((lhs == INT64_MIN) && (rhs == -1)) {
      ASTERIA_THROW("integer division overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return lhs / rhs;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_MOD(int64_t lhs, int64_t rhs)
  {
    if(rhs == 0) {
      ASTERIA_THROW("integer divided by zero (operands were `$1` and `$2`)", lhs, rhs);
    }
    if((lhs == INT64_MIN) && (rhs == -1)) {
      ASTERIA_THROW("integer division overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return lhs % rhs;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_SLL(int64_t lhs, int64_t rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    if(rhs >= 64) {
      return 0;
    }
    return int64_t(uint64_t(lhs) << rhs);
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_SRL(int64_t lhs, int64_t rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    if(rhs >= 64) {
      return 0;
    }
    return int64_t(uint64_t(lhs) >> rhs);
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_SLA(int64_t lhs, int64_t rhs)
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
    auto bc = 63 - int(rhs);
    auto mask_out = uint64_t(lhs) >> bc << bc;
    auto mask_sbt = uint64_t(lhs >> 63) << bc;
    if(mask_out != mask_sbt) {
      ASTERIA_THROW("integer left shift overflow (operands were `$1` and `$2`)", lhs, rhs);
    }
    return int64_t(uint64_t(lhs) << rhs);
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_SRA(int64_t lhs, int64_t rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    if(rhs >= 64) {
      return lhs >> 63;
    }
    return lhs >> rhs;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_NOT(int64_t rhs)
  {
    return ~rhs;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_AND(int64_t lhs, int64_t rhs)
  {
    return lhs & rhs;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_OR(int64_t lhs, int64_t rhs)
  {
    return lhs | rhs;
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_XOR(int64_t lhs, int64_t rhs)
  {
    return lhs ^ rhs;
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_NEG(double rhs)
  {
    return -rhs;
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_SQRT(double rhs)
  {
    return ::std::sqrt(rhs);
  }

ROCKET_PURE_FUNCTION
V_boolean
do_operator_ISINF(double rhs)
  {
    return ::std::isinf(rhs);
  }

ROCKET_PURE_FUNCTION
V_boolean
do_operator_ISNAN(double rhs)
  {
    return ::std::isnan(rhs);
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_ABS(double rhs)
  {
    return ::std::fabs(rhs);
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_SIGN(double rhs)
  {
    return ::std::signbit(rhs) ? -1 : 0;
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_ROUND(double rhs)
  {
    return ::std::round(rhs);
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_FLOOR(double rhs)
  {
    return ::std::floor(rhs);
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_CEIL(double rhs)
  {
    return ::std::ceil(rhs);
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_TRUNC(double rhs)
  {
    return ::std::trunc(rhs);
  }

V_integer
do_cast_to_integer(double value)
  {
    if(!::std::islessequal(-0x1p63, value) || !::std::islessequal(value, 0x1p63 - 0x1p10)) {
      ASTERIA_THROW("value not representable as an `integer` (operand was `$1`)", value);
    }
    return int64_t(value);
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_IROUND(double rhs)
  {
    return do_cast_to_integer(::std::round(rhs));
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_IFLOOR(double rhs)
  {
    return do_cast_to_integer(::std::floor(rhs));
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_ICEIL(double rhs)
  {
    return do_cast_to_integer(::std::ceil(rhs));
  }

ROCKET_PURE_FUNCTION
V_integer
do_operator_ITRUNC(double rhs)
  {
    return do_cast_to_integer(::std::trunc(rhs));
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_ADD(double lhs, double rhs)
  {
    return lhs + rhs;
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_SUB(double lhs, double rhs)
  {
    return lhs - rhs;
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_MUL(double lhs, double rhs)
  {
    return lhs * rhs;
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_DIV(double lhs, double rhs)
  {
    return lhs / rhs;
  }

ROCKET_PURE_FUNCTION
V_real
do_operator_MOD(double lhs, double rhs)
  {
    return ::std::fmod(lhs, rhs);
  }

ROCKET_PURE_FUNCTION
V_string
do_operator_ADD(const cow_string& lhs, const cow_string& rhs)
  {
    return lhs + rhs;
  }

V_string
do_duplicate_string(const cow_string& source, uint64_t count)
  {
    V_string res;
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
    // Append the result string to itself, doubling its length, until more than half of the result string
    // has been populated.
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

ROCKET_PURE_FUNCTION
V_string
do_operator_MUL(const cow_string& lhs, int64_t rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative duplicate count (operands were `$1` and `$2`)", lhs, rhs);
    }
    return do_duplicate_string(lhs, static_cast<uint64_t>(rhs));
  }

ROCKET_PURE_FUNCTION
V_string
do_operator_MUL(int64_t lhs, const cow_string& rhs)
  {
    if(lhs < 0) {
      ASTERIA_THROW("negative duplicate count (operands were `$1` and `$2`)", lhs, rhs);
    }
    return do_duplicate_string(rhs, static_cast<uint64_t>(lhs));
  }

ROCKET_PURE_FUNCTION
V_string
do_operator_SLL(const cow_string& lhs, int64_t rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    V_string res;
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

ROCKET_PURE_FUNCTION
V_string
do_operator_SRL(const cow_string& lhs, int64_t rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    V_string res;
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

ROCKET_PURE_FUNCTION
V_string
do_operator_SLA(const cow_string& lhs, int64_t rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    V_string res;
    if(static_cast<uint64_t>(rhs) >= res.max_size() - lhs.size()) {
      ASTERIA_THROW("string length overflow (`$1` + `$2` > `$3`)", lhs.size(), rhs, res.max_size());
    }
    // Append spaces in the right and return the result.
    size_t count = static_cast<size_t>(rhs);
    res.assign(::rocket::sref(lhs));
    res.append(count, ' ');
    return res;
  }

ROCKET_PURE_FUNCTION
V_string
do_operator_SRA(const cow_string& lhs, int64_t rhs)
  {
    if(rhs < 0) {
      ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
    }
    V_string res;
    if(static_cast<uint64_t>(rhs) >= lhs.size()) {
      return res;
    }
    // Return the substring in the left.
    size_t count = static_cast<size_t>(rhs);
    res.append(lhs.data(), lhs.size() - count);
    return res;
  }

ROCKET_PURE_FUNCTION
V_string
do_operator_NOT(cow_string&& rhs)
  {
    // The length of the result is the same as the operand.
    auto tp = reinterpret_cast<uint8_t*>(rhs.mut_data());

    // Flip every byte in the string.
    for(size_t i = 0;  i < rhs.size();  ++i)
      tp[i] = static_cast<uint8_t>(-1 ^ tp[i]);
    return ::std::move(rhs);
  }

ROCKET_PURE_FUNCTION
V_string
do_operator_AND(const cow_string& lhs, cow_string&& rhs)
  {
    // The length of the result is the shorter from both operands.
    auto sp = reinterpret_cast<const uint8_t*>(lhs.data());
    if(rhs.size() > lhs.size())
      rhs.erase(lhs.size());
    auto tp = reinterpret_cast<uint8_t*>(rhs.mut_data());

    // Bitwise-AND every pair of bytes from both strings.
    for(size_t i = 0;  i < rhs.size();  ++i)
      tp[i] = static_cast<uint8_t>(sp[i] & tp[i]);
    return ::std::move(rhs);
  }

ROCKET_PURE_FUNCTION
V_string
do_operator_OR(const cow_string& lhs, cow_string&& rhs)
  {
    // The length of the result is the longer from both operands.
    auto sp = reinterpret_cast<const uint8_t*>(lhs.data());
    if(rhs.size() < lhs.size())
      rhs.append(lhs.size() - rhs.size(), 0);
    auto tp = reinterpret_cast<uint8_t*>(rhs.mut_data());

    // Bitwise-OR every pair of bytes from both strings.
    for(size_t i = 0;  i < lhs.size();  ++i)
      tp[i] = static_cast<uint8_t>(sp[i] | tp[i]);
    return ::std::move(rhs);
  }

ROCKET_PURE_FUNCTION
V_string
do_operator_XOR(const cow_string& lhs, cow_string&& rhs)
  {
    // The length of the result is the longer from both operands.
    auto sp = reinterpret_cast<const uint8_t*>(lhs.data());
    if(rhs.size() < lhs.size())
      rhs.append(lhs.size() - rhs.size(), 0);
    auto tp = reinterpret_cast<uint8_t*>(rhs.mut_data());

    // Bitwise-XOR every pair of bytes from both strings.
    for(size_t i = 0;  i < lhs.size();  ++i)
      tp[i] = static_cast<uint8_t>(sp[i] ^ tp[i]);
    return ::std::move(rhs);
  }

AIR_Status
do_apply_xop_INC_POST(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& lhs = ctx.stack().get_top().open();
    // Increment the operand and return the old value. `assign` is ignored.
    if(lhs.is_integer()) {
      auto& reg = lhs.open_integer();
      do_set_temporary(ctx.stack(), false, ::std::move(lhs));
      reg = do_operator_ADD(reg, V_integer(1));
    }
    else if(lhs.is_real()) {
      auto& reg = lhs.open_real();
      do_set_temporary(ctx.stack(), false, ::std::move(lhs));
      reg = do_operator_ADD(reg, V_real(1));
    }
    else {
      ASTERIA_THROW("postfix increment not applicable (operand was `$1`)", lhs);
    }
    return air_status_next;
  }

AIR_Status
do_apply_xop_DEC_POST(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& lhs = ctx.stack().get_top().open();
    // Decrement the operand and return the old value. `assign` is ignored.
    if(lhs.is_integer()) {
      auto& reg = lhs.open_integer();
      do_set_temporary(ctx.stack(), false, ::std::move(lhs));
      reg = do_operator_SUB(reg, V_integer(1));
    }
    else if(lhs.is_real()) {
      auto& reg = lhs.open_real();
      do_set_temporary(ctx.stack(), false, ::std::move(lhs));
      reg = do_operator_SUB(reg, V_real(1));
    }
    else {
      ASTERIA_THROW("postfix decrement not applicable (operand was `$1`)", lhs);
    }
    return air_status_next;
  }

AIR_Status
do_apply_xop_SUBSCR(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    auto& lref = ctx.stack().open_top();
    // Append a reference modifier. `assign` is ignored.
    if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      Reference_modifier::S_array_index xmod = { ::std::move(reg) };
      lref.zoom_in(::std::move(xmod));
    }
    else if(rhs.is_string()) {
      auto& reg = rhs.open_string();
      Reference_modifier::S_object_key xmod = { ::std::move(reg) };
      lref.zoom_in(::std::move(xmod));
    }
    else {
      ASTERIA_THROW("subscript value not valid (subscript was `$1`)", rhs);
    }
    return air_status_next;
  }

AIR_Status
do_apply_xop_POS(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Copy the operand to create a temporary value, then return it.
    // N.B. This is one of the few operators that work on all types.
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_NEG(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_NOTB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    else if(rhs.is_string()) {
      auto& reg = rhs.open_string();
      reg = do_operator_NOT(::std::move(reg));
    }
    else {
      ASTERIA_THROW("prefix bitwise NOT not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_NOTL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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

AIR_Status
do_apply_xop_INC_PRE(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& rhs = ctx.stack().get_top().open();
    // Increment the operand and return it. `assign` is ignored.
    if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      reg = do_operator_ADD(reg, V_integer(1));
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_ADD(reg, V_real(1));
    }
    else {
      ASTERIA_THROW("prefix increment not applicable (operand was `$1`)", rhs);
    }
    return air_status_next;
  }

AIR_Status
do_apply_xop_DEC_PRE(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& rhs = ctx.stack().get_top().open();
    // Decrement the operand and return it. `assign` is ignored.
    if(rhs.is_integer()) {
      auto& reg = rhs.open_integer();
      reg = do_operator_SUB(reg, V_integer(1));
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_SUB(reg, V_real(1));
    }
    else {
      ASTERIA_THROW("prefix decrement not applicable (operand was `$1`)", rhs);
    }
    return air_status_next;
  }

AIR_Status
do_apply_xop_UNSET(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().unset();
    // Unset the reference and return the old value.
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_LENGTHOF(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    const auto& rhs = ctx.stack().get_top().read();
    // Return the number of elements in the operand.
    ptrdiff_t nelems;
    switch(weaken_enum(rhs.vtype())) {
      case vtype_null: {
        nelems = 0;
        break;
      }
      case vtype_string: {
        nelems = rhs.as_string().ssize();
        break;
      }
      case vtype_array: {
        nelems = rhs.as_array().ssize();
        break;
      }
      case vtype_object: {
        nelems = rhs.as_object().ssize();
        break;
      }
      default:
        ASTERIA_THROW("prefix `countof` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, nelems);
    return air_status_next;
  }

AIR_Status
do_apply_xop_TYPEOF(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    const auto& rhs = ctx.stack().get_top().read();
    // Return the type name of the operand.
    // N.B. This is one of the few operators that work on all types.
    do_set_temporary(ctx.stack(), assign, ::rocket::sref(rhs.what_vtype()));
    return air_status_next;
  }

AIR_Status
do_apply_xop_SQRT(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Get the square root of the operand as a temporary value, then return it.
    if(rhs.is_integer()) {
      // Note that `rhs` does not have type `V_real`, thus this branch can't be optimized.
      rhs = do_operator_SQRT(rhs.as_integer());
    }
    else if(rhs.is_real()) {
      auto& reg = rhs.open_real();
      reg = do_operator_SQRT(reg);
    }
    else {
      ASTERIA_THROW("prefix `__sqrt` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_ISNAN(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Check whether the operand is a NaN, store the result in a temporary value, then return it.
    if(rhs.is_integer()) {
      // Note that `rhs` does not have type `V_boolean`, thus this branch can't be optimized.
      rhs = do_operator_ISNAN(rhs.as_integer());
    }
    else if(rhs.is_real()) {
      // Note that `rhs` does not have type `V_boolean`, thus this branch can't be optimized.
      rhs = do_operator_ISNAN(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__isnan` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_ISINF(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is unary.
    auto rhs = ctx.stack().get_top().read();
    // Check whether the operand is an infinity, store the result in a temporary value, then return it.
    if(rhs.is_integer()) {
      // Note that `rhs` does not have type `V_boolean`, thus this branch can't be optimized.
      rhs = do_operator_ISINF(rhs.as_integer());
    }
    else if(rhs.is_real()) {
      // Note that `rhs` does not have type `V_boolean`, thus this branch can't be optimized.
      rhs = do_operator_ISINF(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__isinf` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_ABS(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_SIGN(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` does not have type `V_integer`, thus this branch can't be optimized.
      rhs = do_operator_SIGN(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__sign` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_ROUND(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_FLOOR(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_CEIL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_TRUNC(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_IROUND(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` does not have type `V_integer`, thus this branch can't be optimized.
      rhs = do_operator_IROUND(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__iround` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_IFLOOR(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` does not have type `V_integer`, thus this branch can't be optimized.
      rhs = do_operator_IFLOOR(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__ifloor` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_ICEIL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` does not have type `V_integer`, thus this branch can't be optimized.
      rhs = do_operator_ICEIL(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__iceil` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_ITRUNC(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` does not have type `V_integer`, thus this branch can't be optimized.
      rhs = do_operator_ITRUNC(rhs.as_real());
    }
    else {
      ASTERIA_THROW("prefix `__itrunc` not applicable (operand was `$1`)", rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_CMP_XEQ(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    do_set_temporary(ctx.stack(), assign, (comp == expect) != negative);
    return air_status_next;
  }

AIR_Status
do_apply_xop_CMP_XREL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    do_set_temporary(ctx.stack(), assign, (comp == expect) != negative);
    return air_status_next;
  }

AIR_Status
do_apply_xop_CMP_3WAY(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
        do_set_temporary(ctx.stack(), assign, +1);
        break;
      }
      case compare_less: {
        do_set_temporary(ctx.stack(), assign, -1);
        break;
      }
      case compare_equal: {
        do_set_temporary(ctx.stack(), assign, 0);
        break;
      }
      case compare_unordered: {
        do_set_temporary(ctx.stack(), assign, ::rocket::sref("<unordered>"));
        break;
      }
      default:
        ROCKET_ASSERT(false);
    }
    return air_status_next;
  }

AIR_Status
do_apply_xop_ADD(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` might not have type `V_real`, thus this branch can't be optimized.
      rhs = do_operator_ADD(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else if(lhs.is_string() && rhs.is_string()) {
      // For the `string` type, concatenate the operands in lexical order to create a new string.
      auto& reg = rhs.open_string();
      reg = do_operator_ADD(lhs.as_string(), reg);
    }
    else {
      ASTERIA_THROW("infix addition not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_SUB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` might not have type `V_real`, thus this branch can't be optimized.
      rhs = do_operator_SUB(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else {
      ASTERIA_THROW("infix subtraction not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_MUL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` might not have type `V_real`, thus this branch can't be optimized.
      rhs = do_operator_MUL(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If either operand has type `string` and the other has type `integer`, duplicate the string up to
      // the specified number of times and return the result.
      // Note that `rhs` does not have type `V_string`, thus this branch can't be optimized.
      rhs = do_operator_MUL(lhs.as_string(), rhs.as_integer());
    }
    else if(lhs.is_integer() && rhs.is_string()) {
      auto& reg = rhs.open_string();
      reg = do_operator_MUL(lhs.as_integer(), reg);
    }
    else {
      ASTERIA_THROW("infix multiplication not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_DIV(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` might not have type `V_real`, thus this branch can't be optimized.
      rhs = do_operator_DIV(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else {
      ASTERIA_THROW("infix division not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_MOD(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` might not have type `V_real`, thus this branch can't be optimized.
      rhs = do_operator_MOD(lhs.convert_to_real(), rhs.convert_to_real());
    }
    else {
      ASTERIA_THROW("infix modulo not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_SLL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // If the LHS operand has type `integer`, shift the LHS operand to the left by the number of bits specified
      // by the RHS operand. Bits shifted out are discarded. Bits shifted in are filled with zeroes.
      auto& reg = rhs.open_integer();
      reg = do_operator_SLL(lhs.as_integer(), reg);
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If the LHS operand has type `string`, fill space characters in the right and discard characters from the
      // left. The number of bytes in the LHS operand will be preserved.
      // Note that `rhs` does not have type `V_string`, thus this branch can't be optimized.
      rhs = do_operator_SLL(lhs.as_string(), rhs.as_integer());
    }
    else {
      ASTERIA_THROW("infix logical left shift not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_SRL(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // If the LHS operand has type `integer`, shift the LHS operand to the right by the number of bits
      // specified by the RHS operand. Bits shifted out are discarded. Bits shifted in are filled with zeroes.
      auto& reg = rhs.open_integer();
      reg = do_operator_SRL(lhs.as_integer(), reg);
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If the LHS operand has type `string`, fill space characters in the left and discard characters from
      // the right. The number of bytes in the LHS operand will be preserved.
      // Note that `rhs` does not have type `V_string`, thus this branch can't be optimized.
      rhs = do_operator_SRL(lhs.as_string(), rhs.as_integer());
    }
    else {
      ASTERIA_THROW("infix logical right shift not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_SLA(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // If the LHS operand is of type `integer`, shift the LHS operand to the left by the number of bits
      // specified by the RHS operand. Bits shifted out that are equal to the sign bit are discarded. Bits
      // shifted in are filled with zeroes. If any bits that are different from the sign bit would be shifted
      // out, an exception is thrown.
      auto& reg = rhs.open_integer();
      reg = do_operator_SLA(lhs.as_integer(), reg);
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If the LHS operand has type `string`, fill space characters in the right.
      // Note that `rhs` does not have type `V_string`, thus this branch can't be optimized.
      rhs = do_operator_SLA(lhs.as_string(), rhs.as_integer());
    }
    else {
      ASTERIA_THROW("infix arithmetic left shift not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_SRA(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& assign = static_cast<bool>(pu.u8s[0]);

    // This operator is binary.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    const auto& lhs = ctx.stack().get_top().read();
    if(lhs.is_integer() && rhs.is_integer()) {
      // If the LHS operand is of type `integer`, shift the LHS operand to the right by the number of bits
      // specified by the RHS operand. Bits shifted out are discarded. Bits shifted in are filled with the
      // sign bit.
      auto& reg = rhs.open_integer();
      reg = do_operator_SRA(lhs.as_integer(), reg);
    }
    else if(lhs.is_string() && rhs.is_integer()) {
      // If the LHS operand has type `string`, discard characters from the right.
      // Note that `rhs` does not have type `V_string`, thus this branch can't be optimized.
      rhs = do_operator_SRA(lhs.as_string(), rhs.as_integer());
    }
    else {
      ASTERIA_THROW("infix arithmetic right shift not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_ANDB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    else if(lhs.is_string() && rhs.is_string()) {
      // For the `string` type, return bitwise AND'd result of bytes from operands.
      // The result contains no more bytes than either operand.
      auto& reg = rhs.open_string();
      reg = do_operator_AND(lhs.as_string(), ::std::move(reg));
    }
    else {
      ASTERIA_THROW("infix bitwise AND not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_ORB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    else if(lhs.is_string() && rhs.is_string()) {
      // For the `string` type, return bitwise OR'd result of bytes from operands.
      // The result contains no fewer bytes than either operand.
      auto& reg = rhs.open_string();
      reg = do_operator_OR(lhs.as_string(), ::std::move(reg));
    }
    else {
      ASTERIA_THROW("infix bitwise OR not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_XORB(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
    else if(lhs.is_string() && rhs.is_string()) {
      // For the `string` type, return bitwise XOR'd result of bytes from operands.
      // The result contains no fewer bytes than either operand.
      auto& reg = rhs.open_string();
      reg = do_operator_XOR(lhs.as_string(), ::std::move(reg));
    }
    else {
      ASTERIA_THROW("infix bitwise XOR not defined (operands were `$1` and `$2`)", lhs, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_ASSIGN(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // Pop the RHS operand.
    auto rhs = ctx.stack().get_top().read();
    ctx.stack().pop();
    // Copy the value to the LHS operand which is write-only. `assign` is ignored.
    do_set_temporary(ctx.stack(), true, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_FMA(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
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
      // Note that `rhs` might not have type `V_real`, thus this branch can't be optimized.
      rhs = ::std::fma(lhs.convert_to_real(), mid.convert_to_real(), rhs.convert_to_real());
    }
    else {
      ASTERIA_THROW("fused multiply-add not defined (operands were `$1`, `$2` and `$3`)", lhs, mid, rhs);
    }
    do_set_temporary(ctx.stack(), assign, ::std::move(rhs));
    return air_status_next;
  }

AIR_Status
do_apply_xop_HEAD(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& lref = ctx.stack().open_top();
    Reference_modifier::S_array_head xmod = { };
    lref.zoom_in(::std::move(xmod));
    return air_status_next;
  }

AIR_Status
do_apply_xop_TAIL(Executive_Context& ctx, ParamU /*pu*/, const void* /*pv*/)
  {
    // This operator is unary.
    auto& lref = ctx.stack().open_top();
    Reference_modifier::S_array_tail xmod = { };
    lref.zoom_in(::std::move(xmod));
    return air_status_next;
  }

AIR_Status
do_unpack_struct_array(Executive_Context& ctx, ParamU pu, const void* /*pv*/)
  {
    // Unpack arguments.
    const auto& immutable = static_cast<bool>(pu.y8s[0]);
    const auto& nelems = static_cast<size_t>(pu.y32);

    // Read the value of the initializer.
    // Note that the initializer must not have been empty for this function.
    auto val = ctx.stack().get_top().read();
    ctx.stack().pop();
    // Make sure it is really an `array`.
    V_array arr;
    if(!val.is_null()) {
      if(!val.is_array()) {
        ASTERIA_THROW("invalid argument for structured array binding (initializer was `$1`)", val);
      }
      arr = ::std::move(val.open_array());
    }
    for(size_t i = nelems - 1;  i != SIZE_MAX;  --i) {
      // Get the variable back.
      auto var = ctx.stack().get_top().get_variable_opt();
      ctx.stack().pop();
      // Initialize it.
      ROCKET_ASSERT(var && !var->is_initialized());
      auto qinit = arr.mut_ptr(i);
      if(qinit)
        var->initialize(::std::move(*qinit), immutable);
      else
        var->initialize(nullptr, immutable);
    }
    return air_status_next;
  }

AIR_Status
do_unpack_struct_object(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& immutable = static_cast<bool>(pu.u8s[0]);
    const auto& keys = do_pcast<Pv_names>(pv)->names;

    // Read the value of the initializer.
    // Note that the initializer must not have been empty for this function.
    auto val = ctx.stack().get_top().read();
    ctx.stack().pop();
    // Make sure it is really an `object`.
    V_object obj;
    if(!val.is_null()) {
      if(!val.is_object()) {
        ASTERIA_THROW("invalid argument for structured object binding (initializer was `$1`)", val);
      }
      obj = ::std::move(val.open_object());
    }
    for(auto it = keys.rbegin();  it != keys.rend();  ++it) {
      // Get the variable back.
      auto var = ctx.stack().get_top().get_variable_opt();
      ctx.stack().pop();
      // Initialize it.
      ROCKET_ASSERT(var && !var->is_initialized());
      auto qinit = obj.mut_ptr(*it);
      if(qinit)
        var->initialize(::std::move(*qinit), immutable);
      else
        var->initialize(nullptr, immutable);
    }
    return air_status_next;
  }

AIR_Status
do_define_null_variable(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& immutable = static_cast<bool>(pu.u8s[0]);
    const auto& sloc = do_pcast<Pv_sloc_name>(pv)->sloc;
    const auto& name = do_pcast<Pv_sloc_name>(pv)->name;
    const auto& inside = ctx.zvarg()->func();
    const auto& gcoll = ctx.global().genius_collector();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Allocate an uninitialized variable.
    auto var = gcoll->create_variable();
    // Inject the variable into the current context.
    Reference_root::S_variable xref = { var };
    ctx.open_named_reference(name) = ::std::move(xref);
    // Call the hook function if any.
    if(qhooks)
      qhooks->on_variable_declare(sloc, inside, name);
    // Initialize the variable to `null`.
    var->initialize(nullptr, immutable);
    return air_status_next;
  }

AIR_Status
do_single_step_trap(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc>(pv)->sloc;
    const auto& inside = ctx.zvarg()->func();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Call the hook function if any.
    if(qhooks)
      qhooks->on_single_step_trap(sloc, inside, ::std::addressof(ctx));
    return air_status_next;
  }

AIR_Status
do_variadic_call(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_sloc>(pv)->sloc;
    const auto& ptc = static_cast<PTC_Aware>(pu.u8s[0]);
    const auto& inside = ctx.zvarg()->func();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Check for stack overflows.
    const auto sentry = ctx.global().copy_recursion_sentry();
    // Generate a single-step trap.
    if(qhooks)
      qhooks->on_single_step_trap(sloc, inside, ::std::addressof(ctx));

    // Pop the argument generator.
    cow_vector<Reference> args;
    auto value = ctx.stack().get_top().read();
    if(value.is_null()) {
      // Leave `args` empty.
    }
    else if(value.is_array()) {
      auto source = ::std::move(value.open_array());
      ctx.stack().pop();
      // Convert all elements to temporaries.
      args.assign(source.size(), Reference_root::S_void());
      for(size_t i = 0;  i < args.size();  ++i) {
        // Make a reference to temporary.
        Reference_root::S_temporary xref = { ::std::move(source.mut(i)) };
        args.mut(i) = ::std::move(xref);
      }
    }
    else if(value.is_function()) {
      const auto generator = ::std::move(value.open_function());
      auto gself = ctx.stack().open_top().zoom_out();
      // Pass an empty argument list to get the number of arguments to generate.
      cow_vector<Reference> gargs;
      do_invoke_nontail(ctx.stack().open_top(), sloc, ctx, generator, ::std::move(gargs));
      value = ctx.stack().get_top().read();
      ctx.stack().pop();
      // Verify the argument count.
      if(!value.is_integer()) {
        ASTERIA_THROW("invalid number of variadic arguments (value `$1`)", value);
      }
      int64_t nvargs = value.as_integer();
      if((nvargs < 0) || (nvargs > INT_MAX)) {
        ASTERIA_THROW("number of variadic arguments not acceptable (nvargs `$1`)", nvargs);
      }
      // Generate arguments.
      args.assign(static_cast<size_t>(nvargs), gself);
      for(size_t i = 0;  i < args.size();  ++i) {
        // Initialize the argument list for the generator.
        Reference_root::S_constant xref = { int64_t(i) };
        gargs.clear().emplace_back(::std::move(xref));
        // Generate an argument. Ensure it is dereferenceable.
        do_invoke_nontail(args.mut(i), sloc, ctx, generator, ::std::move(gargs));
        static_cast<void>(args[i].read());
      }
    }
    else
      ASTERIA_THROW("invalid variadic argument generator (value `$1`)", value);

    // Copy the target, which shall be of type `function`.
    value = ctx.stack().get_top().read();
    if(!value.is_function()) {
      ASTERIA_THROW("attempt to call a non-function (value `$1`)", value);
    }
    auto& self = ctx.stack().open_top().zoom_out();

    return do_function_call_common(self, sloc, ctx, value.as_function(), ptc, ::std::move(args));
  }

AIR_Status
do_defer_expression(Executive_Context& ctx, ParamU /*pu*/, const void* pv)
  {
    // Unpack arguments.
    const auto& sloc = do_pcast<Pv_defer>(pv)->sloc;
    const auto& code_body = do_pcast<Pv_defer>(pv)->code_body;

    // Rebind the body here.
    bool dirty = false;
    auto bound_body = code_body;
    do_rebind_nodes(dirty, bound_body, ctx);

    // Solidify it.
    AVMC_Queue queue;
    queue.reload(bound_body);

    // Push this expression.
    ctx.defer_expression(sloc, ::std::move(queue));
    return air_status_next;
  }

AIR_Status
do_import_call(Executive_Context& ctx, ParamU pu, const void* pv)
  {
    // Unpack arguments.
    const auto& opts = do_pcast<Pv_opts_sloc>(pv)->opts;
    const auto& sloc = do_pcast<Pv_opts_sloc>(pv)->sloc;
    const auto& nargs = static_cast<size_t>(pu.y32);
    const auto& inside = ctx.zvarg()->func();
    const auto& qhooks = ctx.global().get_hooks_opt();

    // Check for stack overflows.
    const auto sentry = ctx.global().copy_recursion_sentry();
    // Generate a single-step trap.
    if(qhooks)
      qhooks->on_single_step_trap(sloc, inside, ::std::addressof(ctx));

    // Pop arguments off the stack backwards.
    ROCKET_ASSERT(nargs != 0);
    auto args = do_pop_positional_arguments(ctx, nargs - 1);

    // Copy the filename, which shall be of type `string`.
    auto value = ctx.stack().get_top().read();
    if(!value.is_string()) {
      ASTERIA_THROW("invalid path specified for `import` (value `$1` not a string)", value);
    }
    auto path = value.as_string();
    if(path.empty()) {
      ASTERIA_THROW("empty path specified for `import`");
    }
    // Rewrite the path if it is not absolute.
    if((path[0] != '/') && (sloc.c_file()[0] == '/')) {
      path.assign(sloc.file());
      path.erase(path.rfind('/') + 1);
      path.append(value.as_string());
    }
    // Canonicalize it.
    uptr<char, void (&)(void*)> abspath(::realpath(path.safe_c_str(), nullptr), ::free);
    if(!abspath) {
      int err = errno;
      char sbuf[256];
      const char* msg = ::strerror_r(err, sbuf, sizeof(sbuf));
      ASTERIA_THROW("import failure (path `$1` => '$2', errno was `$3`: $4)", value.as_string(), path, err, msg);
    }
    path.assign(abspath);

    // Compile the script file into a function object.
    Loader_Lock::Unique_Stream strm;
    strm.reset(ctx.global().loader_lock(), path.safe_c_str());

    // Parse source code.
    Token_Stream tstrm(opts);
    tstrm.reload(strm, path);

    Statement_Sequence stmtq(opts);
    stmtq.reload(tstrm);

    // Instantiate the function.
    AIR_Optimizer optmz(opts);
    optmz.reload(nullptr, cow_vector<phsh_string>(1, ::rocket::sref("...")), stmtq);
    auto qtarget = optmz.create_function(Source_Location(path, 0, 0), ::rocket::sref("<file scope>"));

    // Update the first argument to `import` if it was passed by reference.
    // `this` is null for imported scripts.
    auto& self = ctx.stack().open_top();
    if(self.is_lvalue()) {
      self.open() = path;
    }
    self = Reference_root::S_constant();

    return do_function_call_common(self, sloc, ctx, qtarget, ptc_aware_none, ::std::move(args));
  }

}  // namespace

opt<AIR_Node>
AIR_Node::
rebind_opt(const Abstract_Context& ctx)
const
  {
    switch(this->index()) {
      case index_clear_stack:
        // There is nothing to rebind.
        return nullopt;

      case index_execute_block: {
        const auto& altr = this->m_stor.as<index_execute_block>();

        // Rebind the body.
        Analytic_Context ctx_body(::rocket::ref(ctx), nullptr);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx_body);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_declare_variable:
      case index_initialize_variable:
        // There is nothing to rebind.
        return nullopt;

      case index_if_statement: {
        const auto& altr = this->m_stor.as<index_if_statement>();

        // Rebind both branches.
        Analytic_Context ctx_body(::rocket::ref(ctx), nullptr);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_true, ctx_body);
        do_rebind_nodes(dirty, bound.code_false, ctx_body);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_switch_statement: {
        const auto& altr = this->m_stor.as<index_switch_statement>();

        // Rebind all clauses.
        Analytic_Context ctx_body(::rocket::ref(ctx), nullptr);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_labels, ctx);  // this is not part of the body!
        do_rebind_nodes(dirty, bound.code_bodies, ctx_body);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_do_while_statement: {
        const auto& altr = this->m_stor.as<index_do_while_statement>();

        // Rebind the body and the condition.
        Analytic_Context ctx_body(::rocket::ref(ctx), nullptr);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx_body);
        do_rebind_nodes(dirty, bound.code_cond, ctx);  // this is not part of the body!
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_while_statement: {
        const auto& altr = this->m_stor.as<index_while_statement>();

        // Rebind the condition and the body.
        Analytic_Context ctx_body(::rocket::ref(ctx), nullptr);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_cond, ctx);  // this is not part of the body!
        do_rebind_nodes(dirty, bound.code_body, ctx_body);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_for_each_statement: {
        const auto& altr = this->m_stor.as<index_for_each_statement>();

        // Rebind the range initializer and the body.
        Analytic_Context ctx_for(::rocket::ref(ctx), nullptr);
        Analytic_Context ctx_body(::rocket::ref(ctx_for), nullptr);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_init, ctx_for);
        do_rebind_nodes(dirty, bound.code_body, ctx_body);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_for_statement: {
        const auto& altr = this->m_stor.as<index_for_statement>();

        // Rebind the initializer, the condition, the loop increment and the body.
        Analytic_Context ctx_for(::rocket::ref(ctx), nullptr);
        Analytic_Context ctx_body(::rocket::ref(ctx_for), nullptr);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_init, ctx_for);
        do_rebind_nodes(dirty, bound.code_cond, ctx_for);
        do_rebind_nodes(dirty, bound.code_step, ctx_for);
        do_rebind_nodes(dirty, bound.code_body, ctx_body);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_try_statement: {
        const auto& altr = this->m_stor.as<index_try_statement>();

        // Rebind the `try` and `catch` clauses.
        Analytic_Context ctx_body(::rocket::ref(ctx), nullptr);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_try, ctx_body);
        do_rebind_nodes(dirty, bound.code_catch, ctx_body);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_throw_statement:
      case index_assert_statement:
      case index_simple_status:
      case index_glvalue_to_prvalue:
      case index_push_immediate:
      case index_push_global_reference:
        // There is nothing to rebind.
        return nullopt;

      case index_push_local_reference: {
        const auto& altr = this->m_stor.as<index_push_local_reference>();

        // Get the context.
        // Don't bind references in analytic contexts.
        const Abstract_Context* qctx = ::std::addressof(ctx);
        ::rocket::ranged_for(uint32_t(0), altr.depth, [&](uint32_t) { qctx = qctx->get_parent_opt();  });
        ROCKET_ASSERT(qctx);
        if(qctx->is_analytic())
          return nullopt;

        // Look for the name in the context.
        auto qref = qctx->get_named_reference_opt(altr.name);
        if(!qref)
          return nullopt;

        // Bind it now.
        S_push_bound_reference xnode = { *qref };
        return ::std::move(xnode);
      }

      case index_push_bound_reference:
        // There is nothing to rebind.
        return nullopt;

      case index_define_function: {
        const auto& altr = this->m_stor.as<index_define_function>();

        // Rebind the function body.
        Analytic_Context ctx_func(::std::addressof(ctx), altr.params);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx_func);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_branch_expression: {
        const auto& altr = this->m_stor.as<index_branch_expression>();

        // Rebind the expression.
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_true, ctx);
        do_rebind_nodes(dirty, bound.code_false, ctx);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();

        // Rebind the expression.
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_null, ctx);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_function_call:
      case index_member_access:
      case index_push_unnamed_array:
      case index_push_unnamed_object:
      case index_apply_operator:
      case index_unpack_struct_array:
      case index_unpack_struct_object:
      case index_define_null_variable:
      case index_single_step_trap:
      case index_variadic_call:
        // There is nothing to rebind.
        return nullopt;

      case index_defer_expression: {
        const auto& altr = this->m_stor.as<index_defer_expression>();

        // Rebind the expression.
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx);
        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_import_call:
        // There is nothing to rebind.
        return nullopt;

      default:
        ASTERIA_TERMINATE("invalid AIR node type (index `$1`)", this->index());
    }
  }

AVMC_Queue&
AIR_Node::
solidify(AVMC_Queue& queue, uint8_t ipass)
const
  {
    switch(this->index()) {
      case index_clear_stack: {
        const auto& altr = this->m_stor.as<index_clear_stack>();

        // There are no symbols.
        AVMC_Appender<void> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        (void)altr;
        return avmcp.output<do_clear_stack>(queue);
      }

      case index_execute_block: {
        const auto& altr = this->m_stor.as<index_execute_block>();

        // There are no symbols.
        AVMC_Appender<Pv_queues_fixed<1>> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.queues[0].reload(altr.code_body);
        return avmcp.output<do_execute_block>(queue);
      }

      case index_declare_variable: {
        const auto& altr = this->m_stor.as<index_declare_variable>();

        // Set up symbols.
        AVMC_Appender<Pv_sloc_name> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.name = altr.name;
        return avmcp.output<do_declare_variable>(queue);
      }

      case index_initialize_variable: {
        const auto& altr = this->m_stor.as<index_initialize_variable>();

        // Set up symbols.
        AVMC_Appender<void> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.pu.u8s[0] = altr.immutable;
        return avmcp.output<do_initialize_variable>(queue);
      }

      case index_if_statement: {
        const auto& altr = this->m_stor.as<index_if_statement>();

        // There are no symbols.
        AVMC_Appender<Pv_queues_fixed<2>> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.pu.u8s[0] = altr.negative;
        avmcp.queues[0].reload(altr.code_true);
        avmcp.queues[1].reload(altr.code_false);
        return avmcp.output<do_if_statement>(queue);
      }

      case index_switch_statement: {
        const auto& altr = this->m_stor.as<index_switch_statement>();

        // There are no symbols.
        AVMC_Appender<Pv_switch> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        for(size_t i = 0;  i < altr.code_bodies.size();  ++i) {
          avmcp.queues_labels.emplace_back().reload(altr.code_labels.at(i));
          avmcp.queues_bodies.emplace_back().reload(altr.code_bodies.at(i));
        }
        avmcp.names_added = altr.names_added;
        return avmcp.output<do_switch_statement>(queue);
      }

      case index_do_while_statement: {
        const auto& altr = this->m_stor.as<index_do_while_statement>();

        // There are no symbols.
        AVMC_Appender<Pv_queues_fixed<2>> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.queues[0].reload(altr.code_body);
        avmcp.pu.u8s[0] = altr.negative;
        avmcp.queues[1].reload(altr.code_cond);
        return avmcp.output<do_do_while_statement>(queue);
      }

      case index_while_statement: {
        const auto& altr = this->m_stor.as<index_while_statement>();

        // There are no symbols.
        AVMC_Appender<Pv_queues_fixed<2>> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.pu.u8s[0] = altr.negative;
        avmcp.queues[0].reload(altr.code_cond);
        avmcp.queues[1].reload(altr.code_body);
        return avmcp.output<do_while_statement>(queue);
      }

      case index_for_each_statement: {
        const auto& altr = this->m_stor.as<index_for_each_statement>();

        // There are no symbols.
        AVMC_Appender<Pv_for_each> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.name_key = altr.name_key;
        avmcp.name_mapped = altr.name_mapped;
        avmcp.queue_init.reload(altr.code_init);
        avmcp.queue_body.reload(altr.code_body);
        return avmcp.output<do_for_each_statement>(queue);
      }

      case index_for_statement: {
        const auto& altr = this->m_stor.as<index_for_statement>();

        // There are no symbols.
        AVMC_Appender<Pv_queues_fixed<4>> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.queues[0].reload(altr.code_init);
        avmcp.queues[1].reload(altr.code_cond);
        avmcp.queues[2].reload(altr.code_step);
        avmcp.queues[3].reload(altr.code_body);
        return avmcp.output<do_for_statement>(queue);
      }

      case index_try_statement: {
        const auto& altr = this->m_stor.as<index_try_statement>();

        // There are no symbols.
        AVMC_Appender<Pv_try> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.queue_try.reload(altr.code_try);
        avmcp.sloc_catch = altr.sloc_catch;
        avmcp.name_except = altr.name_except;
        avmcp.queue_catch.reload(altr.code_catch);
        return avmcp.output<do_try_statement>(queue);
      }

      case index_throw_statement: {
        const auto& altr = this->m_stor.as<index_throw_statement>();

        // The `throw` statement add its own frame so prevent a duplicate.
        AVMC_Appender<Pv_sloc> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.sloc = altr.sloc;
        return avmcp.output<do_throw_statement>(queue);
      }

      case index_assert_statement: {
        const auto& altr = this->m_stor.as<index_assert_statement>();

        // Set up symbols.
        AVMC_Appender<Pv_sloc_msg> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.pu.u8s[0] = altr.negative;
        avmcp.msg = altr.msg;
        return avmcp.output<do_assert_statement>(queue);
      }

      case index_simple_status: {
        const auto& altr = this->m_stor.as<index_simple_status>();

        // There are no symbols.
        AVMC_Appender<void> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.pu.u8s[0] = altr.status;
        return avmcp.output<do_simple_status>(queue);
      }

      case index_glvalue_to_prvalue: {
        const auto& altr = this->m_stor.as<index_glvalue_to_prvalue>();

        // Set up symbols.
        AVMC_Appender<void> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        (void)altr;
        return avmcp.output<do_glvalue_to_prvalue>(queue);
      }

      case index_push_immediate: {
        const auto& altr = this->m_stor.as<index_push_immediate>();

        // There are no symbols.
        AVMC_Appender<Value> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        static_cast<Value&>(avmcp) = altr.value;
        return avmcp.output<do_push_immediate>(queue);
      }

      case index_push_global_reference: {
        const auto& altr = this->m_stor.as<index_push_global_reference>();

        // Set up symbols.
        AVMC_Appender<Pv_name> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.name = altr.name;
        return avmcp.output<do_push_global_reference>(queue);
      }

      case index_push_local_reference: {
        const auto& altr = this->m_stor.as<index_push_local_reference>();

        // Set up symbols.
        AVMC_Appender<Pv_name> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.pu.x32 = altr.depth;
        avmcp.name = altr.name;
        return avmcp.output<do_push_local_reference>(queue);
      }

      case index_push_bound_reference: {
        const auto& altr = this->m_stor.as<index_push_bound_reference>();

        // There are no symbols.
        AVMC_Appender<Pv_ref> avmcp;
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.ref = altr.ref;
        return avmcp.output<do_push_bound_reference>(queue);
      }

      case index_define_function: {
        const auto& altr = this->m_stor.as<index_define_function>();

        // Set up symbols.
        AVMC_Appender<Pv_func> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.opts = altr.opts;
        avmcp.sloc = altr.sloc;
        avmcp.func = altr.func;
        avmcp.params = altr.params;
        avmcp.code_body = altr.code_body;
        return avmcp.output<do_define_function>(queue);
      }

      case index_branch_expression: {
        const auto& altr = this->m_stor.as<index_branch_expression>();

        // Set up symbols.
        AVMC_Appender<Pv_queues_fixed<2>> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.queues[0].reload(altr.code_true);
        avmcp.queues[1].reload(altr.code_false);
        avmcp.pu.u8s[0] = altr.assign;
        return avmcp.output<do_branch_expression>(queue);
      }

      case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();

        // Set up symbols.
        AVMC_Appender<Pv_queues_fixed<1>> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.queues[0].reload(altr.code_null);
        avmcp.pu.u8s[0] = altr.assign;
        return avmcp.output<do_coalescence>(queue);
      }

      case index_function_call: {
        const auto& altr = this->m_stor.as<index_function_call>();

        // Set up symbols.
        AVMC_Appender<Pv_sloc> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.pu.y32 = altr.nargs;
        avmcp.pu.y8s[0] = altr.ptc;
        return avmcp.output<do_function_call>(queue);
      }

      case index_member_access: {
        const auto& altr = this->m_stor.as<index_member_access>();

        // Set up symbols.
        AVMC_Appender<Pv_name> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.name = altr.name;
        return avmcp.output<do_member_access>(queue);
      }

      case index_push_unnamed_array: {
        const auto& altr = this->m_stor.as<index_push_unnamed_array>();

        // Set up symbols.
        AVMC_Appender<void> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.pu.x32 = altr.nelems;
        return avmcp.output<do_push_unnamed_array>(queue);
      }

      case index_push_unnamed_object: {
        const auto& altr = this->m_stor.as<index_push_unnamed_object>();

        // Set up symbols.
        AVMC_Appender<Pv_names> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.names = altr.keys;
        return avmcp.output<do_push_unnamed_object>(queue);
      }

      case index_apply_operator: {
        const auto& altr = this->m_stor.as<index_apply_operator>();

        // Set up symbols.
        AVMC_Appender<void> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        switch(weaken_enum(altr.xop)) {
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
        switch(altr.xop) {
          case xop_inc_post:
            return avmcp.output<do_apply_xop_INC_POST>(queue);

          case xop_dec_post:
            return avmcp.output<do_apply_xop_DEC_POST>(queue);

          case xop_subscr:
            return avmcp.output<do_apply_xop_SUBSCR>(queue);

          case xop_pos:
            return avmcp.output<do_apply_xop_POS>(queue);

          case xop_neg:
            return avmcp.output<do_apply_xop_NEG>(queue);

          case xop_notb:
            return avmcp.output<do_apply_xop_NOTB>(queue);

          case xop_notl:
            return avmcp.output<do_apply_xop_NOTL>(queue);

          case xop_inc_pre:
            return avmcp.output<do_apply_xop_INC_PRE>(queue);

          case xop_dec_pre:
            return avmcp.output<do_apply_xop_DEC_PRE>(queue);

          case xop_unset:
            return avmcp.output<do_apply_xop_UNSET>(queue);

          case xop_countof:
            return avmcp.output<do_apply_xop_LENGTHOF>(queue);

          case xop_typeof:
            return avmcp.output<do_apply_xop_TYPEOF>(queue);

          case xop_sqrt:
            return avmcp.output<do_apply_xop_SQRT>(queue);

          case xop_isnan:
            return avmcp.output<do_apply_xop_ISNAN>(queue);

          case xop_isinf:
            return avmcp.output<do_apply_xop_ISINF>(queue);

          case xop_abs:
            return avmcp.output<do_apply_xop_ABS>(queue);

          case xop_sign:
            return avmcp.output<do_apply_xop_SIGN>(queue);

          case xop_round:
            return avmcp.output<do_apply_xop_ROUND>(queue);

          case xop_floor:
            return avmcp.output<do_apply_xop_FLOOR>(queue);

          case xop_ceil:
            return avmcp.output<do_apply_xop_CEIL>(queue);

          case xop_trunc:
            return avmcp.output<do_apply_xop_TRUNC>(queue);

          case xop_iround:
            return avmcp.output<do_apply_xop_IROUND>(queue);

          case xop_ifloor:
            return avmcp.output<do_apply_xop_IFLOOR>(queue);

          case xop_iceil:
            return avmcp.output<do_apply_xop_ICEIL>(queue);

          case xop_itrunc:
            return avmcp.output<do_apply_xop_ITRUNC>(queue);

          case xop_cmp_eq:
            return avmcp.output<do_apply_xop_CMP_XEQ>(queue);

          case xop_cmp_ne:
            return avmcp.output<do_apply_xop_CMP_XEQ>(queue);

          case xop_cmp_lt:
            return avmcp.output<do_apply_xop_CMP_XREL>(queue);

          case xop_cmp_gt:
            return avmcp.output<do_apply_xop_CMP_XREL>(queue);

          case xop_cmp_lte:
            return avmcp.output<do_apply_xop_CMP_XREL>(queue);

          case xop_cmp_gte:
            return avmcp.output<do_apply_xop_CMP_XREL>(queue);

          case xop_cmp_3way:
            return avmcp.output<do_apply_xop_CMP_3WAY>(queue);

          case xop_add:
            return avmcp.output<do_apply_xop_ADD>(queue);

          case xop_sub:
            return avmcp.output<do_apply_xop_SUB>(queue);

          case xop_mul:
            return avmcp.output<do_apply_xop_MUL>(queue);

          case xop_div:
            return avmcp.output<do_apply_xop_DIV>(queue);

          case xop_mod:
            return avmcp.output<do_apply_xop_MOD>(queue);

          case xop_sll:
            return avmcp.output<do_apply_xop_SLL>(queue);

          case xop_srl:
            return avmcp.output<do_apply_xop_SRL>(queue);

          case xop_sla:
            return avmcp.output<do_apply_xop_SLA>(queue);

          case xop_sra:
            return avmcp.output<do_apply_xop_SRA>(queue);

          case xop_andb:
            return avmcp.output<do_apply_xop_ANDB>(queue);

          case xop_orb:
            return avmcp.output<do_apply_xop_ORB>(queue);

          case xop_xorb:
            return avmcp.output<do_apply_xop_XORB>(queue);

          case xop_assign:
            return avmcp.output<do_apply_xop_ASSIGN>(queue);

          case xop_fma:
            return avmcp.output<do_apply_xop_FMA>(queue);

          case xop_head:
            return avmcp.output<do_apply_xop_HEAD>(queue);

          case xop_tail:
            return avmcp.output<do_apply_xop_TAIL>(queue);

          default:
            ASTERIA_TERMINATE("invalid operator type (xop `$1`)", altr.xop);
        }
      }

      case index_unpack_struct_array: {
        const auto& altr = this->m_stor.as<index_unpack_struct_array>();

        // Set up symbols.
        AVMC_Appender<void> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.pu.y8s[0] = altr.immutable;
        avmcp.pu.y32 = altr.nelems;
        return avmcp.output<do_unpack_struct_array>(queue);
      }

      case index_unpack_struct_object: {
        const auto& altr = this->m_stor.as<index_unpack_struct_object>();

        // Set up symbols.
        AVMC_Appender<Pv_names> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.pu.u8s[0] = altr.immutable;
        avmcp.names = altr.keys;
        return avmcp.output<do_unpack_struct_object>(queue);
      }

      case index_define_null_variable: {
        const auto& altr = this->m_stor.as<index_define_null_variable>();

        // Set up symbols.
        AVMC_Appender<Pv_sloc_name> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.pu.u8s[0] = altr.immutable;
        avmcp.sloc = altr.sloc;
        avmcp.name = altr.name;
        return avmcp.output<do_define_null_variable>(queue);
      }

      case index_single_step_trap: {
        const auto& altr = this->m_stor.as<index_single_step_trap>();

        // Set up symbols.
        AVMC_Appender<Pv_sloc> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.sloc = altr.sloc;
        return avmcp.output<do_single_step_trap>(queue);
      }

      case index_variadic_call: {
        const auto& altr = this->m_stor.as<index_variadic_call>();

        // Set up symbols.
        AVMC_Appender<Pv_sloc> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.pu.u8s[0] = altr.ptc;
        return avmcp.output<do_variadic_call>(queue);
      }

      case index_defer_expression: {
        const auto& altr = this->m_stor.as<index_defer_expression>();

        // Set up symbols.
        AVMC_Appender<Pv_defer> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.code_body = altr.code_body;
        return avmcp.output<do_defer_expression>(queue);
      }

      case index_import_call: {
        const auto& altr = this->m_stor.as<index_import_call>();

        // Set up symbols.
        AVMC_Appender<Pv_opts_sloc> avmcp;
        avmcp.set_symbols(altr.sloc);
        if(ipass == 0)
          return avmcp.request(queue);

        // Encode arguments.
        avmcp.opts = altr.opts;
        avmcp.sloc = altr.sloc;
        avmcp.pu.y32 = altr.nargs;
        return avmcp.output<do_import_call>(queue);
      }

      default:
        ASTERIA_TERMINATE("invalid AIR node type (index `$1`)", this->index());
    }
  }

Variable_Callback&
AIR_Node::
enumerate_variables(Variable_Callback& callback)
const
  {
    switch(this->index()) {
      case index_clear_stack:
        return callback;

      case index_execute_block: {
        const auto& altr = this->m_stor.as<index_execute_block>();
        ::rocket::for_each(altr.code_body, callback);
        return callback;
      }

      case index_declare_variable:
      case index_initialize_variable:
        return callback;

      case index_if_statement: {
        const auto& altr = this->m_stor.as<index_if_statement>();
        ::rocket::for_each(altr.code_true, callback);
        ::rocket::for_each(altr.code_false, callback);
        return callback;
      }

      case index_switch_statement: {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        for(size_t i = 0;  i < altr.code_labels.size();  ++i) {
          ::rocket::for_each(altr.code_labels.at(i), callback);
          ::rocket::for_each(altr.code_bodies.at(i), callback);
        }
        return callback;
      }

      case index_do_while_statement: {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        ::rocket::for_each(altr.code_body, callback);
        ::rocket::for_each(altr.code_cond, callback);
        return callback;
      }

      case index_while_statement: {
        const auto& altr = this->m_stor.as<index_while_statement>();
        ::rocket::for_each(altr.code_cond, callback);
        ::rocket::for_each(altr.code_body, callback);
        return callback;
      }

      case index_for_each_statement: {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        ::rocket::for_each(altr.code_init, callback);
        ::rocket::for_each(altr.code_body, callback);
        return callback;
      }

      case index_for_statement: {
        const auto& altr = this->m_stor.as<index_for_statement>();
        ::rocket::for_each(altr.code_init, callback);
        ::rocket::for_each(altr.code_cond, callback);
        ::rocket::for_each(altr.code_step, callback);
        ::rocket::for_each(altr.code_body, callback);
        return callback;
      }

      case index_try_statement: {
        const auto& altr = this->m_stor.as<index_try_statement>();
        ::rocket::for_each(altr.code_try, callback);
        ::rocket::for_each(altr.code_catch, callback);
        return callback;
      }

      case index_throw_statement:
      case index_assert_statement:
      case index_simple_status:
      case index_glvalue_to_prvalue:
        return callback;

      case index_push_immediate: {
        const auto& altr = this->m_stor.as<index_push_immediate>();
        altr.value.enumerate_variables(callback);
        return callback;
      }

      case index_push_global_reference:
      case index_push_local_reference:
        return callback;

      case index_push_bound_reference: {
        const auto& altr = this->m_stor.as<index_push_bound_reference>();
        altr.ref.enumerate_variables(callback);
        return callback;
      }

      case index_define_function: {
        const auto& altr = this->m_stor.as<index_define_function>();
        ::rocket::for_each(altr.code_body, callback);
        return callback;
      }

      case index_branch_expression: {
        const auto& altr = this->m_stor.as<index_branch_expression>();
        ::rocket::for_each(altr.code_true, callback);
        ::rocket::for_each(altr.code_false, callback);
        return callback;
      }

      case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();
        ::rocket::for_each(altr.code_null, callback);
        return callback;
      }

      case index_function_call:
      case index_member_access:
      case index_push_unnamed_array:
      case index_push_unnamed_object:
      case index_apply_operator:
      case index_unpack_struct_array:
      case index_unpack_struct_object:
      case index_define_null_variable:
      case index_single_step_trap:
      case index_variadic_call:
        return callback;

      case index_defer_expression: {
        const auto& altr = this->m_stor.as<index_defer_expression>();
        ::rocket::for_each(altr.code_body, callback);
        return callback;
      }

      case index_import_call:
        return callback;


      default:
        ASTERIA_TERMINATE("invalid AIR node type (index `$1`)", this->index());
    }
  }

}  // namespace Asteria
