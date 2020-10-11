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
#include "../compiler/statement.hpp"
#include "../compiler/expression_unit.hpp"
#include "../llds/avmc_queue.hpp"
#include "../util.hpp"

namespace asteria {
namespace {

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
do_rebind_nodes(bool& dirty, cow_vector<cow_vector<AIR_Node>>& seqs, const Abstract_Context& ctx)
  {
    for(size_t k = 0;  k < seqs.size();  ++k) {
      for(size_t i = 0;  i < seqs[k].size();  ++i) {
        auto qnode = seqs[k][i].rebind_opt(ctx);
        if(!qnode)
          continue;

        dirty |= true;
        seqs.mut(k).mut(i) = ::std::move(*qnode);
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

bool
do_solidify_nodes(AVMC_Queue& queue, const cow_vector<AIR_Node>& code)
  {
    bool r = ::rocket::all_of(code, [&](const AIR_Node& node) { return node.solidify(queue);  });
    queue.shrink_to_fit();
    return r;
  }

Executive_Context&
do_set_temporary(Executive_Context& ctx, bool assign, Reference::S_temporary&& xref)
  {
    ROCKET_ASSERT(!ctx.stack().empty());

    if(assign) {
      // Write the value to the top refernce.
      ctx.stack().get_top().open() = ::std::move(xref.val);
      return ctx;
    }

    // Replace the top reference with a temporary reference to the value.
    ctx.stack().open_top() = ::std::move(xref);
    return ctx;
  }

AIR_Status
do_evaluate_subexpression(Executive_Context& ctx, bool assign, const AVMC_Queue& queue)
  {
    if(queue.empty())
      // Leave the condition on the top of the stack.
      return air_status_next;

    if(assign) {
      // Evaluate the subexpression.
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
    // Evaluate the subexpression.
    // You must forward the status code as is, because PTCs may return `air_status_return_ref`.
    return queue.execute(ctx);
  }

Reference&
do_declare(Executive_Context& ctx, const phsh_string& name)
  {
    return ctx.open_named_reference(name) = Reference::S_uninit();
  }

AIR_Status
do_execute_block(const AVMC_Queue& queue, const Executive_Context& ctx)
  {
    // Execute the body on a new context.
    Executive_Context ctx_next(::rocket::ref(ctx));
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

// These are user-defined parameter types for AVMC nodes.
// The `enumerate_variables()` callback is optional.

struct Sparam_sloc_text
  {
    Source_Location sloc;
    cow_string text;
  };

struct Sparam_sloc_name
  {
    Source_Location sloc;
    phsh_string name;
  };

struct Sparam_name
  {
    phsh_string name;
  };

struct Sparam_import
  {
    Compiler_Options opts;
    Source_Location sloc;
  };

template<size_t sizeT>
struct Sparam_queues
  {
    array<AVMC_Queue, sizeT> queues;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const
      {
        ::rocket::for_each(this->queues, callback);
        return callback;
      }
  };

using Sparam_queues_2 = Sparam_queues<2>;
using Sparam_queues_3 = Sparam_queues<3>;
using Sparam_queues_4 = Sparam_queues<4>;

struct Sparam_switch
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

struct Sparam_for_each
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

struct Sparam_try_catch
  {
    Source_Location sloc_try;
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

struct Sparam_func
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

struct Sparam_defer
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

// These are traits for individual AIR node types.
// Each traits struct must contain the `execute()` function, and optionally,
// these functions: `make_uparam()`, `make_sparam()`, `make_symbols()`.

struct AIR_Traits_clear_stack
  {
    // `Uparam` is unused.
    // `Sparam` is unused.

    static
    AIR_Status
    execute(Executive_Context& ctx)
      {
        ctx.stack().clear();
        return air_status_next;
      }
  };

struct AIR_Traits_execute_block
  {
    // `Uparam` is unused.
    // `Sparam` is the solidified body.

    static
    AVMC_Queue
    make_sparam(bool& reachable, const AIR_Node::S_execute_block& altr)
      {
        AVMC_Queue queue;
        reachable &= do_solidify_nodes(queue, altr.code_body);
        return queue;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue& queue)
      {
        return do_execute_block(queue, ctx);
      }
  };

struct AIR_Traits_declare_variable
  {
    // `Uparam` is unused.
    // `Sparam` is the source location and name;

    static
    Sparam_sloc_name
    make_sparam(bool& /*reachable*/, const AIR_Node::S_declare_variable& altr)
      {
        Sparam_sloc_name sp;
        sp.sloc = altr.sloc;
        sp.name = altr.name;
        return sp;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_declare_variable& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_sloc_name& sp)
      {
        // Allocate an uninitialized variable.
        auto gcoll = ctx.global().genius_collector();
        auto var = gcoll->create_variable();

        // Inject the variable into the current context.
        Reference::S_variable xref = { ::std::move(var) };
        ctx.open_named_reference(sp.name) = xref;  // it'll be used later so don't move!

        // Call the hook function if any.
        if(auto qhooks = ctx.global().get_hooks_opt())
          qhooks->on_variable_declare(sp.sloc, sp.name);

        // Push a copy of the reference onto the stack.
        ctx.stack().push(::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_initialize_variable
  {
    // `Uparam` is `immutable`.
    // `Sparam` is unused.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_initialize_variable& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = altr.immutable;
        return up;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_initialize_variable& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this function.
        auto val = ctx.stack().get_top().read();
        ctx.stack().pop();

        // Get the variable back.
        auto var = ctx.stack().get_top().get_variable_opt();
        ROCKET_ASSERT(var && !var->is_initialized());
        ctx.stack().pop();

        // Initialize it.
        var->initialize(::std::move(val), up.v8s[0]);
        return air_status_next;
      }
  };

struct AIR_Traits_if_statement
  {
    // `Uparam` is `negative`.
    // `Sparam` is the two branches.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_if_statement& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = altr.negative;
        return up;
      }

    static
    Sparam_queues_2
    make_sparam(bool& reachable, const AIR_Node::S_if_statement& altr)
      {
        Sparam_queues_2 sp;
        bool rtrue = do_solidify_nodes(sp.queues[0], altr.code_true);
        bool rfalse = do_solidify_nodes(sp.queues[1], altr.code_false);
        reachable &= rtrue | rfalse;
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Sparam_queues_2& sp)
      {
        // Check the value of the condition.
        if(ctx.stack().get_top().read().test() != up.v8s[0])
          // Execute the true branch and forward the status verbatim.
          return do_execute_block(sp.queues[0], ctx);

        // Execute the false branch and forward the status verbatim.
        return do_execute_block(sp.queues[1], ctx);
      }
  };

struct AIR_Traits_switch_statement
  {
    // `Uparam` is unused.
    // `Sparam` is ... everything.

    static
    void
    do_xsolidify_code(cow_vector<AVMC_Queue>& queues, const cow_vector<cow_vector<AIR_Node>>& seqs)
      {
        ROCKET_ASSERT(queues.empty());
        queues.reserve(seqs.size());
        ::rocket::for_each(seqs, [&](const auto& code) { do_solidify_nodes(queues.emplace_back(), code);  });
      }

    static
    Sparam_switch
    make_sparam(bool& /*reachable*/, const AIR_Node::S_switch_statement& altr)
      {
        Sparam_switch sp;
        do_xsolidify_code(sp.queues_labels, altr.code_labels);
        do_xsolidify_code(sp.queues_bodies, altr.code_bodies);
        sp.names_added = altr.names_added;
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_switch& sp)
      {
        // Get the number of clauses.
        auto nclauses = sp.queues_labels.size();
        ROCKET_ASSERT(nclauses == sp.queues_bodies.size());
        ROCKET_ASSERT(nclauses == sp.names_added.size());

        // Read the value of the condition.
        auto cond = ctx.stack().get_top().read();

        // Find a target clause.
        size_t bp = SIZE_MAX;

        // This is different from the `switch` statement in C, where `case` labels must have constant operands.
        for(size_t i = 0;  i < nclauses;  ++i) {
          // This is a `default` clause if the condition is empty, and a `case` clause otherwise.
          if(sp.queues_labels[i].empty()) {
            if(bp != SIZE_MAX)
              ASTERIA_THROW("Multiple `default` clauses");

            bp = i;
            continue;
          }
          // Evaluate the operand and check whether it equals `cond`.
          auto status = sp.queues_labels[i].execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
          if(ctx.stack().get_top().read().compare(cond) == compare_equal) {
            bp = i;
            break;
          }
        }

        // Skip this statement if no matching clause has been found.
        if(bp != SIZE_MAX) {
          // Note that all clauses share the same context.
          Executive_Context ctx_body(::rocket::ref(ctx));
          AIR_Status status;

          // Inject all bypassed variables into the scope.
          for(const auto& name : sp.names_added[bp])
            do_declare(ctx_body, name);

          ASTERIA_RUNTIME_TRY {
            do {
              // Execute the body.
              status = sp.queues_bodies[bp].execute(ctx_body);
              if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_switch }))
                break;

              if(status != air_status_next)
                return status;
            }
            while(++bp < nclauses);
          }
          ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
            ctx_body.on_scope_exit(except);
            throw;
          }
          ctx_body.on_scope_exit(status);
        }
        return air_status_next;
      }
  };

struct AIR_Traits_do_while_statement
  {
    // `Uparam` is `negative`.
    // `Sparam` is the loop body and condition.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_do_while_statement& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = altr.negative;
        return up;
      }

    static
    Sparam_queues_2
    make_sparam(bool& /*reachable*/, const AIR_Node::S_do_while_statement& altr)
      {
        Sparam_queues_2 sp;
        do_solidify_nodes(sp.queues[0], altr.code_body);
        do_solidify_nodes(sp.queues[1], altr.code_cond);
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Sparam_queues_2& sp)
      {
        // This is the same as the `do...while` statement in C.
        for(;;) {
          // Execute the body.
          auto status = do_execute_block(sp.queues[0], ctx);
          if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while }))
            break;

          if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                            air_status_continue_while }))
            return status;

          // Check the condition.
          status = sp.queues[1].execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
          if(ctx.stack().get_top().read().test() == up.v8s[0])
            break;
        }
        return air_status_next;
      }
  };

struct AIR_Traits_while_statement
  {
    // `Uparam` is `negative`.
    // `Sparam` is the condition and loop body.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_while_statement& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = altr.negative;
        return up;
      }

    static
    Sparam_queues_2
    make_sparam(bool& /*reachable*/, const AIR_Node::S_while_statement& altr)
      {
        Sparam_queues_2 sp;
        do_solidify_nodes(sp.queues[0], altr.code_cond);
        do_solidify_nodes(sp.queues[1], altr.code_body);
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Sparam_queues_2& sp)
      {
        // This is the same as the `while` statement in C.
        for(;;) {
          // Check the condition.
          auto status = sp.queues[0].execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
          if(ctx.stack().get_top().read().test() == up.v8s[0])
            break;

          // Execute the body.
          status = do_execute_block(sp.queues[1], ctx);
          if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while }))
            break;

          if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                            air_status_continue_while }))
            return status;
        }
        return air_status_next;
      }
  };

struct AIR_Traits_for_each_statement
  {
    // `Uparam` is unused.
    // `Sparam` is ... everything.

    static
    Sparam_for_each
    make_sparam(bool& /*reachable*/, const AIR_Node::S_for_each_statement& altr)
      {
        Sparam_for_each sp;
        sp.name_key = altr.name_key;
        sp.name_mapped = altr.name_mapped;
        do_solidify_nodes(sp.queue_init, altr.code_init);
        do_solidify_nodes(sp.queue_body, altr.code_body);
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_for_each& sp)
      {
        // Get global interfaces.
        auto gcoll = ctx.global().genius_collector();

        // We have to create an outer context due to the fact that the key and mapped
        // references outlast every iteration.
        Executive_Context ctx_for(::rocket::ref(ctx));

        // Allocate an uninitialized variable for the key.
        const auto vkey = gcoll->create_variable();
        // Inject the variable into the current context.
        Reference::S_variable xref = { vkey };
        ctx_for.open_named_reference(sp.name_key) = xref;

        // Create the mapped reference.
        auto& mapped = do_declare(ctx_for, sp.name_mapped);
        // Evaluate the range initializer.
        auto status = sp.queue_init.execute(ctx_for);
        ROCKET_ASSERT(status == air_status_next);
        // Set the range up, which isn't going to change for the entire loop.
        mapped = ::std::move(ctx_for.stack().open_top());

        const auto range = mapped.read();
        switch(weaken_enum(range.type())) {
          case type_null:
            // Do nothing.
            return air_status_next;

          case type_array: {
            const auto& arr = range.as_array();
            for(int64_t i = 0;  i < arr.ssize();  ++i) {
              // Set the key which is the subscript of the mapped element in the array.
              vkey->initialize(i, true);
              // Set the mapped reference.
              Reference::M_array_index xmod = { i };
              mapped.zoom_in(::std::move(xmod));

              // Execute the loop body.
              status = do_execute_block(sp.queue_body, ctx_for);
              if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for }))
                break;

              if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                                air_status_continue_for }))
                return status;

              // Restore the mapped reference.
              mapped.zoom_out();
            }
            return air_status_next;
          }

          case type_object: {
            const auto& obj = range.as_object();
            for(auto it = obj.begin();  it != obj.end();  ++it) {
              // Set the key which is the key of this element in the object.
              vkey->initialize(it->first.rdstr(), true);
              // Set the mapped reference.
              Reference::M_object_key xmod = { it->first.rdstr() };
              mapped.zoom_in(::std::move(xmod));

              // Execute the loop body.
              status = do_execute_block(sp.queue_body, ctx_for);
              if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for }))
                break;

              if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                                air_status_continue_for }))
                return status;

              // Restore the mapped reference.
              mapped.zoom_out();
            }
            return air_status_next;
          }

          default:
            ASTERIA_THROW("Range value not iterable (range `$1`)", range);
        }
      }
  };

struct AIR_Traits_for_statement
  {
    // `Uparam` is unused.
    // `Sparam` is ... everything.

    static
    Sparam_queues_4
    make_sparam(bool& /*reachable*/, const AIR_Node::S_for_statement& altr)
      {
        Sparam_queues_4 sp;
        do_solidify_nodes(sp.queues[0], altr.code_init);
        do_solidify_nodes(sp.queues[1], altr.code_cond);
        do_solidify_nodes(sp.queues[2], altr.code_step);
        do_solidify_nodes(sp.queues[3], altr.code_body);
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_queues_4& sp)
      {
        // This is the same as the `for` statement in C.
        // We have to create an outer context due to the fact that names declared in the first segment
        // outlast every iteration.
        Executive_Context ctx_for(::rocket::ref(ctx));

        // Execute the loop initializer, which shall only be a definition or an expression statement.
        auto status = sp.queues[0].execute(ctx_for);
        ROCKET_ASSERT(status == air_status_next);
        for(;;) {
          // Check the condition.
          status = sp.queues[1].execute(ctx_for);
          ROCKET_ASSERT(status == air_status_next);
          // This is a special case: If the condition is empty then the loop is infinite.
          if(!ctx_for.stack().empty() && !ctx_for.stack().get_top().read().test())
            break;

          // Execute the body.
          status = do_execute_block(sp.queues[3], ctx_for);
          if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for }))
            break;

          if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                            air_status_continue_for }))
            return status;

          // Execute the increment.
          status = sp.queues[2].execute(ctx_for);
          ROCKET_ASSERT(status == air_status_next);
        }
        return air_status_next;
      }
  };

struct AIR_Traits_try_statement
  {
    // `Uparam` is unused.
    // `Sparam` is ... everything.

    static
    Sparam_try_catch
    make_sparam(bool& reachable, const AIR_Node::S_try_statement& altr)
      {
        Sparam_try_catch sp;
        sp.sloc_try = altr.sloc_try;
        bool rtry = do_solidify_nodes(sp.queue_try, altr.code_try);
        sp.sloc_catch = altr.sloc_catch;
        sp.name_except = altr.name_except;
        bool rcatch = do_solidify_nodes(sp.queue_catch, altr.code_catch);
        reachable &= rtry | rcatch;
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_try_catch& sp)
      ASTERIA_RUNTIME_TRY {
        // This is almost identical to JavaScript.
        // Execute the `try` block. If no exception is thrown, this will have little overhead.
        auto status = do_execute_block(sp.queue_try, ctx);
        if(status != air_status_return_ref)
          return status;

        // This must not be PTC'd, otherwise exceptions thrown from tail calls won't be caught.
        ctx.stack().open_top().finish_call(ctx.global());
        return status;
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
        // Append a frame due to exit of the `try` clause.
        // Reuse the exception object. Don't bother allocating a new one.
        except.push_frame_try(sp.sloc_try);

        // This branch must be executed inside this `catch` block.
        // User-provided bindings may obtain the current exception using `::std::current_exception`.
        Executive_Context ctx_catch(::rocket::ref(ctx));
        AIR_Status status;

        ASTERIA_RUNTIME_TRY {
          // Set the exception reference.
          Reference::S_temporary xref = { except.value() };
          ctx_catch.open_named_reference(sp.name_except) = ::std::move(xref);

          // Set backtrace frames.
          V_array backtrace;
          for(size_t i = 0;  i < except.count_frames();  ++i) {
            const auto& f = except.frame(i);

            // Translate each frame into a human-readable format.
            V_object r;
            r.try_emplace(::rocket::sref("frame"), ::rocket::sref(f.what_type()));
            r.try_emplace(::rocket::sref("file"), f.file());
            r.try_emplace(::rocket::sref("line"), f.line());
            r.try_emplace(::rocket::sref("offset"), f.offset());
            r.try_emplace(::rocket::sref("value"), f.value());

            // Append this frame.
            backtrace.emplace_back(::std::move(r));
          }
          xref.val = ::std::move(backtrace);
          ctx_catch.open_named_reference(::rocket::sref("__backtrace")) = ::std::move(xref);

          // Execute the `catch` clause.
          status = sp.queue_catch.execute(ctx_catch);
        }
        ASTERIA_RUNTIME_CATCH(Runtime_Error& nested) {
          ctx_catch.on_scope_exit(nested);
          nested.push_frame_catch(sp.sloc_catch, except.value());
          throw;
        }
        ctx_catch.on_scope_exit(status);
        return status;
      }
  };

struct AIR_Traits_throw_statement
  {
    // `Uparam` is unused.
    // `Sparam` is the source location.

    static
    Source_Location
    make_sparam(bool& /*reachable*/, const AIR_Node::S_throw_statement& altr)
      {
        return altr.sloc;
      }

    [[noreturn]] static
    AIR_Status
    execute(Executive_Context& ctx, const Source_Location& sloc)
      {
        // Read the value to throw.
        // Note that the operand must not have been empty for this code.
        throw Runtime_Error(Runtime_Error::M_throw(), ctx.stack().get_top().read(), sloc);
      }
  };

struct AIR_Traits_assert_statement
  {
    // `Uparam` is `negative`.
    // `Sparam` is the source location.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_assert_statement& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = altr.negative;
        return up;
      }

    static
    Sparam_sloc_text
    make_sparam(bool& /*reachable*/, const AIR_Node::S_assert_statement& altr)
      {
        Sparam_sloc_text sp;
        sp.sloc = altr.sloc;
        sp.text = altr.msg;
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Sparam_sloc_text& sp)
      {
        // Check the value of the condition.
        if(ROCKET_EXPECT(ctx.stack().get_top().read().test() != up.v8s[0]))
          // When the assertion succeeds, there is nothing to do.
          return air_status_next;

        // Throw a `Runtime_Error`.
        throw Runtime_Error(Runtime_Error::M_assert(), sp.sloc, sp.text);
      }
  };

struct AIR_Traits_return_statement
  {
    // `Uparam` is `status`.
    // `Sparam` is unused.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& reachable, const AIR_Node::S_return_statement& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = weaken_enum(altr.status);
        reachable = false;
        return up;
      }

    static
    AIR_Status
    execute(Executive_Context& /*ctx*/, const AVMC_Queue::Uparam& up)
      {
        auto status = static_cast<AIR_Status>(up.v8s[0]);
        ROCKET_ASSERT(::rocket::is_any_of(status, { air_status_return_void, air_status_return_ref }));
        return status;
      }
  };

struct AIR_Traits_glvalue_to_prvalue
  {
    // `Uparam` is unused.
    // `Sparam` is unused.

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_glvalue_to_prvalue& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx)
      {
        // Check for glvalues only.
        // Void references and PTC wrappers are forwarded as is.
        auto& self = ctx.stack().open_top();
        if(self.is_prvalue())
          return air_status_next;

        // Convert the result to an rvalue.
        Reference::S_temporary xref = { self.read() };
        self = ::std::move(xref);
        return air_status_next;
      }
  };

struct AIR_Traits_push_immediate
  {
    // `Uparam` is unused.
    // `Sparam` is the value to push.

    static
    Value
    make_sparam(bool& /*reachable*/, const AIR_Node::S_push_immediate& altr)
      {
        return altr.value;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Value& value)
      {
        // Push a constant.
        Reference::S_constant xref = { value };
        ctx.stack().push(::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_push_global_reference
  {
    // `Uparam` is unused.
    // `Sparam` is the source location and name;

    static
    Sparam_sloc_name
    make_sparam(bool& /*reachable*/, const AIR_Node::S_push_global_reference& altr)
      {
        Sparam_sloc_name sp;
        sp.sloc = altr.sloc;
        sp.name = altr.name;
        return sp;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_push_global_reference& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_sloc_name& sp)
      {
        // Look for the name in the global context.
        auto qref = ctx.global().get_named_reference_opt(sp.name);
        if(!qref)
          ASTERIA_THROW("Undeclared identifier `$1`", sp.name);

        // Push a copy of it.
        ctx.stack().push(*qref);
        return air_status_next;
      }
  };

struct AIR_Traits_push_local_reference
  {
    // `Uparam` is the depth.
    // `Sparam` is the source location and name;

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_push_local_reference& altr)
      {
        AVMC_Queue::Uparam up;
        up.x32 = altr.depth;
        return up;
      }

    static
    Sparam_sloc_name
    make_sparam(bool& /*reachable*/, const AIR_Node::S_push_local_reference& altr)
      {
        Sparam_sloc_name sp;
        sp.sloc = altr.sloc;
        sp.name = altr.name;
        return sp;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_push_local_reference& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Sparam_sloc_name& sp)
      {
        // Get the context.
        const Executive_Context* qctx = &ctx;
        ::rocket::ranged_for(UINT32_C(0), up.x32, [&](uint32_t) { qctx = qctx->get_parent_opt();  });
        ROCKET_ASSERT(qctx);

        // Look for the name in the context.
        auto qref = qctx->get_named_reference_opt(sp.name);
        if(!qref)
          ASTERIA_THROW("Undeclared identifier `$1`", sp.name);

        // Check if control flow has bypassed its initialization.
        if(qref->is_uninit())
          ASTERIA_THROW("Use of bypassed variable or reference `$1`", sp.name);

        // Push a copy of it.
        ctx.stack().push(*qref);
        return air_status_next;
      }
  };

struct AIR_Traits_push_bound_reference
  {
    // `Uparam` is unused.
    // `Sparam` is the reference to push.

    static
    Reference
    make_sparam(bool& /*reachable*/, const AIR_Node::S_push_bound_reference& altr)
      {
        return altr.ref;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Reference& ref)
      {
        // Push a copy of the bound reference.
        ctx.stack().push(ref);
        return air_status_next;
      }
  };

struct AIR_Traits_define_function
  {
    // `Uparam` is unused.
    // `Sparam` is ... everything.

    static
    Sparam_func
    make_sparam(bool& /*reachable*/, const AIR_Node::S_define_function& altr)
      {
        Sparam_func sp;
        sp.opts = altr.opts;
        sp.sloc = altr.sloc;
        sp.func = altr.func;
        sp.params = altr.params;
        sp.code_body = altr.code_body;
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_func& sp)
      {
        // Rewrite nodes in the body as necessary.
        AIR_Optimizer optmz(sp.opts);
        optmz.rebind(&ctx, sp.params, sp.code_body);
        auto qtarget = optmz.create_function(sp.sloc, sp.func);

        // Push the function as a temporary.
        Reference::S_temporary xref = { ::std::move(qtarget) };
        ctx.stack().push(::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_branch_expression
  {
    // `Uparam` is `assign`.
    // `Sparam` is the branches.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_branch_expression& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = altr.assign;
        return up;
      }

    static
    Sparam_queues_2
    make_sparam(bool& reachable, const AIR_Node::S_branch_expression& altr)
      {
        Sparam_queues_2 sp;
        bool rtrue = do_solidify_nodes(sp.queues[0], altr.code_true);
        bool rfalse = do_solidify_nodes(sp.queues[1], altr.code_false);
        reachable &= rtrue | rfalse;
        return sp;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_branch_expression& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Sparam_queues_2& sp)
      {
        // Check the value of the condition.
        if(ctx.stack().get_top().read().test())
          // Execute the true branch and forward the status verbatim.
          return do_evaluate_subexpression(ctx, up.v8s[0], sp.queues[0]);

        // Execute the false branch and forward the status verbatim.
        return do_evaluate_subexpression(ctx, up.v8s[0], sp.queues[1]);
      }
  };

struct AIR_Traits_coalescence
  {
    // `Uparam` is `assign`.
    // `Sparam` is the null branch.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_coalescence& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = altr.assign;
        return up;
      }

    static
    AVMC_Queue
    make_sparam(bool& /*reachable*/, const AIR_Node::S_coalescence& altr)
      {
        AVMC_Queue queue;
        do_solidify_nodes(queue, altr.code_null);
        return queue;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_coalescence& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const AVMC_Queue& queue)
      {
        // Check the value of the condition.
        if(!ctx.stack().get_top().read().is_null())
          // Leave the condition on the stack.
          return air_status_next;

        // Execute the null branch and forward the status verbatim.
        return do_evaluate_subexpression(ctx, up.v8s[0], queue);
      }
  };

ROCKET_NOINLINE
Reference&
do_invoke_nontail(Reference& self, const Source_Location& sloc, Executive_Context& ctx,
                  const cow_function& target, cow_vector<Reference>&& args)
  {
    // Note exceptions thrown here are not caught.
    if(auto qhooks = ctx.global().get_hooks_opt())
      qhooks->on_function_call(sloc, target);

    // Execute the target function
    ASTERIA_RUNTIME_TRY {
      target.invoke(self, ctx.global(), ::std::move(args));
    }
    ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
      if(auto qhooks = ctx.global().get_hooks_opt())
        qhooks->on_function_except(sloc, target, except);
      throw;
    }
    if(auto qhooks = ctx.global().get_hooks_opt())
      qhooks->on_function_return(sloc, target, self);
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
    auto ptca = ::rocket::make_refcnt<PTC_Arguments>(sloc, ptc, target, ::std::move(args));

    // Set the result, which will be unpacked outside this scope.
    Reference::S_ptc_args xref = { ::std::move(ptca) };
    self = ::std::move(xref);

    // Force `air_status_return_ref` if control flow reaches the end of a function.
    // Otherwise a null reference is returned instead of this PTC wrapper, which can then never be unpacked.
    return air_status_return_ref;
  }

cow_vector<Reference>
do_pop_positional_arguments(Executive_Context& ctx, size_t nargs)
  {
    cow_vector<Reference> args;
    args.append(nargs, Reference::S_uninit());
    for(size_t i = args.size() - 1;  i != SIZE_MAX;  --i) {
      // Get an argument. Ensure it is dereferenceable.
      auto& arg = ctx.stack().open_top();
      static_cast<void>(arg.read());

      // Set the argument as is.
      args.mut(i) = ::std::move(arg);
      ctx.stack().pop();
    }
    return args;
  }

struct AIR_Traits_function_call
  {
    // `Uparam` is `nargs` and `ptc`.
    // `Sparam` is the source location.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& reachable, const AIR_Node::S_function_call& altr)
      {
        AVMC_Queue::Uparam up;
        up.y32 = altr.nargs;
        up.y8s[0] = weaken_enum(altr.ptc);
        reachable &= (altr.ptc == ptc_aware_none);
        return up;
      }

    static
    Source_Location
    make_sparam(bool& /*reachable*/, const AIR_Node::S_function_call& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_function_call& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Source_Location& sloc)
      {
        // Check for stack overflows.
        const auto sentry = ctx.global().copy_recursion_sentry();

        // Generate a single-step trap before unpacking arguments.
        if(auto qhooks = ctx.global().get_hooks_opt())
          qhooks->on_single_step_trap(sloc);

        // Pop arguments off the stack backwards.
        auto args = do_pop_positional_arguments(ctx, up.y32);

        // Copy the target, which shall be of type `function`.
        auto value = ctx.stack().get_top().read();
        if(!value.is_function())
          ASTERIA_THROW("Attempt to call a non-function (value `$1`)", value);
        auto& self = ctx.stack().open_top().zoom_out();

        return do_function_call_common(self, sloc, ctx, value.as_function(),
                                       static_cast<PTC_Aware>(up.y8s[0]), ::std::move(args));
      }
  };

struct AIR_Traits_member_access
  {
    // `Uparam` is unused.
    // `Sparam` is the name.

    static
    phsh_string
    make_sparam(bool& /*reachable*/, const AIR_Node::S_member_access& altr)
      {
        return altr.name;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_member_access& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const phsh_string& name)
      {
        // Append a modifier to the reference at the top.
        Reference::M_object_key xmod = { name };
        ctx.stack().open_top().zoom_in(::std::move(xmod));
        return air_status_next;
      }
  };

struct AIR_Traits_push_unnamed_array
  {
    // `Uparam` is `nelems`.
    // `Sparam` is unused.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_push_unnamed_array& altr)
      {
        AVMC_Queue::Uparam up;
        up.x32 = altr.nelems;
        return up;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_push_unnamed_array& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // Pop elements from the stack and store them in an array backwards.
        V_array array;
        array.resize(up.x32);
        for(auto it = array.mut_rbegin();  it != array.rend();  ++it) {
          // Write elements backwards.
          *it = ctx.stack().get_top().read();
          ctx.stack().pop();
        }

        // Push the array as a temporary.
        Reference::S_temporary xref = { ::std::move(array) };
        ctx.stack().push(::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_push_unnamed_object
  {
    // `Uparam` is unused.
    // `Sparam` is the list of keys.

    static
    cow_vector<phsh_string>
    make_sparam(bool& /*reachable*/, const AIR_Node::S_push_unnamed_object& altr)
      {
        return altr.keys;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_push_unnamed_object& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const cow_vector<phsh_string>& keys)
      {
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
        Reference::S_temporary xref = { ::std::move(object) };
        ctx.stack().push(::std::move(xref));
        return air_status_next;
      }
  };

enum Vmask : uint32_t
  {
    vmask_null        = 1 << type_null,
    vmask_boolean     = 1 << type_boolean,
    vmask_integer     = 1 << type_integer,
    vmask_real        = 1 << type_real,
    vmask_string      = 1 << type_string,
    vmask_opaque      = 1 << type_opaque,
    vmask_function    = 1 << type_function,
    vmask_array       = 1 << type_array,
    vmask_object      = 1 << type_object,
  };

inline
uint32_t do_vmask_of(const Value& val)
noexcept
  {
    return UINT32_C(1) << val.type();
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_neg(int64_t rhs)
  {
    if(rhs == INT64_MIN)
      ASTERIA_THROW("Integer negation overflow (operand was `$1`)", rhs);
    return -rhs;
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_add(int64_t lhs, int64_t rhs)
  {
    if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs))
      ASTERIA_THROW("Integer addition overflow (operands were `$1` and `$2`)", lhs, rhs);
    return lhs + rhs;
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_sub(int64_t lhs, int64_t rhs)
  {
    if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs))
      ASTERIA_THROW("Integer subtraction overflow (operands were `$1` and `$2`)", lhs, rhs);
    return lhs - rhs;
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_mul(int64_t lhs, int64_t rhs)
  {
    if((lhs == 0) || (rhs == 0))
      return 0;

    if((lhs == 1) || (rhs == 1))
      return (lhs ^ rhs) ^ 1;

    if((lhs == INT64_MIN) || (rhs == INT64_MIN))
      ASTERIA_THROW("Integer multiplication overflow (operands were `$1` and `$2`)", lhs, rhs);

    if((lhs == -1) || (rhs == -1))
      return (lhs ^ rhs) + 1;

    // absolute lhs and signed rhs
    auto m = lhs >> 63;
    auto alhs = (lhs ^ m) - m;  // may only be positive
    auto srhs = (rhs ^ m) - m;
    if((srhs >= 0) ? (alhs > INT64_MAX / srhs) : (alhs > INT64_MIN / srhs))
      ASTERIA_THROW("Integer multiplication overflow (operands were `$1` and `$2`)", lhs, rhs);
    return alhs * srhs;
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_div(int64_t lhs, int64_t rhs)
  {
    if(rhs == 0)
      ASTERIA_THROW("Integer divided by zero (operands were `$1` and `$2`)", lhs, rhs);

    if((lhs == INT64_MIN) && (rhs == -1))
      ASTERIA_THROW("Integer division overflow (operands were `$1` and `$2`)", lhs, rhs);
    return lhs / rhs;
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_mod(int64_t lhs, int64_t rhs)
  {
    if(rhs == 0)
      ASTERIA_THROW("Integer divided by zero (operands were `$1` and `$2`)", lhs, rhs);

    if((lhs == INT64_MIN) && (rhs == -1))
      ASTERIA_THROW("Integer division overflow (operands were `$1` and `$2`)", lhs, rhs);
    return lhs % rhs;
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_sll(int64_t lhs, int64_t rhs)
  {
    if(rhs < 0)
      ASTERIA_THROW("Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

    if(rhs >= 64)
      return 0;
    return int64_t(uint64_t(lhs) << rhs);
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_srl(int64_t lhs, int64_t rhs)
  {
    if(rhs < 0)
      ASTERIA_THROW("Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

    if(rhs >= 64)
      return 0;
    return int64_t(uint64_t(lhs) >> rhs);
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_sla(int64_t lhs, int64_t rhs)
  {
    if(rhs < 0)
      ASTERIA_THROW("Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

    if(lhs == 0)
      return 0;

    if(rhs >= 64)
      ASTERIA_THROW("Integer left shift overflow (operands were `$1` and `$2`)", lhs, rhs);

    // Bits that will be shifted out must be all zeroes or all ones.
    auto bc = 63 - int(rhs);
    auto mask_out = uint64_t(lhs) >> bc << bc;
    auto mask_sbt = uint64_t(lhs >> 63) << bc;
    if(mask_out != mask_sbt)
      ASTERIA_THROW("Integer left shift overflow (operands were `$1` and `$2`)", lhs, rhs);
    return int64_t(uint64_t(lhs) << rhs);
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_sra(int64_t lhs, int64_t rhs)
  {
    if(rhs < 0)
      ASTERIA_THROW("Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

    if(rhs >= 64)
      return lhs >> 63;

    return lhs >> rhs;
  }

ROCKET_CONST_FUNCTION
int64_t
do_check_itrunc(double value)
  {
    if(!::std::islessequal(-0x1p63, value) || !::std::islessequal(value, 0x1p63 - 0x1p10))
      ASTERIA_THROW("Value not representable as an `integer` (operand was `$1`)", value);
    return int64_t(value);
  }

cow_string
do_string_dup(const cow_string& src, int64_t count)
  {
    if(count < 0)
      ASTERIA_THROW("Negative duplicate count (count was `$2`)", count);

    cow_string res;
    size_t nchars = src.size();
    if((nchars == 0) || (count == 0))
      return res;

    if(nchars > res.max_size() / static_cast<uint64_t>(count))
      ASTERIA_THROW("String length overflow (`$1` * `$2` > `$3`)", nchars, count, res.max_size());

    size_t times = static_cast<size_t>(count);
    if(nchars == 1) {
      // Fast fill.
      res.append(times, src.front());
    }
    else {
      // Reserve space for the result string.
      res.append(nchars * times, '*');
      char* ptr = res.mut_data();

      // Make the first copy of the src string.
      ::std::memcpy(ptr, src.data(), nchars);

      // Append the result string to itself, doubling its length, until more than half of
      // the result string has been populated.
      for(; nchars <= res.size() / 2; nchars *= 2)
        ::std::memcpy(ptr + nchars, ptr, nchars);

      // Copy remaining characters, if any.
      if(nchars < res.size())
        ::std::memcpy(ptr + nchars, ptr, res.size() - nchars);
    }
    return res;
  }

cow_string
do_string_sll(const cow_string& lhs, int64_t rhs)
  {
    if(rhs < 0)
      ASTERIA_THROW("Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

    cow_string res;
    res.append(lhs.size(), ' ');
    if(static_cast<uint64_t>(rhs) >= lhs.size())
      return res;

    // Copy the substring in the right.
    size_t count = static_cast<size_t>(rhs);
    ::std::memcpy(res.mut_data(), lhs.data() + count, lhs.size() - count);
    return res;
  }

cow_string
do_string_srl(const cow_string& lhs, int64_t rhs)
  {
    if(rhs < 0)
      ASTERIA_THROW("Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

    cow_string res;
    res.append(lhs.size(), ' ');
    if(static_cast<uint64_t>(rhs) >= lhs.size())
      return res;

    // Copy the substring in the left.
    size_t count = static_cast<size_t>(rhs);
    ::std::memcpy(res.mut_data() + count, lhs.data(), lhs.size() - count);
    return res;
  }

cow_string
do_string_sla(const cow_string& lhs, int64_t rhs)
  {
    if(rhs < 0)
      ASTERIA_THROW("Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

    cow_string res;
    if(static_cast<uint64_t>(rhs) >= res.max_size() - lhs.size())
      ASTERIA_THROW("String length overflow (`$1` + `$2` > `$3`)", lhs.size(), rhs, res.max_size());

    // Append spaces in the right and return the result.
    size_t count = static_cast<size_t>(rhs);
    res.assign(lhs).append(count, ' ');
    return res;
  }

cow_string
do_string_sra(const cow_string& lhs, int64_t rhs)
  {
    if(rhs < 0)
      ASTERIA_THROW("Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

    cow_string res;
    if(static_cast<uint64_t>(rhs) >= lhs.size())
      return res;

    // Return the substring in the left.
    size_t count = static_cast<size_t>(rhs);
    res.append(lhs.data(), lhs.size() - count);
    return res;
  }

cow_string
do_string_notb(cow_string&& rhs)
  {
    // The length of the result is the same as the operand.
    auto tp = reinterpret_cast<uint8_t*>(rhs.mut_data());

    // Flip every byte in the string.
    for(size_t i = 0;  i < rhs.size();  ++i)
      tp[i] = static_cast<uint8_t>(-1 ^ tp[i]);
    return ::std::move(rhs);
  }

cow_string
do_string_andb(const cow_string& lhs, cow_string&& rhs)
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

cow_string
do_string_orb(const cow_string& lhs, cow_string&& rhs)
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

cow_string
do_string_xorb(const cow_string& lhs, cow_string&& rhs)
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

struct AIR_Traits_apply_operator_common
  {
    // `Uparam` is `assign`.
    // `Sparam` is unused.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = altr.assign;
        return up;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }
  };

struct AIR_Traits_apply_operator_inc_post : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& /*up*/)
      {
        // This operator is unary.
        auto& lhs = ctx.stack().get_top().open();
        Reference::S_temporary xref = { lhs };

        switch(do_vmask_of(lhs)) {
          case vmask_integer:
            // Increment the operand and return the old value. `assign` is ignored.
            lhs = do_check_add(lhs.as_integer(), 1);
            break;

          case vmask_real:
            // Increment the operand and return the old value. `assign` is ignored.
            lhs.open_real() += 1;
            break;

          default:
            ASTERIA_THROW("Postfix increment not applicable (operand was `$1`)", lhs);
        }
        ctx.stack().open_top() = ::std::move(xref);
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_dec_post : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& /*up*/)
      {
        // This operator is unary.
        auto& lhs = ctx.stack().get_top().open();
        Reference::S_temporary xref = { lhs };

        switch(do_vmask_of(lhs)) {
          case vmask_integer:
            // Decrement the operand and return the old value. `assign` is ignored.
            lhs = do_check_sub(lhs.as_integer(), 1);
            break;

          case vmask_real:
            // Decrement the operand and return the old value. `assign` is ignored.
            lhs.open_real() -= 1;
            break;

          default:
            ASTERIA_THROW("Postfix decrement not applicable (operand was `$1`)", lhs);
        }
        ctx.stack().open_top() = ::std::move(xref);
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_subscr : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& /*up*/)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        auto& lref = ctx.stack().open_top();

        switch(do_vmask_of(rhs)) {
          case vmask_integer: {
            // Append an array subscript. `assign` is ignored.
            Reference::M_array_index xmod = { rhs.as_integer() };
            lref.zoom_in(::std::move(xmod));
            break;
          }

          case vmask_string: {
            // Append an object subscript. `assign` is ignored.
            Reference::M_object_key xmod = { ::std::move(rhs.open_string()) };
            lref.zoom_in(::std::move(xmod));
            break;
          }

          default:
            ASTERIA_THROW("Subscript value not valid (subscript was `$1`)", rhs);
        }
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_pos : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };

        // Copy the operand to create a temporary value.
        // N.B. This is one of the few operators that work on all types.
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_neg : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // Get the opposite of the operand as a temporary value.
            rhs = do_check_neg(rhs.as_integer());
            break;

          case vmask_real:
            // Get the opposite of the operand as a temporary value.
            rhs = -rhs.as_real();
            break;

          default:
            ASTERIA_THROW("Prefix negation not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_notb : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_boolean:
            // Perform logical NOT operation on the operand to create a temporary value.
            rhs.open_boolean() ^= true;
            break;

          case vmask_integer:
            // Perform bitwise NOT operation on the operand to create a temporary value.
            rhs.open_integer() ^= -1;
            break;

          case vmask_string:
            // Perform bitwise NOT operation on all bytes in the operand to create a temporary value.
            rhs = do_string_notb(::std::move(rhs.open_string()));
            break;

          default:
            ASTERIA_THROW("Prefix bitwise NOT not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_notl : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        // Perform logical NOT operation on the operand to create a temporary value.
        // N.B. This is one of the few operators that work on all types.
        rhs = !rhs.test();

        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_inc_pre : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& /*up*/)
      {
        // This operator is unary.
        auto& rhs = ctx.stack().get_top().open();

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // Increment the operand and return it. `assign` is ignored.
            rhs = do_check_add(rhs.as_integer(), 1);
            break;

          case vmask_real:
            // Increment the operand and return it. `assign` is ignored.
            rhs.open_real() += 1;
            break;

          default:
            ASTERIA_THROW("Prefix increment not applicable (operand was `$1`)", rhs);
        }
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_dec_pre : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& /*up*/)
      {
        // This operator is unary.
        auto& rhs = ctx.stack().get_top().open();

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // Decrement the operand and return it. `assign` is ignored.
            rhs = do_check_sub(rhs.as_integer(), 1);
            break;

          case vmask_real:
            // Decrement the operand and return it. `assign` is ignored.
            rhs.open_real() -= 1;
            break;

          default:
            ASTERIA_THROW("Prefix decrement not applicable (operand was `$1`)", rhs);
        }
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_unset : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().unset() };

        // Unset the reference and return the old value.
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_countof : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        // Return the number of elements in the operand.
        switch(do_vmask_of(rhs)) {
          case vmask_null:
            rhs = 0;
            break;

          case vmask_string:
            rhs = rhs.as_string().ssize();
            break;

          case vmask_array:
            rhs = rhs.as_array().ssize();
            break;

          case vmask_object:
            rhs = rhs.as_object().ssize();
            break;

          default:
            ASTERIA_THROW("Prefix `countof` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_typeof : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        // Return the type name of the operand, which is static.
        // N.B. This is one of the few operators that work on all types.
        rhs = ::rocket::sref(rhs.what_type());

        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_sqrt : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // Get the square root of the operand as a temporary value.
            rhs = ::std::sqrt(static_cast<double>(rhs.as_integer()));
            break;

          case vmask_real:
            // Get the square root of the operand as a temporary value.
            rhs = ::std::sqrt(rhs.as_real());
            break;

          default:
            ASTERIA_THROW("Prefix `__sqrt` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_isnan : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // An integer is never a NaN.
            rhs = false;
            break;

          case vmask_real:
            // Check whether the operand is a NaN, store the result in a temporary value.
            rhs = ::std::isnan(rhs.as_real());
            break;

          default:
            ASTERIA_THROW("Prefix `__isnan` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_isinf : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // An integer is never an infinity.
            rhs = false;
            break;

          case vmask_real:
            // Check whether the operand is an infinity, store the result in a temporary value.
            rhs = ::std::isinf(rhs.as_real());
            break;

          default:
            ASTERIA_THROW("Prefix `__isinf` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_abs : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // Get the absolute value of the operand as a temporary value.
            rhs = ::std::abs(do_check_neg(rhs.as_integer()));
            break;

          case vmask_real:
            // Get the absolute value of the operand as a temporary value.
            rhs = ::std::fabs(rhs.as_real());
            break;

          default:
            ASTERIA_THROW("Prefix `__abs` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_sign : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // Get the sign bit of the operand as a temporary value.
            rhs.open_integer() >>= 63;
            break;

          case vmask_real:
            // Get the sign bit of the operand as a temporary value.
            rhs = ::std::signbit(rhs.as_real()) ? INT64_C(-1) : 0;
            break;

          default:
            ASTERIA_THROW("Prefix `__sign` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_round : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // No conversion is needed.
            break;

          case vmask_real:
            // Round the operand to the nearest integer as a temporary real.
            rhs = ::std::round(rhs.as_real());
            break;

          default:
            ASTERIA_THROW("Prefix `__round` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_floor : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // No conversion is needed.
            break;

          case vmask_real:
            // Round the operand towards positive infinity as a temporary real.
            rhs = ::std::floor(rhs.as_real());
            break;

          default:
            ASTERIA_THROW("Prefix `__floor` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_ceil : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // No conversion is needed.
            break;

          case vmask_real:
            // Round the operand towards negative infinity as a temporary real.
            rhs = ::std::ceil(rhs.as_real());
            break;

          default:
            ASTERIA_THROW("Prefix `__ceil` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_trunc : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // No conversion is needed.
            break;

          case vmask_real:
            // Round the operand to zero as a temporary real.
            rhs = ::std::trunc(rhs.as_real());
            break;

          default:
            ASTERIA_THROW("Prefix `__trunc` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_iround : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // No conversion is needed.
            break;

          case vmask_real:
            // Round the operand to the nearest integer as a temporary integer.
            rhs = do_check_itrunc(::std::round(rhs.as_real()));
            break;

          default:
            ASTERIA_THROW("Prefix `__iround` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_ifloor : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // No conversion is needed.
            break;

          case vmask_real:
            // Round the operand to negative infinity as a temporary integer.
            rhs = do_check_itrunc(::std::floor(rhs.as_real()));
            break;

          default:
            ASTERIA_THROW("Prefix `__ifloor` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_iceil : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // No conversion is needed.
            break;

          case vmask_real:
            // Round the operand to positive infinity as a temporary integer.
            rhs = do_check_itrunc(::std::ceil(rhs.as_real()));
            break;

          default:
            ASTERIA_THROW("Prefix `__iceil` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_itrunc : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is unary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;

        switch(do_vmask_of(rhs)) {
          case vmask_integer:
            // No conversion is needed.
            break;

          case vmask_real:
            // Round the operand to zero as a temporary integer.
            rhs = do_check_itrunc(rhs.as_real());
            break;

          default:
            ASTERIA_THROW("Prefix `__itrunc` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_cmp_eq : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        rhs = comp == compare_equal;

        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_cmp_ne : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        rhs = comp != compare_equal;

        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_cmp_lt : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        // Throw an exception if the operands compare unequal.
        auto comp = lhs.compare(rhs);
        if(comp == compare_unordered)
          ASTERIA_THROW("Values not comparable (operands were `$1` and `$2`)", lhs, rhs);
        rhs = comp == compare_less;

        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_cmp_gt : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        // Throw an exception if the operands compare unequal.
        auto comp = lhs.compare(rhs);
        if(comp == compare_unordered)
          ASTERIA_THROW("Values not comparable (operands were `$1` and `$2`)", lhs, rhs);
        rhs = comp == compare_greater;

        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_cmp_lte : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        // Throw an exception if the operands compare unequal.
        auto comp = lhs.compare(rhs);
        if(comp == compare_unordered)
          ASTERIA_THROW("Values not comparable (operands were `$1` and `$2`)", lhs, rhs);
        rhs = comp != compare_greater;

        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_cmp_gte : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        // Throw an exception if the operands compare unequal.
        auto comp = lhs.compare(rhs);
        if(comp == compare_unordered)
          ASTERIA_THROW("Values not comparable (operands were `$1` and `$2`)", lhs, rhs);
        rhs = comp != compare_less;

        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_cmp_3way : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        switch(comp) {
          case compare_greater:
            rhs = +1;
            break;

          case compare_less:
            rhs = -1;
            break;

          case compare_equal:
            rhs = 0;
            break;

          case compare_unordered:
            rhs = ::rocket::sref("[unordered]");
            break;

          default:
            ROCKET_ASSERT(false);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_add : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        switch(do_vmask_of(lhs) | do_vmask_of(rhs)) {
          case vmask_boolean:
            // For the `boolean` type, return the logical OR'd result of both operands.
            rhs.open_boolean() |= lhs.as_boolean();
            break;

          case vmask_integer:
            // For the `integer` type, return the sum of both operands.
            rhs = do_check_add(lhs.as_integer(), rhs.as_integer());
            break;

          case vmask_integer | vmask_real:
          case vmask_real:
            // For the `integer` and `real` types, return the sum of both operands as `real`.
            rhs.mutate_into_real() += lhs.convert_to_real();
            break;

          case vmask_string:
            // For the `string` type, concatenate the operands in lexical order to create a new string.
            rhs.open_string().insert(0, lhs.as_string());
            break;

          default:
            ASTERIA_THROW("Infix addition not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_sub : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        switch(do_vmask_of(lhs) | do_vmask_of(rhs)) {
          case vmask_boolean:
            // For the `boolean` type, return the logical XOR'd result of both operands.
            rhs.open_boolean() ^= lhs.as_boolean();
            break;

          case vmask_integer:
            // For the `integer` type, return the difference of both operands.
            rhs = do_check_sub(lhs.as_integer(), rhs.as_integer());
            break;

          case vmask_integer | vmask_real:
          case vmask_real:
            // For the `integer` and `real` types, return the difference of both operands as `real`.
            rhs = lhs.convert_to_real() - rhs.convert_to_real();
            break;

          default:
            ASTERIA_THROW("Infix subtraction not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_mul : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        switch(do_vmask_of(lhs) | do_vmask_of(rhs)) {
          case vmask_boolean:
            // For the `boolean` type, return the logical AND'd result of both operands.
            rhs.open_boolean() &= lhs.as_boolean();
            break;

          case vmask_integer:
            // For the `integer` type, return the product of both operands.
            rhs = do_check_mul(lhs.as_integer(), rhs.as_integer());
            break;

          case vmask_integer | vmask_real:
          case vmask_real:
            // For the `integer` and `real` types, return the product of both operands as `real`.
            rhs.mutate_into_real() *= lhs.convert_to_real();
            break;

          case vmask_integer | vmask_string:
            // If either operand has type `string` and the other has type `integer`, duplicate
            // the string up to the specified number of times and return the result.
            rhs = lhs.is_string() ? do_string_dup(lhs.as_string(), rhs.as_integer())
                                  : do_string_dup(rhs.as_string(), lhs.as_integer());
            break;

          default:
            ASTERIA_THROW("Infix multiplication not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_div : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        switch(do_vmask_of(lhs) | do_vmask_of(rhs)) {
          case vmask_integer:
            // For the `integer` type, return the quotient of both operands.
            rhs = do_check_div(lhs.as_integer(), rhs.as_integer());
            break;

          case vmask_integer | vmask_real:
          case vmask_real:
            // For the `integer` and `real` types, return the quotient of both operands as `real`.
            rhs = lhs.convert_to_real() / rhs.convert_to_real();
            break;

          default:
            ASTERIA_THROW("Infix division not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_mod : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        switch(do_vmask_of(lhs) | do_vmask_of(rhs)) {
          case vmask_integer:
            // For the `integer` type, return the remainder of both operands.
            rhs = do_check_mod(lhs.as_integer(), rhs.as_integer());
            break;

          case vmask_integer | vmask_real:
          case vmask_real:
            // For the `integer` and `real` types, return the remainder of both operands.
            rhs = ::std::fmod(lhs.convert_to_real(), rhs.convert_to_real());
            break;

          default:
            ASTERIA_THROW("Infix modulo not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_sll : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        if(!rhs.is_integer())
          ASTERIA_THROW("Shift count not an integer (operands were `$1` and `$2`)", lhs, rhs);

        switch(do_vmask_of(lhs)) {
          case vmask_integer:
            // If the LHS operand has type `integer`, shift the LHS operand to the left by the number of bits
            // specified by the RHS operand. Bits shifted out are discarded. Bits shifted in are filled with zeroes.
            rhs = do_check_sll(lhs.as_integer(), rhs.as_integer());
            break;

          case vmask_string:
            // If the LHS operand has type `string`, fill space characters in the right and discard characters from
            // the left. The number of bytes in the LHS operand will be preserved.
            rhs = do_string_sll(lhs.as_string(), rhs.as_integer());
            break;

          default:
            ASTERIA_THROW("Infix logical left shift not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_srl : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        if(!rhs.is_integer())
          ASTERIA_THROW("Shift count not an integer (operands were `$1` and `$2`)", lhs, rhs);

        switch(do_vmask_of(lhs)) {
          case vmask_integer:
            // If the LHS operand has type `integer`, shift the LHS operand to the right by the number of bits
            // specified by the RHS operand. Bits shifted out are discarded. Bits shifted in are filled with zeroes.
            rhs = do_check_srl(lhs.as_integer(), rhs.as_integer());
            break;

          case vmask_string:
            // If the LHS operand has type `string`, fill space characters in the left and discard characters from
            // the right. The number of bytes in the LHS operand will be preserved.
            rhs = do_string_srl(lhs.as_string(), rhs.as_integer());
            break;

          default:
            ASTERIA_THROW("Infix logical right shift not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_sla : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        if(!rhs.is_integer())
          ASTERIA_THROW("Shift count not an integer (operands were `$1` and `$2`)", lhs, rhs);

        switch(do_vmask_of(lhs)) {
          case vmask_integer:
            // If the LHS operand is of type `integer`, shift the LHS operand to the left by the number of bits
            // specified by the RHS operand. Bits shifted out that are equal to the sign bit are discarded. Bits
            // shifted in are filled with zeroes. If any bits that are different from the sign bit would be shifted
            // out, an exception is thrown.
            rhs = do_check_sla(lhs.as_integer(), rhs.as_integer());
            break;

          case vmask_string:
            // If the LHS operand has type `string`, fill space characters in the right.
            rhs = do_string_sla(lhs.as_string(), rhs.as_integer());
            break;

          default:
            ASTERIA_THROW("Infix arithmetic left shift not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_sra : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        if(!rhs.is_integer())
          ASTERIA_THROW("Shift count not an integer (operands were `$1` and `$2`)", lhs, rhs);

        switch(do_vmask_of(lhs)) {
          case vmask_integer:
            // If the LHS operand is of type `integer`, shift the LHS operand to the right by the number of bits
            // specified by the RHS operand. Bits shifted out are discarded. Bits shifted in are filled with the
            // sign bit.
            rhs = do_check_sra(lhs.as_integer(), rhs.as_integer());
            break;

          case vmask_string:
            // If the LHS operand has type `string`, discard characters from the right.
            rhs = do_string_sra(lhs.as_string(), rhs.as_integer());
            break;

          default:
            ASTERIA_THROW("Infix arithmetic right shift not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_andb : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        switch(do_vmask_of(lhs) | do_vmask_of(rhs)) {
          case vmask_boolean:
            // For the `boolean` type, return the logical AND'd result of both operands.
            rhs.open_boolean() &= lhs.as_boolean();
            break;

          case vmask_integer:
            // For the `integer` type, return bitwise AND'd result of both operands.
            rhs.open_integer() &= lhs.as_integer();
            break;

          case vmask_string:
            // For the `string` type, return bitwise AND'd result of bytes from operands.
            // The result contains no more bytes than either operand.
            rhs = do_string_andb(lhs.as_string(), ::std::move(rhs.open_string()));
            break;

          default:
            ASTERIA_THROW("Infix bitwise AND not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_orb : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        switch(do_vmask_of(lhs) | do_vmask_of(rhs)) {
          case vmask_boolean:
            // For the `boolean` type, return the logical OR'd result of both operands.
            rhs.open_boolean() |= lhs.as_boolean();
            break;

          case vmask_integer:
            // For the `integer` type, return bitwise OR'd result of both operands.
            rhs.open_integer() |= lhs.as_integer();
            break;

          case vmask_string:
            // For the `string` type, return bitwise OR'd result of bytes from operands.
            // The result contains no fewer bytes than either operand.
            rhs = do_string_orb(lhs.as_string(), ::std::move(rhs.open_string()));
            break;

          default:
            ASTERIA_THROW("Infix bitwise OR not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_xorb : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is binary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        switch(do_vmask_of(lhs) | do_vmask_of(rhs)) {
          case vmask_boolean:
            // For the `boolean` type, return the logical XOR'd result of both operands.
            rhs.open_boolean() ^= lhs.as_boolean();
            break;

          case vmask_integer:
            // For the `integer` type, return bitwise XOR'd result of both operands.
            rhs.open_integer() ^= lhs.as_integer();
            break;

          case vmask_string:
            // For the `string` type, return bitwise XOR'd result of bytes from operands.
            // The result contains no fewer bytes than either operand.
            rhs = do_string_xorb(lhs.as_string(), ::std::move(rhs.open_string()));
            break;

          default:
            ASTERIA_THROW("Infix bitwise XOR not applicable (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_assign : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& /*up*/)
      {
        // Pop the RHS operand.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();

        // Copy the value to the LHS operand which is write-only. `assign` is ignored.
        ctx.stack().get_top().open() = ::std::move(rhs);
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_fma : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // This operator is ternary.
        Reference::S_temporary xref = { ctx.stack().get_top().read() };
        auto& rhs = xref.val;
        ctx.stack().pop();
        auto mid = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();

        switch(do_vmask_of(lhs) | do_vmask_of(rhs)) {
          case vmask_integer:
          case vmask_integer | vmask_real:
          case vmask_real:
            // Calculate the fused multiply-add result of the operands.
            rhs = ::std::fma(lhs.convert_to_real(), mid.convert_to_real(), rhs.convert_to_real());
            break;

          default:
            ASTERIA_THROW("Fused multiply-add not applicable (operands were `$1`, `$2` and `$3`)", lhs, mid, rhs);
        }
        do_set_temporary(ctx, up.v8s[0], ::std::move(xref));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_head : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& /*up*/)
      {
        // This operator is unary.
        auto& lref = ctx.stack().open_top();

        Reference::M_array_head xmod = { };
        lref.zoom_in(::std::move(xmod));
        return air_status_next;
      }
  };

struct AIR_Traits_apply_operator_tail : AIR_Traits_apply_operator_common
  {
    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& /*up*/)
      {
        // This operator is unary.
        auto& lref = ctx.stack().open_top();

        Reference::M_array_tail xmod = { };
        lref.zoom_in(::std::move(xmod));
        return air_status_next;
      }
  };

struct AIR_Traits_unpack_struct_array
  {
    // `Uparam` is `immutable` and `nelems`.
    // `Sparam` is unused.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_unpack_struct_array& altr)
      {
        AVMC_Queue::Uparam up;
        up.y32 = altr.nelems;
        up.y8s[0] = altr.immutable;
        return up;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_unpack_struct_array& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up)
      {
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this function.
        auto val = ctx.stack().get_top().read();
        ctx.stack().pop();

        // Make sure it is really an `array`.
        V_array arr;
        if(!val.is_null() && !val.is_array())
          ASTERIA_THROW("Invalid argument for structured array binding (initializer was `$1`)", val);

        if(val.is_array())
          arr = ::std::move(val.open_array());

        for(uint32_t i = up.y32 - 1;  i != UINT32_MAX;  --i) {
          // Get the variable back.
          auto var = ctx.stack().get_top().get_variable_opt();
          ctx.stack().pop();

          // Initialize it.
          ROCKET_ASSERT(var && !var->is_initialized());
          auto qinit = arr.mut_ptr(i);
          if(qinit)
            var->initialize(::std::move(*qinit), up.y8s[0]);
          else
            var->initialize(V_null(), up.y8s[0]);
        }
        return air_status_next;
      }
  };

struct AIR_Traits_unpack_struct_object
  {
    // `Uparam` is `immutable`.
    // `Sparam` is the list of keys.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_unpack_struct_object& altr)
      {
        AVMC_Queue::Uparam up;
        up.y8s[0] = altr.immutable;
        return up;
      }

    static
    cow_vector<phsh_string>
    make_sparam(bool& /*reachable*/, const AIR_Node::S_unpack_struct_object& altr)
      {
        return altr.keys;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_unpack_struct_object& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const cow_vector<phsh_string>& keys)
      {
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this function.
        auto val = ctx.stack().get_top().read();
        ctx.stack().pop();

        // Make sure it is really an `object`.
        V_object obj;
        if(!val.is_null() && !val.is_object())
          ASTERIA_THROW("Invalid argument for structured object binding (initializer was `$1`)", val);

        if(val.is_object())
          obj = ::std::move(val.open_object());

        for(auto it = keys.rbegin();  it != keys.rend();  ++it) {
          // Get the variable back.
          auto var = ctx.stack().get_top().get_variable_opt();
          ctx.stack().pop();

          // Initialize it.
          ROCKET_ASSERT(var && !var->is_initialized());
          auto qinit = obj.mut_ptr(*it);
          if(qinit)
            var->initialize(::std::move(*qinit), up.y8s[0]);
          else
            var->initialize(V_null(), up.y8s[0]);
        }
        return air_status_next;
      }
  };

struct AIR_Traits_define_null_variable
  {
    // `Uparam` is `immutable`.
    // `Sparam` is the source location and name.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_define_null_variable& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = altr.immutable;
        return up;
      }

    static
    Sparam_sloc_name
    make_sparam(bool& /*reachable*/, const AIR_Node::S_define_null_variable& altr)
      {
        Sparam_sloc_name sp;
        sp.sloc = altr.sloc;
        sp.name = altr.name;
        return sp;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_define_null_variable& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Sparam_sloc_name& sp)
      {
        // Allocate an uninitialized variable.
        auto gcoll = ctx.global().genius_collector();
        auto var = gcoll->create_variable();

        // Inject the variable into the current context.
        Reference::S_variable xref = { var };
        ctx.open_named_reference(sp.name) = ::std::move(xref);

        // Call the hook function if any.
        if(auto qhooks = ctx.global().get_hooks_opt())
          qhooks->on_variable_declare(sp.sloc, sp.name);

        // Initialize the variable to `null`.
        var->initialize(V_null(), up.v8s[0]);
        return air_status_next;
      }
  };

struct AIR_Traits_single_step_trap
  {
    // `Uparam` is unused.
    // `Sparam` is the source location.

    static
    Source_Location
    make_sparam(bool& /*reachable*/, const AIR_Node::S_single_step_trap& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_single_step_trap& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Source_Location& sloc)
      {
        // Call the hook function if any.
        if(auto qhooks = ctx.global().get_hooks_opt())
          qhooks->on_single_step_trap(sloc);
        return air_status_next;
      }
  };

struct AIR_Traits_variadic_call
  {
    // `Uparam` is `ptc`.
    // `Sparam` is the source location.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_variadic_call& altr)
      {
        AVMC_Queue::Uparam up;
        up.y8s[0] = weaken_enum(altr.ptc);
        return up;
      }

    static
    Source_Location
    make_sparam(bool& /*reachable*/, const AIR_Node::S_variadic_call& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_variadic_call& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Source_Location& sloc)
      {
        // Check for stack overflows.
        const auto sentry = ctx.global().copy_recursion_sentry();

        // Generate a single-step trap.
        if(auto qhooks = ctx.global().get_hooks_opt())
          qhooks->on_single_step_trap(sloc);

        // Pop the argument generator.
        cow_vector<Reference> args;
        auto value = ctx.stack().get_top().read();
        switch(weaken_enum(value.type())) {
          case type_null:
            // Leave `args` empty.
            break;

          case type_array: {
            auto source = ::std::move(value.open_array());
            ctx.stack().pop();

            // Convert all elements to temporaries.
            args.reserve(source.size());
            for(size_t i = 0;  i < source.size();  ++i) {
              // Make a reference to temporary.
              Reference::S_temporary xref = { ::std::move(source.mut(i)) };
              args.emplace_back(::std::move(xref));
            }
            break;
          }

          case type_function: {
            const auto generator = ::std::move(value.open_function());
            auto gself = ctx.stack().open_top().zoom_out();

            // Pass an empty argument list to get the number of arguments to generate.
            cow_vector<Reference> gargs;
            do_invoke_nontail(ctx.stack().open_top(), sloc, ctx, generator, ::std::move(gargs));
            value = ctx.stack().get_top().read();
            ctx.stack().pop();

            // Verify the argument count.
            if(!value.is_integer())
              ASTERIA_THROW("Invalid number of variadic arguments (value `$1`)", value);

            int64_t nvargs = value.as_integer();
            if((nvargs < 0) || (nvargs > INT_MAX))
              ASTERIA_THROW("Number of variadic arguments not acceptable (nvargs `$1`)", nvargs);

            // Generate arguments.
            args.assign(static_cast<size_t>(nvargs), gself);
            for(size_t i = 0;  i < args.size();  ++i) {
              // Initialize the argument list for the generator.
              Reference::S_constant xref = { int64_t(i) };
              gargs.clear().emplace_back(::std::move(xref));

              // Generate an argument. Ensure it is dereferenceable.
              do_invoke_nontail(args.mut(i), sloc, ctx, generator, ::std::move(gargs));
              static_cast<void>(args[i].read());
            }
            break;
          }

          default:
            ASTERIA_THROW("Invalid variadic argument generator (value `$1`)", value);
        }

        // Copy the target, which shall be of type `function`.
        value = ctx.stack().get_top().read();
        if(!value.is_function())
          ASTERIA_THROW("Attempt to call a non-function (value `$1`)", value);
        auto& self = ctx.stack().open_top().zoom_out();

        return do_function_call_common(self, sloc, ctx, value.as_function(),
                                       static_cast<PTC_Aware>(up.y8s[0]), ::std::move(args));
      }
  };

struct AIR_Traits_defer_expression
  {
    // `Uparam` is unused.
    // `Sparam` is the source location and body.

    static
    Sparam_defer
    make_sparam(bool& /*reachable*/, const AIR_Node::S_defer_expression& altr)
      {
        Sparam_defer sp;
        sp.sloc = altr.sloc;
        sp.code_body = altr.code_body;
        return sp;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_defer_expression& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_defer& sp)
      {
        // Rebind the body here.
        bool dirty = false;
        auto bound_body = sp.code_body;
        do_rebind_nodes(dirty, bound_body, ctx);

        // Solidify it.
        AVMC_Queue queue;
        do_solidify_nodes(queue, bound_body);

        // Push this expression.
        ctx.defer_expression(sp.sloc, ::std::move(queue));
        return air_status_next;
      }
  };

struct AIR_Traits_import_call
  {
    // `Uparam` is `nargs`.
    // `Sparam` is the source location and compiler options.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_import_call& altr)
      {
        AVMC_Queue::Uparam up;
        up.y32 = altr.nargs;
        return up;
      }

    static
    Sparam_import
    make_sparam(bool& /*reachable*/, const AIR_Node::S_import_call& altr)
      {
        Sparam_import sp;
        sp.opts = altr.opts;
        sp.sloc = altr.sloc;
        return sp;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_import_call& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Sparam_import& sp)
      {
        // Check for stack overflows.
        const auto sentry = ctx.global().copy_recursion_sentry();

        // Generate a single-step trap.
        if(auto qhooks = ctx.global().get_hooks_opt())
          qhooks->on_single_step_trap(sp.sloc);

        // Pop arguments off the stack backwards.
        ROCKET_ASSERT(up.y32 != 0);
        auto args = do_pop_positional_arguments(ctx, up.y32 - 1);

        // Copy the filename, which shall be of type `string`.
        auto value = ctx.stack().get_top().read();
        if(!value.is_string())
          ASTERIA_THROW("Invalid path specified for `import` (value `$1` not a string)", value);

        auto path = value.as_string();
        if(path.empty())
          ASTERIA_THROW("Empty path specified for `import`");

        // Rewrite the path if it is not absolute.
        if((path[0] != '/') && (sp.sloc.c_file()[0] == '/')) {
          path.assign(sp.sloc.file());
          path.erase(path.rfind('/') + 1);
          path.append(value.as_string());
        }

        auto abspath = ::rocket::make_unique_handle(::realpath(path.safe_c_str(), nullptr), ::free);
        if(!abspath)
          ASTERIA_THROW("Could not open script file '$2'\n"
                        "[`realpath()` failed: $1]",
                        format_errno(errno), path);
        path.assign(abspath);

        // Compile the script file into a function object.
        Loader_Lock::Unique_Stream strm;
        strm.reset(ctx.global().loader_lock(), path.safe_c_str());

        // Parse source code.
        Token_Stream tstrm(sp.opts);
        tstrm.reload(path, 1, strm);

        Statement_Sequence stmtq(sp.opts);
        stmtq.reload(tstrm);

        // Instantiate the function.
        AIR_Optimizer optmz(sp.opts);
        optmz.reload(nullptr, cow_vector<phsh_string>(1, ::rocket::sref("...")), stmtq);
        auto qtarget = optmz.create_function(Source_Location(path, 0, 0), ::rocket::sref("[file scope]"));

        // Update the first argument to `import` if it was passed by reference.
        // `this` is null for imported scripts.
        auto& self = ctx.stack().open_top();
        if(self.is_lvalue())
          self.open() = path;
        self = Reference::S_constant();

        return do_function_call_common(self, sp.sloc, ctx, qtarget, ptc_aware_none, ::std::move(args));
      }
  };

struct AIR_Traits_break_or_continue
  {
    // `Uparam` is `status`.
    // `Sparam` is the source location.

    static
    AVMC_Queue::Uparam
    make_uparam(bool& reachable, const AIR_Node::S_break_or_continue& altr)
      {
        AVMC_Queue::Uparam up;
        up.v8s[0] = weaken_enum(altr.status);
        reachable = false;
        return up;
      }

    static
    Source_Location
    make_sparam(bool& /*reachable*/, const AIR_Node::S_break_or_continue& altr)
      {
        return altr.sloc;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue::Uparam& up, const Source_Location& sloc)
      {
        Reference::S_jump_src xref = { sloc };
        ctx.stack().push(::std::move(xref));

        auto status = static_cast<AIR_Status>(up.v8s[0]);
        ROCKET_ASSERT(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_switch,
                                                    air_status_break_while, air_status_break_for,
                                                    air_status_continue_unspec, air_status_continue_while,
                                                    air_status_continue_for }));
        return status;
      }
  };

struct AIR_Traits_declare_reference
  {
    // `Uparam` is unused.
    // `Sparam` is the name;

    static
    Sparam_name
    make_sparam(bool& /*reachable*/, const AIR_Node::S_declare_reference& altr)
      {
        Sparam_name sp;
        sp.name = altr.name;
        return sp;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_name& sp)
      {
        // Inject a placeholder reference into the current context.
        ctx.open_named_reference(sp.name) = Reference::S_uninit();
        return air_status_next;
      }
  };

struct AIR_Traits_initialize_reference
  {
    // `Uparam` is unused.
    // `Sparam` is the name;

    static
    Sparam_name
    make_sparam(bool& /*reachable*/, const AIR_Node::S_initialize_reference& altr)
      {
        Sparam_name sp;
        sp.name = altr.name;
        return sp;
      }

    static
    AVMC_Queue::Symbols
    make_symbols(const AIR_Node::S_initialize_reference& altr)
      {
        AVMC_Queue::Symbols syms;
        syms.sloc = altr.sloc;
        return syms;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_name& sp)
      {
        // Pop a reference from the stack. Ensure it is dereferenceable.
        auto& top = ctx.stack().open_top();
        static_cast<void>(top.read());

        // Move it into the context.
        ctx.open_named_reference(sp.name) = ::std::move(top);
        ctx.stack().pop();
        return air_status_next;
      }
  };

// These are helper type traits.
// Depending on the existence of Uparam, Sparam and Symbols, the code will look very different.

// check for `make_uparam`
template<typename TraitsT, typename XNodeT, typename = void>
struct Uparam_of
  : ::rocket::identity<void>
  { };

template<typename TraitsT, typename XNodeT>
struct Uparam_of<TraitsT, XNodeT, ROCKET_VOID_T(decltype(&TraitsT::make_uparam))>
  : ::std::decay<decltype(TraitsT::make_uparam(::std::declval<bool&>(), ::std::declval<const XNodeT&>()))>
  { };

// check for `make_sparam`
template<typename TraitsT, typename XNodeT, typename = void>
struct Sparam_of
  : ::rocket::identity<void>
  { };

template<typename TraitsT, typename XNodeT>
struct Sparam_of<TraitsT, XNodeT, ROCKET_VOID_T(decltype(&TraitsT::make_sparam))>
  : ::std::decay<decltype(TraitsT::make_sparam(::std::declval<bool&>(), ::std::declval<const XNodeT&>()))>
  { };

// check for `make_symbols`
template<typename TraitsT, typename XNodeT, typename = void>
struct Symbols_of
  : ::rocket::identity<void>
  { };

template<typename TraitsT, typename XNodeT>
struct Symbols_of<TraitsT, XNodeT, ROCKET_VOID_T(decltype(&TraitsT::make_symbols))>
  : ::std::decay<decltype(TraitsT::make_symbols(::std::declval<const XNodeT&>()))>
  { };

// executor thunk
template<typename TraitsT, typename UparamT, typename SparamT>
struct Executor_of
  {
    static
    AIR_Status
    thunk(Executive_Context& ctx, AVMC_Queue::Uparam up, const void* sp)
      { return TraitsT::execute(ctx, UparamT(up), *(const SparamT*)sp);  }
  };

template<typename TraitsT, typename UparamT>
struct Executor_of<TraitsT, UparamT, void>
  {
    static
    AIR_Status
    thunk(Executive_Context& ctx, AVMC_Queue::Uparam up, const void* /*sp*/)
      { return TraitsT::execute(ctx, UparamT(up));  }
  };

template<typename TraitsT, typename SparamT>
struct Executor_of<TraitsT, void, SparamT>
  {
    static
    AIR_Status
    thunk(Executive_Context& ctx, AVMC_Queue::Uparam /*up*/, const void* sp)
      { return TraitsT::execute(ctx, *(const SparamT*)sp);  }
  };

template<typename TraitsT>
struct Executor_of<TraitsT, void, void>
  {
    static
    AIR_Status
    thunk(Executive_Context& ctx, AVMC_Queue::Uparam /*up*/, const void* /*sp*/)
      { return TraitsT::execute(ctx);  }
  };

// enumerator thunk
template<typename SparamT, typename = void>
struct Enumerator_of
  {
    static constexpr AVMC_Queue::Enumerator* thunk = nullptr;
  };

template<typename SparamT>
struct Enumerator_of<SparamT, ROCKET_VOID_T(decltype(&SparamT::enumerate_variables))>
  {
    static
    Variable_Callback&
    thunk(Variable_Callback& callback, AVMC_Queue::Uparam /*up*/, const void* sp)
      { return static_cast<const SparamT*>(sp)->enumerate_variables(callback);  }
  };

// Finally...
template<typename TraitsT, typename XNodeT, typename UparamT, typename SparamT, typename SymbolT>
struct AVMC_Appender
  {
    static
    bool
    do_append(AVMC_Queue& queue, const XNodeT& altr)
      {
        bool reachable = true;
        queue.template append<Executor_of<TraitsT, UparamT, SparamT>::thunk,
                              Enumerator_of<SparamT>::thunk>(
                              TraitsT::make_symbols(altr),
                              TraitsT::make_uparam(reachable, altr),
                              TraitsT::make_sparam(reachable, altr));
        return reachable;
      }
  };

template<typename TraitsT, typename XNodeT, typename UparamT, typename SparamT>
struct AVMC_Appender<TraitsT, XNodeT, UparamT, SparamT, void>
  {
    static
    bool
    do_append(AVMC_Queue& queue, const XNodeT& altr)
      {
        bool reachable = true;
        queue.template append<Executor_of<TraitsT, UparamT, SparamT>::thunk,
                              Enumerator_of<SparamT>::thunk>(
                              TraitsT::make_uparam(reachable, altr),
                              TraitsT::make_sparam(reachable, altr));
        return reachable;
      }
  };

template<typename TraitsT, typename XNodeT, typename UparamT, typename SymbolT>
struct AVMC_Appender<TraitsT, XNodeT, UparamT, void, SymbolT>
  {
    static
    bool
    do_append(AVMC_Queue& queue, const XNodeT& altr)
      {
        bool reachable = true;
        queue.template append<Executor_of<TraitsT, UparamT, void>::thunk,
                              nullptr>(
                              TraitsT::make_symbols(altr),
                              TraitsT::make_uparam(reachable, altr));
        return reachable;
      }
  };

template<typename TraitsT, typename XNodeT, typename UparamT>
struct AVMC_Appender<TraitsT, XNodeT, UparamT, void, void>
  {
    static
    bool
    do_append(AVMC_Queue& queue, const XNodeT& altr)
      {
        bool reachable = true;
        queue.template append<Executor_of<TraitsT, UparamT, void>::thunk,
                              nullptr>(
                              TraitsT::make_uparam(reachable, altr));
        return reachable;
      }
  };

template<typename TraitsT, typename XNodeT, typename SparamT, typename SymbolT>
struct AVMC_Appender<TraitsT, XNodeT, void, SparamT, SymbolT>
  {
    static
    bool
    do_append(AVMC_Queue& queue, const XNodeT& altr)
      {
        bool reachable = true;
        queue.template append<Executor_of<TraitsT, void, SparamT>::thunk,
                              Enumerator_of<SparamT>::thunk>(
                              TraitsT::make_symbols(altr),
                              AVMC_Queue::Uparam(),
                              TraitsT::make_sparam(reachable, altr));
        return reachable;
      }
  };

template<typename TraitsT, typename XNodeT, typename SparamT>
struct AVMC_Appender<TraitsT, XNodeT, void, SparamT, void>
  {
    static
    bool
    do_append(AVMC_Queue& queue, const XNodeT& altr)
      {
        bool reachable = true;
        queue.template append<Executor_of<TraitsT, void, SparamT>::thunk,
                              Enumerator_of<SparamT>::thunk>(
                              AVMC_Queue::Uparam(),
                              TraitsT::make_sparam(reachable, altr));
        return reachable;
      }
  };

template<typename TraitsT, typename XNodeT, typename SymbolT>
struct AVMC_Appender<TraitsT, XNodeT, void, void, SymbolT>
  {
    static
    bool
    do_append(AVMC_Queue& queue, const XNodeT& altr)
      {
        bool reachable = true;
        queue.template append<Executor_of<TraitsT, void, void>::thunk,
                              nullptr>(
                              TraitsT::make_symbols(altr),
                              AVMC_Queue::Uparam());
        return reachable;
      }
  };

template<typename TraitsT, typename XNodeT>
struct AVMC_Appender<TraitsT, XNodeT, void, void, void>
  {
    static
    bool
    do_append(AVMC_Queue& queue, const XNodeT& /*altr*/)
      {
        bool reachable = true;
        queue.template append<Executor_of<TraitsT, void, void>::thunk,
                              nullptr>(
                              AVMC_Queue::Uparam());
        return reachable;
      }
  };

template<typename TraitsT, typename XNodeT>
inline
bool
do_solidify(AVMC_Queue& queue, const XNodeT& altr)
  {
    return AVMC_Appender<TraitsT, XNodeT,
                         typename Uparam_of<TraitsT, XNodeT>::type,
                         typename Sparam_of<TraitsT, XNodeT>::type,
                         typename Symbols_of<TraitsT, XNodeT>::type>
               ::do_append(queue, altr);
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
        Analytic_Context ctx_body(::rocket::ref(ctx));
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
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_true, ctx_body);
        do_rebind_nodes(dirty, bound.code_false, ctx_body);

        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_switch_statement: {
        const auto& altr = this->m_stor.as<index_switch_statement>();

        // Rebind all clauses.
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_labels, ctx);  // this is not part of the body!
        do_rebind_nodes(dirty, bound.code_bodies, ctx_body);

        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_do_while_statement: {
        const auto& altr = this->m_stor.as<index_do_while_statement>();

        // Rebind the body and the condition.
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx_body);
        do_rebind_nodes(dirty, bound.code_cond, ctx);  // this is not part of the body!

        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_while_statement: {
        const auto& altr = this->m_stor.as<index_while_statement>();

        // Rebind the condition and the body.
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_cond, ctx);  // this is not part of the body!
        do_rebind_nodes(dirty, bound.code_body, ctx_body);

        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_for_each_statement: {
        const auto& altr = this->m_stor.as<index_for_each_statement>();

        // Rebind the range initializer and the body.
        Analytic_Context ctx_for(::rocket::ref(ctx));
        Analytic_Context ctx_body(::rocket::ref(ctx_for));
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_init, ctx_for);
        do_rebind_nodes(dirty, bound.code_body, ctx_body);

        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_for_statement: {
        const auto& altr = this->m_stor.as<index_for_statement>();

        // Rebind the initializer, the condition, the loop increment and the body.
        Analytic_Context ctx_for(::rocket::ref(ctx));
        Analytic_Context ctx_body(::rocket::ref(ctx_for));
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
        Analytic_Context ctx_body(::rocket::ref(ctx));
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_try, ctx_body);
        do_rebind_nodes(dirty, bound.code_catch, ctx_body);

        return do_forward_if_opt(dirty, ::std::move(bound));
      }

      case index_throw_statement:
      case index_assert_statement:
      case index_return_statement:
      case index_glvalue_to_prvalue:
      case index_push_immediate:
      case index_push_global_reference:
        // There is nothing to rebind.
        return nullopt;

      case index_push_local_reference: {
        const auto& altr = this->m_stor.as<index_push_local_reference>();

        // Get the context.
        // Don't bind references in analytic contexts.
        const Abstract_Context* qctx = &ctx;
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
        Analytic_Context ctx_func(&ctx, altr.params);
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
      case index_break_or_continue:
      case index_declare_reference:
      case index_initialize_reference:
        // There is nothing to rebind.
        return nullopt;

      default:
        ASTERIA_TERMINATE("invalid AIR node type (index `$1`)", this->index());
    }
  }

bool
AIR_Node::
solidify(AVMC_Queue& queue)
const
  {
    switch(this->index()) {
      case index_clear_stack:
        return do_solidify<AIR_Traits_clear_stack>(queue,
                                     this->m_stor.as<index_clear_stack>());

      case index_execute_block:
        return do_solidify<AIR_Traits_execute_block>(queue,
                                     this->m_stor.as<index_execute_block>());

      case index_declare_variable:
        return do_solidify<AIR_Traits_declare_variable>(queue,
                                     this->m_stor.as<index_declare_variable>());

      case index_initialize_variable:
        return do_solidify<AIR_Traits_initialize_variable>(queue,
                                     this->m_stor.as<index_initialize_variable>());

      case index_if_statement:
        return do_solidify<AIR_Traits_if_statement>(queue,
                                     this->m_stor.as<index_if_statement>());

      case index_switch_statement:
        return do_solidify<AIR_Traits_switch_statement>(queue,
                                     this->m_stor.as<index_switch_statement>());

      case index_do_while_statement:
        return do_solidify<AIR_Traits_do_while_statement>(queue,
                                     this->m_stor.as<index_do_while_statement>());

      case index_while_statement:
        return do_solidify<AIR_Traits_while_statement>(queue,
                                     this->m_stor.as<index_while_statement>());

      case index_for_each_statement:
        return do_solidify<AIR_Traits_for_each_statement>(queue,
                                     this->m_stor.as<index_for_each_statement>());

      case index_for_statement:
        return do_solidify<AIR_Traits_for_statement>(queue,
                                     this->m_stor.as<index_for_statement>());

      case index_try_statement:
        return do_solidify<AIR_Traits_try_statement>(queue,
                                     this->m_stor.as<index_try_statement>());

      case index_throw_statement:
        return do_solidify<AIR_Traits_throw_statement>(queue,
                                     this->m_stor.as<index_throw_statement>());

      case index_assert_statement:
        return do_solidify<AIR_Traits_assert_statement>(queue,
                                     this->m_stor.as<index_assert_statement>());

      case index_return_statement:
        return do_solidify<AIR_Traits_return_statement>(queue,
                                     this->m_stor.as<index_return_statement>());

      case index_glvalue_to_prvalue:
        return do_solidify<AIR_Traits_glvalue_to_prvalue>(queue,
                                     this->m_stor.as<index_glvalue_to_prvalue>());

      case index_push_immediate:
        return do_solidify<AIR_Traits_push_immediate>(queue,
                                     this->m_stor.as<index_push_immediate>());

      case index_push_global_reference:
        return do_solidify<AIR_Traits_push_global_reference>(queue,
                                     this->m_stor.as<index_push_global_reference>());

      case index_push_local_reference:
        return do_solidify<AIR_Traits_push_local_reference>(queue,
                                     this->m_stor.as<index_push_local_reference>());

      case index_push_bound_reference:
        return do_solidify<AIR_Traits_push_bound_reference>(queue,
                                     this->m_stor.as<index_push_bound_reference>());

      case index_define_function:
        return do_solidify<AIR_Traits_define_function>(queue,
                                     this->m_stor.as<index_define_function>());

      case index_branch_expression:
        return do_solidify<AIR_Traits_branch_expression>(queue,
                                     this->m_stor.as<index_branch_expression>());

      case index_coalescence:
        return do_solidify<AIR_Traits_coalescence>(queue,
                                     this->m_stor.as<index_coalescence>());

      case index_function_call:
        return do_solidify<AIR_Traits_function_call>(queue,
                                     this->m_stor.as<index_function_call>());

      case index_member_access:
        return do_solidify<AIR_Traits_member_access>(queue,
                                     this->m_stor.as<index_member_access>());

      case index_push_unnamed_array:
        return do_solidify<AIR_Traits_push_unnamed_array>(queue,
                                     this->m_stor.as<index_push_unnamed_array>());

      case index_push_unnamed_object:
        return do_solidify<AIR_Traits_push_unnamed_object>(queue,
                                     this->m_stor.as<index_push_unnamed_object>());

      case index_apply_operator: {
        const auto& altr = this->m_stor.as<index_apply_operator>();
        switch(altr.xop) {
          case xop_inc_post:
            return do_solidify<AIR_Traits_apply_operator_inc_post>(queue, altr);

          case xop_dec_post:
            return do_solidify<AIR_Traits_apply_operator_dec_post>(queue, altr);

          case xop_subscr:
            return do_solidify<AIR_Traits_apply_operator_subscr>(queue, altr);

          case xop_pos:
            return do_solidify<AIR_Traits_apply_operator_pos>(queue, altr);

          case xop_neg:
            return do_solidify<AIR_Traits_apply_operator_neg>(queue, altr);

          case xop_notb:
            return do_solidify<AIR_Traits_apply_operator_notb>(queue, altr);

          case xop_notl:
            return do_solidify<AIR_Traits_apply_operator_notl>(queue, altr);

          case xop_inc_pre:
            return do_solidify<AIR_Traits_apply_operator_inc_pre>(queue, altr);

          case xop_dec_pre:
            return do_solidify<AIR_Traits_apply_operator_dec_pre>(queue, altr);

          case xop_unset:
            return do_solidify<AIR_Traits_apply_operator_unset>(queue, altr);

          case xop_countof:
            return do_solidify<AIR_Traits_apply_operator_countof>(queue, altr);

          case xop_typeof:
            return do_solidify<AIR_Traits_apply_operator_typeof>(queue, altr);

          case xop_sqrt:
            return do_solidify<AIR_Traits_apply_operator_sqrt>(queue, altr);

          case xop_isnan:
            return do_solidify<AIR_Traits_apply_operator_isnan>(queue, altr);

          case xop_isinf:
            return do_solidify<AIR_Traits_apply_operator_isinf>(queue, altr);

          case xop_abs:
            return do_solidify<AIR_Traits_apply_operator_abs>(queue, altr);

          case xop_sign:
            return do_solidify<AIR_Traits_apply_operator_sign>(queue, altr);

          case xop_round:
            return do_solidify<AIR_Traits_apply_operator_round>(queue, altr);

          case xop_floor:
            return do_solidify<AIR_Traits_apply_operator_floor>(queue, altr);

          case xop_ceil:
            return do_solidify<AIR_Traits_apply_operator_ceil>(queue, altr);

          case xop_trunc:
            return do_solidify<AIR_Traits_apply_operator_trunc>(queue, altr);

          case xop_iround:
            return do_solidify<AIR_Traits_apply_operator_iround>(queue, altr);

          case xop_ifloor:
            return do_solidify<AIR_Traits_apply_operator_ifloor>(queue, altr);

          case xop_iceil:
            return do_solidify<AIR_Traits_apply_operator_iceil>(queue, altr);

          case xop_itrunc:
            return do_solidify<AIR_Traits_apply_operator_itrunc>(queue, altr);

          case xop_cmp_eq:
            return do_solidify<AIR_Traits_apply_operator_cmp_eq>(queue, altr);

          case xop_cmp_ne:
            return do_solidify<AIR_Traits_apply_operator_cmp_ne>(queue, altr);

          case xop_cmp_lt:
            return do_solidify<AIR_Traits_apply_operator_cmp_lt>(queue, altr);

          case xop_cmp_gt:
            return do_solidify<AIR_Traits_apply_operator_cmp_gt>(queue, altr);

          case xop_cmp_lte:
            return do_solidify<AIR_Traits_apply_operator_cmp_lte>(queue, altr);

          case xop_cmp_gte:
            return do_solidify<AIR_Traits_apply_operator_cmp_gte>(queue, altr);

          case xop_cmp_3way:
            return do_solidify<AIR_Traits_apply_operator_cmp_3way>(queue, altr);

          case xop_add:
            return do_solidify<AIR_Traits_apply_operator_add>(queue, altr);

          case xop_sub:
            return do_solidify<AIR_Traits_apply_operator_sub>(queue, altr);

          case xop_mul:
            return do_solidify<AIR_Traits_apply_operator_mul>(queue, altr);

          case xop_div:
            return do_solidify<AIR_Traits_apply_operator_div>(queue, altr);

          case xop_mod:
            return do_solidify<AIR_Traits_apply_operator_mod>(queue, altr);

          case xop_sll:
            return do_solidify<AIR_Traits_apply_operator_sll>(queue, altr);

          case xop_srl:
            return do_solidify<AIR_Traits_apply_operator_srl>(queue, altr);

          case xop_sla:
            return do_solidify<AIR_Traits_apply_operator_sla>(queue, altr);

          case xop_sra:
            return do_solidify<AIR_Traits_apply_operator_sra>(queue, altr);

          case xop_andb:
            return do_solidify<AIR_Traits_apply_operator_andb>(queue, altr);

          case xop_orb:
            return do_solidify<AIR_Traits_apply_operator_orb>(queue, altr);

          case xop_xorb:
            return do_solidify<AIR_Traits_apply_operator_xorb>(queue, altr);

          case xop_assign:
            return do_solidify<AIR_Traits_apply_operator_assign>(queue, altr);

          case xop_fma:
            return do_solidify<AIR_Traits_apply_operator_fma>(queue, altr);

          case xop_head:
            return do_solidify<AIR_Traits_apply_operator_head>(queue, altr);

          case xop_tail:
            return do_solidify<AIR_Traits_apply_operator_tail>(queue, altr);

          default:
            ASTERIA_TERMINATE("invalid operator type (xop `$1`)", altr.xop);
        }
      }

      case index_unpack_struct_array:
        return do_solidify<AIR_Traits_unpack_struct_array>(queue,
                                     this->m_stor.as<index_unpack_struct_array>());

      case index_unpack_struct_object:
        return do_solidify<AIR_Traits_unpack_struct_object>(queue,
                                     this->m_stor.as<index_unpack_struct_object>());

      case index_define_null_variable:
        return do_solidify<AIR_Traits_define_null_variable>(queue,
                                     this->m_stor.as<index_define_null_variable>());

      case index_single_step_trap:
        return do_solidify<AIR_Traits_single_step_trap>(queue,
                                     this->m_stor.as<index_single_step_trap>());

      case index_variadic_call:
        return do_solidify<AIR_Traits_variadic_call>(queue,
                                     this->m_stor.as<index_variadic_call>());

      case index_defer_expression:
        return do_solidify<AIR_Traits_defer_expression>(queue,
                                     this->m_stor.as<index_defer_expression>());

      case index_import_call:
        return do_solidify<AIR_Traits_import_call>(queue,
                                     this->m_stor.as<index_import_call>());

      case index_break_or_continue:
        return do_solidify<AIR_Traits_break_or_continue>(queue,
                                     this->m_stor.as<index_break_or_continue>());

      case index_declare_reference:
        return do_solidify<AIR_Traits_declare_reference>(queue,
                                     this->m_stor.as<index_declare_reference>());

      case index_initialize_reference:
        return do_solidify<AIR_Traits_initialize_reference>(queue,
                                     this->m_stor.as<index_initialize_reference>());

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
      case index_return_statement:
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
      case index_break_or_continue:
      case index_declare_reference:
      case index_initialize_reference:
        return callback;

      default:
        ASTERIA_TERMINATE("invalid AIR node type (index `$1`)", this->index());
    }
  }

}  // namespace asteria
