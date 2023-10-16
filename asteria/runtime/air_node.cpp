// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "air_node.hpp"
#include "enums.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "analytic_context.hpp"
#include "garbage_collector.hpp"
#include "random_engine.hpp"
#include "runtime_error.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "module_loader.hpp"
#include "air_optimizer.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../compiler/statement.hpp"
#include "../compiler/expression_unit.hpp"
#include "../llds/avmc_queue.hpp"
#include "../utils.hpp"
namespace asteria {
namespace {

bool&
do_rebind_nodes(bool& dirty, cow_vector<AIR_Node>& code, Abstract_Context& ctx)
  {
    for(size_t i = 0;  i < code.size();  ++i) {
      auto qnode = code[i].rebind_opt(ctx);
      if(qnode) {
        code.mut(i) = ::std::move(*qnode);
        dirty |= true;
      }
    }
    return dirty;
  }

bool&
do_rebind_nodes(bool& dirty, cow_vector<cow_vector<AIR_Node>>& seqs, Abstract_Context& ctx)
  {
    for(size_t k = 0;  k < seqs.size();  ++k) {
      for(size_t i = 0;  i < seqs[k].size();  ++i) {
        auto qnode = seqs[k][i].rebind_opt(ctx);
        if(qnode) {
          seqs.mut(k).mut(i) = ::std::move(*qnode);
          dirty |= true;
        }
      }
    }
    return dirty;
  }

template<typename NodeT>
opt<AIR_Node>
do_return_rebound_opt(bool dirty, NodeT&& xnode)
  {
    opt<AIR_Node> res;
    if(dirty)
      res.emplace(::std::forward<NodeT>(xnode));
    return res;
  }

bool
do_solidify_nodes(AVMC_Queue& queue, const cow_vector<AIR_Node>& code)
  {
    queue.clear();
    bool r = ::rocket::all_of(code, [&](const AIR_Node& node) { return node.solidify(queue);  });
    queue.finalize();
    return r;
  }

void
do_solidify_nodes(cow_vector<AVMC_Queue>& queue, const cow_vector<cow_vector<AIR_Node>>& code)
  {
    queue.resize(code.size());
    for(size_t k = 0;  k != code.size();  ++k)
      do_solidify_nodes(queue.mut(k), code.at(k));
  }

AIR_Status
do_evaluate_subexpression(Executive_Context& ctx, bool assign, const AVMC_Queue& queue)
  {
    // If the queue is empty, leave the condition on the top of the stack.
    if(queue.empty())
      return air_status_next;

    if(!assign) {
      // If this is not a compound assignment, discard the top which will be
      // overwritten anyway, then evaluate the subexpression. The status code
      // must be forwarded as is, because PTCs may return `air_status_return_ref`.
      ctx.stack().pop();
      return queue.execute(ctx);
    }

    // Evaluate the subexpression.
    AIR_Status status = queue.execute(ctx);
    ROCKET_ASSERT(status == air_status_next);
    auto value = ctx.stack().top().dereference_readonly();
    ctx.stack().pop();
    ctx.stack().top().dereference_mutable() = ::std::move(value);
    return air_status_next;
  }

AIR_Status
do_execute_block(const AVMC_Queue& queue, Executive_Context& ctx)
  {
    Executive_Context ctx_next(Executive_Context::M_plain(), ctx);
    AIR_Status status = air_status_next;
    try {
      status = queue.execute(ctx_next);
    }
    catch(Runtime_Error& except) {
      ctx_next.on_scope_exit_exceptional(except);
      throw;
    }
    ctx_next.on_scope_exit_normal(status);
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

template<typename ContainerT>
inline
void
do_collect_variables_for_each(ContainerT& cont, Variable_HashMap& staged,
                              Variable_HashMap& temp)
  {
    for(const auto& r : cont)
      r.collect_variables(staged, temp);
  }

template<size_t N>
struct Sparam_queues
  {
    AVMC_Queue queues[N];

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        do_collect_variables_for_each(this->queues, staged, temp);
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

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        do_collect_variables_for_each(this->queues_labels, staged, temp);
        do_collect_variables_for_each(this->queues_bodies, staged, temp);
      }
  };

struct Sparam_for_each
  {
    phsh_string name_key;
    phsh_string name_mapped;
    Source_Location sloc_init;
    AVMC_Queue queue_init;
    AVMC_Queue queue_body;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        this->queue_init.collect_variables(staged, temp);
        this->queue_body.collect_variables(staged, temp);
      }
  };

struct Sparam_try_catch
  {
    Source_Location sloc_try;
    AVMC_Queue queue_try;
    Source_Location sloc_catch;
    phsh_string name_except;
    AVMC_Queue queue_catch;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        this->queue_try.collect_variables(staged, temp);
        this->queue_catch.collect_variables(staged, temp);
      }
  };

struct Sparam_func
  {
    Compiler_Options opts;
    Source_Location sloc;
    cow_string func;
    cow_vector<phsh_string> params;
    cow_vector<AIR_Node> code_body;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        do_collect_variables_for_each(this->code_body, staged, temp);
      }
  };

struct Sparam_defer
  {
    Source_Location sloc;
    cow_vector<AIR_Node> code_body;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        do_collect_variables_for_each(this->code_body, staged, temp);
      }
  };

// These are traits for individual AIR node types.
// Each traits struct must contain the `execute()` function, and optionally,
// these functions: `make_uparam()`, `make_sparam()`, `get_symbols()`.

struct Traits_clear_stack
  {
    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx)
      {
        ctx.stack().clear();
        return air_status_next;
      }
  };

struct Traits_execute_block
  {
    static
    AVMC_Queue
    make_sparam(bool& reachable, const AIR_Node::S_execute_block& altr)
      {
        AVMC_Queue queue;
        reachable &= do_solidify_nodes(queue, altr.code_body);
        return queue;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue& queue)
      {
        return do_execute_block(queue, ctx);
      }
  };

struct Traits_declare_variable
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_declare_variable& altr)
      {
        return altr.sloc;
      }

    static
    Sparam_sloc_name
    make_sparam(bool& /*reachable*/, const AIR_Node::S_declare_variable& altr)
      {
        Sparam_sloc_name sp;
        sp.sloc = altr.sloc;
        sp.name = altr.name;
        return sp;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_sloc_name& sp)
      {
        // Allocate a variable and inject it into the current context.
        const auto gcoll = ctx.global().garbage_collector();
        const auto var = gcoll->create_variable();
        ctx.insert_named_reference(sp.name).set_variable(var);
        ASTERIA_CALL_GLOBAL_HOOK(ctx.global(), on_variable_declare, sp.sloc, sp.name);

        // Push a copy of the reference onto the stack. We will get it back
        // after its initializer finishes execution.
        ctx.stack().push().set_variable(var);
        return air_status_next;
      }
  };

struct Traits_initialize_variable
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_initialize_variable& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_initialize_variable& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.immutable;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this function.
        auto val = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();

        // Get the variable back.
        auto var = ctx.stack().top().unphase_variable_opt();
        ROCKET_ASSERT(var && !var->is_initialized());
        ctx.stack().pop();

        var->initialize(::std::move(val));
        var->set_immutable(up.u8v[0]);
        return air_status_next;
      }
  };

struct Traits_if_statement
  {
    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_if_statement& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.negative;
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

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const Sparam_queues_2& sp)
      {
        // Check the value of the condition.
        return (ctx.stack().top().dereference_readonly().test() != up.u8v[0])
                  ? do_execute_block(sp.queues[0], ctx)
                  : do_execute_block(sp.queues[1], ctx);
      }
  };

struct Traits_switch_statement
  {
    static
    Sparam_switch
    make_sparam(bool& /*reachable*/, const AIR_Node::S_switch_statement& altr)
      {
        Sparam_switch sp;
        do_solidify_nodes(sp.queues_labels, altr.code_labels);
        do_solidify_nodes(sp.queues_bodies, altr.code_bodies);
        sp.names_added = altr.names_added;
        return sp;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_switch& sp)
      {
        // Get the number of clauses.
        size_t nclauses = sp.queues_labels.size();
        ROCKET_ASSERT(nclauses == sp.queues_bodies.size());
        ROCKET_ASSERT(nclauses == sp.names_added.size());

        // Read the value of the condition and find the target clause for it.
        auto cond = ctx.stack().top().dereference_readonly();
        size_t target_index = SIZE_MAX;

        // This is different from the `switch` statement in C, where `case` labels must
        // have constant operands.
        for(size_t i = 0;  i < nclauses;  ++i) {
          // This is a `default` clause if the condition is empty, and a `case` clause
          // otherwise.
          if(sp.queues_labels[i].empty()) {
            target_index = i;
            continue;
          }

          // Evaluate the operand and check whether it equals `cond`.
          AIR_Status status = sp.queues_labels[i].execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
          if(ctx.stack().top().dereference_readonly().compare(cond) == compare_equal) {
            target_index = i;
            break;
          }
        }

        if(target_index >= nclauses)
          return air_status_next;

        // Skip this statement if no matching clause has been found.
        Executive_Context ctx_body(Executive_Context::M_plain(), ctx);
        AIR_Status status = air_status_next;
        try {
          for(size_t i = 0;  i < nclauses;  ++i)
            if(i < target_index) {
              // Inject bypassed variables into the scope.
              for(const auto& name : sp.names_added[i])
                ctx_body.insert_named_reference(name);
            }
            else {
              // Execute the body of this clause.
              status = sp.queues_bodies[i].execute(ctx_body);
              if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_switch })) {
                status = air_status_next;
                break;
              }
              else if(status != air_status_next)
                break;
            }
        }
        catch(Runtime_Error& except) {
          ctx_body.on_scope_exit_exceptional(except);
          throw;
        }
        ctx_body.on_scope_exit_normal(status);
        return air_status_next;
      }
  };

struct Traits_do_while_statement
  {
    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_do_while_statement& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.negative;
        return up;
      }

    static
    Sparam_queues_2
    make_sparam(bool& reachable, const AIR_Node::S_do_while_statement& altr)
      {
        Sparam_queues_2 sp;
        reachable &= do_solidify_nodes(sp.queues[0], altr.code_body);
        do_solidify_nodes(sp.queues[1], altr.code_cond);
        return sp;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const Sparam_queues_2& sp)
      {
        AIR_Status status = air_status_next;
        for(;;) {
          // Execute the body.
          status = do_execute_block(sp.queues[0], ctx);
          if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while })) {
            status = air_status_next;
            break;
          }
          else if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                                 air_status_continue_while }))
            break;

          // Check the condition.
          status = sp.queues[1].execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
          if(ctx.stack().top().dereference_readonly().test() == up.u8v[0])
            break;
        }
        return status;
      }
  };

struct Traits_while_statement
  {
    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_while_statement& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.negative;
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

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const Sparam_queues_2& sp)
      {
        AIR_Status status = air_status_next;
        for(;;) {
          // Check the condition.
          status = sp.queues[0].execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
          if(ctx.stack().top().dereference_readonly().test() == up.u8v[0])
            break;

          // Execute the body.
          status = do_execute_block(sp.queues[1], ctx);
          if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while })) {
            status = air_status_next;
            break;
          }
          else if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                                 air_status_continue_while }))
            break;
        }
        return status;
      }
  };

struct Traits_for_each_statement
  {
    static
    Sparam_for_each
    make_sparam(bool& /*reachable*/, const AIR_Node::S_for_each_statement& altr)
      {
        Sparam_for_each sp;
        sp.name_key = altr.name_key;
        sp.name_mapped = altr.name_mapped;
        sp.sloc_init = altr.sloc_init;
        do_solidify_nodes(sp.queue_init, altr.code_init);
        do_solidify_nodes(sp.queue_body, altr.code_body);
        return sp;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_for_each& sp)
      {
        // Get global interfaces.
        auto gcoll = ctx.global().garbage_collector();

        // We have to create an outer context due to the fact that the key and mapped
        // references outlast every iteration.
        Executive_Context ctx_for(Executive_Context::M_plain(), ctx);

        // Create key and mapped references.
        auto& key = ctx_for.insert_named_reference(sp.name_key);
        auto& mapped = ctx_for.insert_named_reference(sp.name_mapped);
        refcnt_ptr<Variable> kvar;

        // Evaluate the range initializer and set the range up, which isn't going to
        // change for all loops.
        AIR_Status status = sp.queue_init.execute(ctx_for);
        ROCKET_ASSERT(status == air_status_next);
        mapped = ::std::move(ctx_for.stack().mut_top());

        const auto range = mapped.dereference_readonly();
        switch(weaken_enum(range.type())) {
          case type_null:
            // Do nothing.
            break;

          case type_array: {
            const auto& arr = range.as_array();
            for(int64_t i = 0;  i < arr.ssize();  ++i) {
              // Set the key variable which is the subscript of the mapped element
              // in the array.
              if(!kvar) {
                kvar = gcoll->create_variable();
                key.set_variable(kvar);
              }
              else
                mapped.pop_modifier();

              kvar->initialize(i);
              kvar->set_immutable();

              Reference_Modifier::S_array_index xmod = { i };
              mapped.push_modifier(::std::move(xmod));
              mapped.dereference_readonly();

              // Execute the loop body.
              status = do_execute_block(sp.queue_body, ctx_for);
              if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
                status = air_status_next;
                break;
              }
              else if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                                     air_status_continue_for }))
                break;
            }
            break;
          }

          case type_object: {
            const auto& obj = range.as_object();
            for(auto it = obj.begin();  it != obj.end();  ++it) {
              // Set the key variable which is the name of the mapped element in
              // the object.
              if(!kvar) {
                kvar = gcoll->create_variable();
                key.set_variable(kvar);
              }
              else
                mapped.pop_modifier();

              kvar->initialize(it->first.rdstr());
              kvar->set_immutable();

              Reference_Modifier::S_object_key xmod = { it->first };
              mapped.push_modifier(::std::move(xmod));
              mapped.dereference_readonly();

              // Execute the loop body.
              status = do_execute_block(sp.queue_body, ctx_for);
              if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
                status = air_status_next;
                break;
              }
              else if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                                     air_status_continue_for }))
                break;

              // Restore the mapped reference.
              mapped.pop_modifier();
            }
            break;
          }

          default:
            ::rocket::tinyfmt_str fmt;
            format(fmt, "Range value not iterable (range `$1`)", range);
            Runtime_Error except(Runtime_Error::M_native(), fmt.extract_string());
            except.push_frame_plain(sp.sloc_init, sref(""));
            throw except;
        }
        return status;
      }
  };

struct Traits_for_statement
  {
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

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_queues_4& sp)
      {
        // This is the same as the `for` statement in C.
        // We have to create an outer context due to the fact that names declared in the
        // first segment outlast every iteration.
        Executive_Context ctx_for(Executive_Context::M_plain(), ctx);

        // Execute the loop initializer, which shall only be a definition or an expression
        // statement.
        AIR_Status status = sp.queues[0].execute(ctx_for);
        ROCKET_ASSERT(status == air_status_next);
        for(;;) {
          // Check the condition. There is a special case: If the condition is empty,
          // then the loop is infinite.
          status = sp.queues[1].execute(ctx_for);
          ROCKET_ASSERT(status == air_status_next);
          if(!ctx_for.stack().empty() && !ctx_for.stack().top().dereference_readonly().test())
            break;

          // Execute the body.
          status = do_execute_block(sp.queues[3], ctx_for);
          if(::rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
            status = air_status_next;
            break;
          }
          else if(::rocket::is_none_of(status, { air_status_next, air_status_continue_unspec,
                                                 air_status_continue_for }))
            break;

          // Execute the increment.
          status = sp.queues[2].execute(ctx_for);
          ROCKET_ASSERT(status == air_status_next);
        }
        return status;
      }
  };

struct Traits_try_statement
  {
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

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_try_catch& sp)
      try {
        // Execute the `try` block. If no exception is thrown, this will have
        // little overhead. This is almost identical to JavaScript.
        AIR_Status status = do_execute_block(sp.queue_try, ctx);
        if(status == air_status_return_ref)
          ctx.stack().mut_top().check_function_result(ctx.global());
        return status;
      }
      catch(Runtime_Error& except) {
        // Append a frame due to exit of the `try` clause.
        // Reuse the exception object. Don't bother allocating a new one.
        except.push_frame_try(sp.sloc_try);

        // This branch must be executed inside this `catch` block.
        // User-provided bindings may obtain the current exception using
        // `::std::current_exception`.
        Executive_Context ctx_catch(Executive_Context::M_plain(), ctx);
        AIR_Status status;
        try {
          // Set the exception reference.
          ctx_catch.insert_named_reference(sp.name_except)
              .set_temporary(except.value());

          // Set backtrace frames.
          V_array backtrace;
          for(size_t i = 0;  i < except.count_frames();  ++i) {
            const auto& f = except.frame(i);
            V_object r;

            r.try_emplace(sref("frame"), sref(f.what_type()));
            r.try_emplace(sref("file"), f.file());
            r.try_emplace(sref("line"), f.line());
            r.try_emplace(sref("column"), f.column());
            r.try_emplace(sref("value"), f.value());

            backtrace.emplace_back(::std::move(r));
          }

          ctx_catch.insert_named_reference(sref("__backtrace"))
              .set_temporary(::std::move(backtrace));

          // Execute the `catch` clause.
          status = sp.queue_catch.execute(ctx_catch);
        }
        catch(Runtime_Error& nested) {
          ctx_catch.on_scope_exit_exceptional(nested);
          nested.push_frame_catch(sp.sloc_catch, except.value());
          throw;
        }
        ctx_catch.on_scope_exit_normal(status);
        return status;
      }
  };

struct Traits_throw_statement
  {
    static
    Source_Location
    make_sparam(bool& reachable, const AIR_Node::S_throw_statement& altr)
      {
        reachable = false;
        return altr.sloc;
      }

    [[noreturn]] static
    AIR_Status
    execute(Executive_Context& ctx, const Source_Location& sloc)
      {
        // Read the value to throw.
        // Note that the operand must not have been empty for this code.
        throw Runtime_Error(Runtime_Error::M_throw(),
                 ctx.stack().top().dereference_readonly(), sloc);
      }
  };

struct Traits_assert_statement
  {
    static
    Sparam_sloc_text
    make_sparam(bool& /*reachable*/, const AIR_Node::S_assert_statement& altr)
      {
        Sparam_sloc_text sp;
        sp.sloc = altr.sloc;
        sp.text = altr.msg;
        return sp;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_sloc_text& sp)
      {
        // Check the value of the condition.
        // When the assertion succeeds, there is nothing to do.
        if(ROCKET_EXPECT(ctx.stack().top().dereference_readonly().test()))
          return air_status_next;

        // Throw an exception if the assertion fails.
        // This cannot be disabled.
        throw Runtime_Error(Runtime_Error::M_assert(), sp.sloc, sp.text);
      }
  };

struct Traits_simple_status
  {
    static
    AVMC_Queue::Uparam
    make_uparam(bool& reachable, const AIR_Node::S_simple_status& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = weaken_enum(altr.status);
        reachable = altr.status != air_status_next;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& /*ctx*/, AVMC_Queue::Uparam up)
      {
        AIR_Status status = static_cast<AIR_Status>(up.u8v[0]);
        ROCKET_ASSERT(status != air_status_next);
        return status;
      }
  };

struct Traits_check_argument
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_check_argument& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_check_argument& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.by_ref;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        if(up.u8v[0]) {
          // The argument is passed by reference, but it always has to be
          // dereferenceable.
          ctx.stack().top().dereference_readonly();
          return air_status_next;
        }

        // The argument is passed by copy, so convert it to a temporary.
        ctx.stack().mut_top().dereference_copy();
        return air_status_next;
      }
  };

struct Traits_push_global_reference
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_push_global_reference& altr)
      {
        return altr.sloc;
      }

    static
    phsh_string
    make_sparam(bool& /*reachable*/, const AIR_Node::S_push_global_reference& altr)
      {
        return altr.name;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, phsh_stringR name)
      {
        // Look for the name in the global context.
        auto qref = ctx.global().get_named_reference_opt(name);
        if(!qref)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Unresolvable global identifier `$1`"),
              name);

        ctx.stack().push() = *qref;
        return air_status_next;
      }
  };

struct Traits_push_local_reference
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_push_local_reference& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_push_local_reference& altr)
      {
        AVMC_Queue::Uparam up;
        up.u32 = altr.depth;
        return up;
      }

    static
    phsh_string
    make_sparam(bool& /*reachable*/, const AIR_Node::S_push_local_reference& altr)
      {
        return altr.name;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, phsh_stringR name)
      {
        // Get the context.
        Executive_Context* qctx = &ctx;
        for(uint32_t k = 0;  k != up.u32;  ++k)
          qctx = qctx->get_parent_opt();

        // Look for the name in the context.
        auto qref = qctx->get_named_reference_opt(name);
        if(!qref)
          ASTERIA_THROW_RUNTIME_ERROR(("Undeclared identifier `$1`"), name);

        // Check if control flow has bypassed its initialization.
        if(qref->is_invalid())
          ASTERIA_THROW_RUNTIME_ERROR(("Use of bypassed variable or reference `$1`"), name);

        ctx.stack().push() = *qref;
        return air_status_next;
      }
  };

struct Traits_push_bound_reference
  {
    static
    Reference
    make_sparam(bool& /*reachable*/, const AIR_Node::S_push_bound_reference& altr)
      {
        return altr.ref;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Reference& ref)
      {
        ctx.stack().push() = ref;
        return air_status_next;
      }
  };

struct Traits_define_function
  {
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
        AIR_Optimizer optmz(sp.opts);
        optmz.rebind(&ctx, sp.params, sp.code_body);
        ctx.stack().push().set_temporary(optmz.create_function(sp.sloc, sp.func));
        return air_status_next;
      }
  };

struct Traits_branch_expression
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_branch_expression& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_branch_expression& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
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

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const Sparam_queues_2& sp)
      {
        // Check the value of the condition.
        return ctx.stack().top().dereference_readonly().test()
                    ? do_evaluate_subexpression(ctx, up.u8v[0], sp.queues[0])
                    : do_evaluate_subexpression(ctx, up.u8v[0], sp.queues[1]);
      }
  };

struct Traits_coalescence
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_coalescence& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_coalescence& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
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

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const AVMC_Queue& queue)
      {
        // Check the value of the condition.
        return ctx.stack().top().dereference_readonly().is_null()
                    ? do_evaluate_subexpression(ctx, up.u8v[0], queue)
                    : air_status_next;
      }
  };

AIR_Status
do_invoke_nontail(Reference& self, const Source_Location& sloc, const cow_function& target,
                  Global_Context& global, Reference_Stack&& stack)
  {
    ASTERIA_CALL_GLOBAL_HOOK(global, on_function_call, sloc, target);
    try {
      target.invoke(self, global, ::std::move(stack));
    }
    catch(Runtime_Error& except) {
      ASTERIA_CALL_GLOBAL_HOOK(global, on_function_except, sloc, target, except);
      throw;
    }
    ASTERIA_CALL_GLOBAL_HOOK(global, on_function_return, sloc, target, self);
    return air_status_next;
  }

AIR_Status
do_invoke_tail(Reference& self, const Source_Location& sloc, const cow_function& target,
               PTC_Aware ptc, Reference_Stack&& stack)
  {
    // Pack arguments for this PTC, and return `air_status_return_ref` to allow
    // the result to be unpacked outside; otherwise a null reference is returned
    // instead of this PTC wrapper, which can then never be unpacked.
    stack.push() = ::std::move(self);
    self.set_ptc(::rocket::make_refcnt<PTC_Arguments>(sloc, ptc, target, ::std::move(stack)));
    return air_status_return_ref;
  }

void
do_pop_positional_arguments(Reference_Stack& alt_stack, Reference_Stack& stack, size_t count)
  {
    alt_stack.clear();
    ROCKET_ASSERT(count <= stack.size());
    for(size_t k = count - 1;  k != SIZE_MAX;  --k)
      alt_stack.push() = ::std::move(stack.mut_top(k));
    stack.pop(count);
  }

struct Traits_function_call
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_function_call& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& reachable, const AIR_Node::S_function_call& altr)
      {
        AVMC_Queue::Uparam up;
        up.u32 = altr.nargs;
        up.u8v[0] = static_cast<uint8_t>(altr.ptc);
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
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const Source_Location& sloc)
      {
        const auto sentry = ctx.global().copy_recursion_sentry();
        ASTERIA_CALL_GLOBAL_HOOK(ctx.global(), on_single_step_trap, sloc);

        // Pop arguments off the stack backwards.
        auto& alt_stack = ctx.alt_stack();
        auto& stack = ctx.stack();
        do_pop_positional_arguments(alt_stack, stack, up.u32);

        // Copy the target, which shall be of type `function`.
        auto value = stack.top().dereference_readonly();
        if(value.is_null())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Target function not found"));
        else if(!value.is_function())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Target value not a function (value `$1`)"),
              value);

        const auto& target = value.as_function();
        auto& self = stack.mut_top().pop_modifier();
        const auto ptc = static_cast<PTC_Aware>(up.u8v[0]);

        stack.clear_cache();
        alt_stack.clear_cache();

        return ROCKET_EXPECT(ptc == ptc_aware_none)
                 ? do_invoke_nontail(self, sloc, target, ctx.global(), ::std::move(alt_stack))
                 : do_invoke_tail(self, sloc, target, ptc, ::std::move(alt_stack));
      }
  };

struct Traits_member_access
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_member_access& altr)
      {
        return altr.sloc;
      }

    static
    phsh_string
    make_sparam(bool& /*reachable*/, const AIR_Node::S_member_access& altr)
      {
        return altr.name;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, phsh_stringR name)
      {
        Reference_Modifier::S_object_key xmod = { name };
        ctx.stack().mut_top().push_modifier(::std::move(xmod));

        ctx.stack().top().dereference_readonly();
        return air_status_next;
      }
  };

struct Traits_push_unnamed_array
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_push_unnamed_array& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_push_unnamed_array& altr)
      {
        AVMC_Queue::Uparam up;
        up.u32 = altr.nelems;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // Pop elements from the stack and store them in an array backwards.
        V_array array;
        array.resize(up.u32);
        for(auto it = array.mut_rbegin();  it != array.rend();  ++it) {
          *it = ctx.stack().top().dereference_readonly();
          ctx.stack().pop();
        }

        // Push the array as a temporary.
        ctx.stack().push().set_temporary(::std::move(array));
        return air_status_next;
      }
  };

struct Traits_push_unnamed_object
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_push_unnamed_object& altr)
      {
        return altr.sloc;
      }

    static
    cow_vector<phsh_string>
    make_sparam(bool& /*reachable*/, const AIR_Node::S_push_unnamed_object& altr)
      {
        return altr.keys;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const cow_vector<phsh_string>& keys)
      {
        // Pop elements from the stack and store them in an object backwards.
        V_object object;
        object.reserve(keys.size());
        for(auto it = keys.rbegin();  it != keys.rend();  ++it) {
          // Use `try_emplace()` instead of `insert_or_assign()`. In case of duplicate keys,
          // the last value takes precedence.
          object.try_emplace(*it, ctx.stack().top().dereference_readonly());
          ctx.stack().pop();
        }

        // Push the object as a temporary.
        ctx.stack().push().set_temporary(::std::move(object));
        return air_status_next;
      }
  };

struct Traits_return_statement
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_return_statement& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& reachable, const AIR_Node::S_return_statement& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.by_ref;
        up.u8v[1] = altr.is_void;
        reachable = false;
        return up;
      }


    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        if(up.u8v[1] || ctx.stack().top().is_void())
          return air_status_return_void;

        if(up.u8v[0]) {
          // The result is passed by reference, but it always has to be
          // dereferenceable.
          ctx.stack().top().dereference_readonly();
          return air_status_return_ref;
        }

        // The result is passed by copy, so convert it to a temporary.
        ctx.stack().mut_top().dereference_copy();
        return air_status_return_ref;
      }
  };

struct Traits_apply_xop_inc
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary. `assign` is `true` for the postfix variant
        // and `false` for the prefix variant.
        auto& top = ctx.stack().mut_top();
        auto& lhs = top.dereference_mutable();

        if(lhs.type() == type_integer) {
          V_integer& val = lhs.mut_integer();

          // Increment the value with overflow checking.
          int64_t result;
          if(ROCKET_ADD_OVERFLOW(val, 1, &result))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Integer increment overflow (operand was `$1`)"),
                val);

          if(up.u8v[0])
            top.set_temporary(val);

          val = result;
          return air_status_next;
        }
        else if(lhs.type() == type_real) {
          V_real& val = lhs.mut_real();

          // Overflow will result in an infinity, so this is safe.
          double result = val + 1;

          if(up.u8v[0])
            top.set_temporary(val);

          val = result;
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Postfix increment not applicable (operand was `$1`)"),
              lhs);
      }
  };

struct Traits_apply_xop_dec
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary. `assign` is `true` for the postfix variant
        // and `false` for the prefix variant.
        auto& top = ctx.stack().mut_top();
        auto& lhs = top.dereference_mutable();

        if(lhs.type() == type_integer) {
          V_integer& val = lhs.mut_integer();

          // Decrement the value with overflow checking.
          int64_t result;
          if(ROCKET_SUB_OVERFLOW(val, 1, &result))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Integer decrement overflow (operand was `$1`)"),
                val);

          if(up.u8v[0])
            top.set_temporary(val);

          val = result;
          return air_status_next;
        }
        else if(lhs.type() == type_real) {
          V_real& val = lhs.mut_real();

          // Overflow will result in an infinity, so this is safe.
          double result = val - 1;

          if(up.u8v[0])
            top.set_temporary(val);

          val = result;
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Postfix decrement not applicable (operand was `$1`)"),
              lhs);
      }
  };

struct Traits_apply_xop_subscr
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();

        if(rhs.type() == type_integer) {
          // Push an array subscript.
          Reference_Modifier::S_array_index xmod = { rhs.as_integer() };
          top.push_modifier(::std::move(xmod));
          top.dereference_readonly();
          return air_status_next;
        }
        else if(rhs.type() == type_string) {
          // Push an object subscript.
          Reference_Modifier::S_object_key xmod = { rhs.as_string() };
          top.push_modifier(::std::move(xmod));
          top.dereference_readonly();
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Subscript not valid (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_pos
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // There is nothing to do.
        (void) rhs;
        return air_status_next;
      }
  };

struct Traits_apply_xop_neg
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          V_integer& val = rhs.mut_integer();

          // Negate the value with overflow checks.
          int64_t result;
          if(ROCKET_SUB_OVERFLOW(0, val, &result))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Integer negation overflow (operand was `$1`)"),
                val);

          val = result;
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          V_real& val = rhs.mut_real();

          // Flip the sign bit of the value.
          int64_t bits;
          ::memcpy(&bits, &val, sizeof(val));
          bits ^= INT64_MIN;

          ::memcpy(&val, &bits, sizeof(val));
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Arithmetic negation not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_notb
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_boolean) {
          V_boolean& val = rhs.mut_boolean();

          // Flip the bit.
          val = !val;
          return air_status_next;
        }
        else if(rhs.type() == type_integer) {
          V_integer& val = rhs.mut_integer();

          // Flip all bits.
          val = ~val;
          return air_status_next;
        }
        else if(rhs.type() == type_string) {
          V_string& val = rhs.mut_string();

          // Flip all bits in every byte of the string.
          for(auto it = val.mut_begin();  it != val.end();  ++it)
            *it = (char) ~*it;
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Bitwise NOT not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_notl
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Perform logical NOT operation on the operand.
        rhs = !rhs.test();
        return air_status_next;
      }
  };

struct Traits_apply_xop_unset
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();

        // Unset the last element and return it as a temporary.
        auto val = top.dereference_unset();
        top.set_temporary(::std::move(val));
        return air_status_next;
      }
  };

struct Traits_apply_xop_countof
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_null) {
          // `countof null` is defined to be zero.
          rhs = V_integer(0);
          return air_status_next;
        }
        else if(rhs.type() == type_string) {
          // Yield the number of bytes in the stirng.
          rhs = V_integer(rhs.as_string().size());
          return air_status_next;
        }
        else if(rhs.type() == type_array) {
          // Yield the number of elements in the array.
          rhs = V_integer(rhs.as_array().size());
          return air_status_next;
        }
        else if(rhs.type() == type_object) {
          // Yield the number of key-value pairs in the object.
          rhs = V_integer(rhs.as_object().size());
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`countof` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_typeof
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Yield the type name of the operand, as a string.
        rhs = V_string(::rocket::sref(describe_type(rhs.type())));
        return air_status_next;
      }
  };

struct Traits_apply_xop_sqrt
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.is_real()) {
          // Yield the arithmetic square root of the operand. Integers are
          // converted to real numbers.
          rhs = ::std::sqrt(rhs.as_real());
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__sqrt` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_isnan
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is never a NaN.
          rhs = false;
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Checks whether the operand is a NaN.
          rhs = ::std::isnan(rhs.as_real());
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__isnan` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_isinf
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is never an infinity.
          rhs = false;
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Checks whether the operand is an infinity.
          rhs = ::std::isinf(rhs.as_real());
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__isinf` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_abs
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          V_integer& val = rhs.mut_integer();

          // Negate of the operand with overflow checks.
          V_integer neg_val;
          if(ROCKET_SUB_OVERFLOW(0, val, &neg_val))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Integer negation overflow (operand was `$1`)"),
                val);

          val ^= (val ^ neg_val) & (val >> 63);
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          V_real& val = rhs.mut_real();

          // Clear the sign bit of the operand.
          double result = ::std::fabs(val);

          val = result;
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__abs` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_sign
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // Get the sign bit.
          rhs = rhs.as_integer() < 0;
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Get the sign bit.
          rhs = ::std::signbit(rhs.as_real());
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__sign` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_round
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is always exact.
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Round the operand to the nearest integer.
          rhs.mut_real() = ::std::round(rhs.as_real());
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__round` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_floor
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is always exact.
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Round the operand to the nearest integer towards negative infinity.
          rhs.mut_real() = ::std::floor(rhs.as_real());
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__floor` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_ceil
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is always exact.
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Round the operand to the nearest integer towards positive infinity.
          rhs.mut_real() = ::std::ceil(rhs.as_real());
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__ceil` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_trunc
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is always exact.
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Truncate the operand to the nearest integer towards zero.
          rhs.mut_real() = ::std::trunc(rhs.as_real());
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__trunc` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_iround
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is always exact.
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Round the operand to the nearest integer.
          rhs = safe_double_to_int64(::std::round(rhs.as_real()));
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__iround` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_ifloor
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is always exact.
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Round the operand to the nearest integer towards negative infinity.
          rhs = safe_double_to_int64(::std::floor(rhs.as_real()));
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__ifloor` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_iceil
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is always exact.
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Round the operand to the nearest integer towards positive infinity.
          rhs = safe_double_to_int64(::std::ceil(rhs.as_real()));
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__iceil` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_itrunc
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          // An integer is always exact.
          return air_status_next;
        }
        else if(rhs.type() == type_real) {
          // Truncate the operand to the nearest integer towards zero.
          rhs = safe_double_to_int64(::std::trunc(rhs.as_real()));
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__itrunc` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_cmp_eq
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Check whether the two operands are equal. Unordered values are
        // considered to compare unequal.
        lhs = lhs.compare(rhs) == compare_equal;
        return air_status_next;
      }
  };

struct Traits_apply_xop_cmp_ne
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Check whether the two operands are not equal. Unordered values
        // are considered to compare unequal.
        lhs = lhs.compare(rhs) != compare_equal;
        return air_status_next;
      }
  };

struct Traits_apply_xop_cmp_lt
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Check whether the LHS operand is less than the RHS operand. If
        // they are unordered, an exception shall be thrown.
        Compare cmp = lhs.compare(rhs);
        if(cmp == compare_unordered)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Values not comparable (operands were `$1` and `$2`)"),
              lhs, rhs);

        lhs = cmp == compare_less;
        return air_status_next;
      }
  };

struct Traits_apply_xop_cmp_gt
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Check whether the LHS operand is greater than the RHS operand. If
        // they are unordered, an exception shall be thrown.
        Compare cmp = lhs.compare(rhs);
        if(cmp == compare_unordered)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Values not comparable (operands were `$1` and `$2`)"),
              lhs, rhs);

        lhs = cmp == compare_greater;
        return air_status_next;
      }
  };

struct Traits_apply_xop_cmp_lte
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Check whether the LHS operand is less than or equal to the RHS
        // operand. If they are unordered, an exception shall be thrown.
        Compare cmp = lhs.compare(rhs);
        if(cmp == compare_unordered)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Values not comparable (operands were `$1` and `$2`)"),
              lhs, rhs);

        lhs = cmp != compare_greater;
        return air_status_next;
      }
  };

struct Traits_apply_xop_cmp_gte
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Check whether the LHS operand is greater than or equal to the RHS
        // operand. If they are unordered, an exception shall be thrown.
        Compare cmp = lhs.compare(rhs);
        if(cmp == compare_unordered)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Values not comparable (operands were `$1` and `$2`)"),
              lhs, rhs);

        lhs = cmp != compare_less;
        return air_status_next;
      }
  };

struct Traits_apply_xop_cmp_3way
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Perform 3-way comparison of both operands.
        Compare cmp = lhs.compare(rhs);
        if(cmp == compare_unordered)
          lhs = sref("[unordered]");
        else {
          V_integer r = -1;
          r += cmp != compare_less;
          r += cmp == compare_greater;
          lhs = r;
        }
        return air_status_next;
      }
  };

struct Traits_apply_xop_cmp_un
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        // Check whether the two operands are unordered.
        lhs = lhs.compare(rhs) == compare_unordered;
        return air_status_next;
      }
  };

struct Traits_apply_xop_add
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_boolean() && rhs.is_boolean()) {
          V_boolean& val = lhs.mut_boolean();
          V_boolean other = rhs.as_boolean();

          // Perform logical OR of the operands.
          val |= other;
          return air_status_next;
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform arithmetic addition with overflow checking.
          int64_t result;
          if(ROCKET_ADD_OVERFLOW(val, other, &result))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Integer addition overflow (operands were `$1` and `$2`)"),
                val, other);

          val = result;
          return air_status_next;
        }
        else if(lhs.is_real() && rhs.is_real()) {
          V_real& val = lhs.mut_real();
          V_real other = rhs.as_real();

          // Overflow will result in an infinity, so this is safe.
          val += other;
          return air_status_next;
        }
        else if(lhs.is_string() && rhs.is_string()) {
          V_string& val = lhs.mut_string();
          const V_string& other = rhs.as_string();

          // Concatenate the two strings.
          val.append(other);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Addition not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_sub
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_boolean() && rhs.is_boolean()) {
          V_boolean& val = lhs.mut_boolean();
          V_boolean other = rhs.as_boolean();

          // Perform logical XOR of the operands.
          val ^= other;
          return air_status_next;
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform arithmetic subtraction with overflow checking.
          int64_t result;
          if(ROCKET_SUB_OVERFLOW(val, other, &result))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Integer subtraction overflow (operands were `$1` and `$2`)"),
                val, other);

          val = result;
          return air_status_next;
        }
        else if(lhs.is_real() && rhs.is_real()) {
          V_real& val = lhs.mut_real();
          V_real other = rhs.as_real();

          // Overflow will result in an infinity, so this is safe.
          val -= other;
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Subtraction not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_mul
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    template<typename ContainerT>
    static
    void
    do_duplicate_sequence_common(ContainerT& container, int64_t count)
      {
        if(count < 0)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Negative duplication count (operand was `$2`)"),
              count);

        if(container.empty() || (count == 1))
          return;

        if(count == 0) {
          container.clear();
          return;
        }

        // Calculate the result length with overflow checking.
        int64_t rlength;
        if(ROCKET_MUL_OVERFLOW((int64_t) container.size(), count, &rlength) || (rlength > PTRDIFF_MAX))
          ASTERIA_THROW_RUNTIME_ERROR((
              "Data length overflow (`$1` * `$2` > `$3`)"),
              container.size(), count, PTRDIFF_MAX);

        // Duplicate elements, using binary exponential backoff.
        while(container.ssize() < rlength)
          container.append(container.begin(),
                container.begin() + ::rocket::min(rlength - container.ssize(), container.ssize()));
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_boolean() && rhs.is_boolean()) {
          V_boolean& val = lhs.mut_boolean();
          V_boolean other = rhs.as_boolean();

          // Perform logical AND of the operands.
          val &= other;
          return air_status_next;
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform arithmetic multiplication with overflow checking.
          int64_t result;
          if(ROCKET_MUL_OVERFLOW(val, other, &result))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Integer multiplication overflow (operands were `$1` and `$2`)"),
                val, other);

          val = result;
          return air_status_next;
        }
        else if(lhs.is_real() && rhs.is_real()) {
          V_real& val = lhs.mut_real();
          V_real other = rhs.as_real();

          // Overflow will result in an infinity, so this is safe.
          val *= other;
          return air_status_next;
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          V_string& val = lhs.mut_string();
          V_integer count = rhs.as_integer();

          // Duplicate the string.
          do_duplicate_sequence_common(val, count);
          return air_status_next;
        }
        else if(lhs.is_integer() && rhs.is_string()) {
          V_integer count = lhs.as_integer();
          lhs = rhs.as_string();
          V_string& val = lhs.mut_string();

          // Duplicate the string.
          do_duplicate_sequence_common(val, count);
          return air_status_next;
        }
        else if(lhs.is_array() && rhs.is_integer()) {
          V_array& val = lhs.mut_array();
          V_integer count = rhs.as_integer();

          // Duplicate the array.
          do_duplicate_sequence_common(val, count);
          return air_status_next;
        }
        else if(lhs.is_integer() && rhs.is_array()) {
          V_integer count = lhs.as_integer();
          lhs = rhs.as_array();
          V_array& val = lhs.mut_array();

          // Duplicate the array.
          do_duplicate_sequence_common(val, count);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Multiplication not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_div
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // The divisor shall not be zero. Also, dividing `INT64_MIN` by `-1`
          // would cause the quotient to overflow, which shall be prevented.
          if(other == 0)
            ASTERIA_THROW_RUNTIME_ERROR((
                "Zero as divisor (operands were `$1` and `$2`)"),
                val, other);

          if((val == INT64_MIN) && (other == -1))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Integer division overflow (operands were `$1` and `$2`)"),
                val, other);

          val /= other;
          return air_status_next;
        }
        else if(lhs.is_real() && rhs.is_real()) {
          V_real& val = lhs.mut_real();
          V_real other = rhs.as_real();

          // This is always correct for floating-point numbers.
          val /= other;
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Division not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_mod
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // The divisor shall not be zero. Also, dividing `INT64_MIN` by `-1`
          // would cause the quotient to overflow, which shall be prevented.
          if(other == 0)
            ASTERIA_THROW_RUNTIME_ERROR((
                "Zero as divisor (operands were `$1` and `$2`)"),
                val, other);

          if((val == INT64_MIN) && (other == -1))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Integer division overflow (operands were `$1` and `$2`)"),
                val, other);

          val %= other;
          return air_status_next;
        }
        else if(lhs.is_real() && rhs.is_real()) {
          V_real& val = lhs.mut_real();
          V_real other = rhs.as_real();

          // This is always correct for floating-point numbers.
          val = ::std::fmod(val, other);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Modulo not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
   };

struct Traits_apply_xop_sll
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() != type_integer)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Invalid shift count (operands were `$1` and `$2`)"),
              lhs, rhs);

        if(rhs.as_integer() < 0)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Negative shift count (operands were `$1` and `$2`)"),
              lhs, rhs);

        if(lhs.is_integer()) {
          V_integer& val = lhs.mut_integer();

          // Shift the operand to the left by `count` bits.
          int64_t count = rhs.as_integer();
          val = (int64_t) ((uint64_t) val << (count & 63));
          val &= ((count - 64) >> 63);
          return air_status_next;
        }
        else if(lhs.is_string()) {
          V_string& val = lhs.mut_string();

          // Remove characters from the left and insert spaces in the right. The
          // length of the string is unchanged.
          size_t tlen = ::rocket::min((size_t) rhs.as_integer(), val.size());
          val.erase(0, tlen);
          val.append(tlen, ' ');
          return air_status_next;
        }
        else if(lhs.is_array()) {
          V_array& val = lhs.mut_array();

          // Remove elements from the left and insert nulls in the right. The
          // length of the array is unchanged.
          size_t tlen = ::rocket::min((size_t) rhs.as_integer(), val.size());
          val.erase(0, tlen);
          val.append(tlen);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Logical left shift not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_srl
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() != type_integer)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Invalid shift count (operands were `$1` and `$2`)"),
              lhs, rhs);

        if(rhs.as_integer() < 0)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Negative shift count (operands were `$1` and `$2`)"),
              lhs, rhs);

        if(lhs.is_integer()) {
          V_integer& val = lhs.mut_integer();

          // Shift the operand to the right by `count` bits.
          int64_t count = rhs.as_integer();
          val = (int64_t) ((uint64_t) val >> (count & 63));
          val &= ((count - 64) >> 63);
          return air_status_next;
        }
        else if(lhs.is_string()) {
          V_string& val = lhs.mut_string();

          // Remove characters from the right and insert spaces in the left. The
          // length of the string is unchanged.
          size_t tlen = ::rocket::min((size_t) rhs.as_integer(), val.size());
          val.pop_back(tlen);
          val.insert(0, tlen, ' ');
          return air_status_next;
        }
        else if(lhs.is_array()) {
          V_array& val = lhs.mut_array();

          // Remove elements from the right and insert nulls in the left. The
          // length of the array is unchanged.
          size_t tlen = ::rocket::min((size_t) rhs.as_integer(), val.size());
          val.pop_back(tlen);
          val.insert(0, tlen);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Logical right shift not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_sla
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() != type_integer)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Invalid shift count (operands were `$1` and `$2`)"),
              lhs, rhs);

        if(rhs.as_integer() < 0)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Negative shift count (operands were `$1` and `$2`)"),
              lhs, rhs);

        if(lhs.is_integer()) {
          V_integer& val = lhs.mut_integer();

          // Shift the operand to the left by `count` bits, with overflow checking.
          int64_t count = ::rocket::min(rhs.as_integer(), 63);
          if((val != 0) && ((count != rhs.as_integer()) || (((val >> 63) ^ val) >> (63 - count) != 0)))
            ASTERIA_THROW_RUNTIME_ERROR((
                "Arithmetic left shift overflow (operands were `$1` and `$2`)"),
                lhs, rhs);

          val <<= count;
          return air_status_next;
        }
        else if(lhs.is_string()) {
          V_string& val = lhs.mut_string();

          // Insert spaces in the right.
          size_t tlen = ::rocket::min((size_t) rhs.as_integer(), val.size());
          val.append(tlen, ' ');
          return air_status_next;
        }
        else if(lhs.is_array()) {
          V_array& val = lhs.mut_array();

          // Insert nulls in the right.
          size_t tlen = ::rocket::min((size_t) rhs.as_integer(), val.size());
          val.append(tlen);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Arithmetic left shift not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_sra
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() != type_integer)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Invalid shift count (operands were `$1` and `$2`)"),
              lhs, rhs);

        if(rhs.as_integer() < 0)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Negative shift count (operands were `$1` and `$2`)"),
              lhs, rhs);

        if(lhs.is_integer()) {
          V_integer& val = lhs.mut_integer();

          // Shift the operand to the right by `count` bits.
          int64_t count = ::rocket::min(rhs.as_integer(), 63);
          val >>= count;
          return air_status_next;
        }
        else if(lhs.is_string()) {
          V_string& val = lhs.mut_string();

          // Remove characters from the right.
          size_t tlen = ::rocket::min((size_t) rhs.as_integer(), val.size());
          val.pop_back(tlen);
          return air_status_next;
        }
        else if(lhs.is_array()) {
          V_array& val = lhs.mut_array();

          // Remove elements from the right.
          size_t tlen = ::rocket::min((size_t) rhs.as_integer(), val.size());
          val.pop_back(tlen);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Arithmetic right shift not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_andb
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_boolean() && rhs.is_boolean()) {
          V_boolean& val = lhs.mut_boolean();
          V_boolean other = rhs.as_boolean();

          // Perform logical AND of the operands.
          val &= other;
          return air_status_next;
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform bitwise AND of the operands.
          val &= other;
          return air_status_next;
        }
        else if(lhs.is_string() && rhs.is_string()) {
          V_string& val = lhs.mut_string();
          const V_string& mask = rhs.as_string();

          // Perform bitwise AND of all bytes of the operands. The result string
          // has the same length with the shorter one.
          if(val.size() > mask.size())
            val.erase(mask.size());
          auto maskp = mask.begin();
          for(auto it = val.mut_begin();  it != val.end();  ++it, ++maskp)
            *it = (char) (*it & *maskp);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Bitwise AND not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_orb
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_boolean() && rhs.is_boolean()) {
          V_boolean& val = lhs.mut_boolean();
          V_boolean other = rhs.as_boolean();

          // Perform logical OR of the operands.
          val |= other;
          return air_status_next;
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform bitwise OR of the operands.
          val |= other;
          return air_status_next;
        }
        else if(lhs.is_string() && rhs.is_string()) {
          V_string& val = lhs.mut_string();
          const V_string& mask = rhs.as_string();

          // Perform bitwise OR of all bytes of the operands. The result string
          // has the same length with the longer one.
          if(val.size() < mask.size())
            val.append(mask.size() - val.size(), 0);
          auto valp = val.mut_begin();
          for(auto it = mask.begin();  it != mask.end();  ++it, ++valp)
            *valp = (char) (*valp | *it);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Bitwise OR not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_xorb
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_boolean() && rhs.is_boolean()) {
          V_boolean& val = lhs.mut_boolean();
          V_boolean other = rhs.as_boolean();

          // Perform logical XOR of the operands.
          val ^= other;
          return air_status_next;
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform bitwise XOR of the operands.
          val ^= other;
          return air_status_next;
        }
        else if(lhs.is_string() && rhs.is_string()) {
          V_string& val = lhs.mut_string();
          const V_string& mask = rhs.as_string();

          // Perform bitwise XOR of all bytes of the operands. The result string
          // has the same length with the longer one.
          if(val.size() < mask.size())
            val.append(mask.size() - val.size(), 0);
          auto valp = val.mut_begin();
          for(auto it = mask.begin();  it != mask.end();  ++it, ++valp)
            *valp = (char) (*valp ^ *it);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Bitwise XOR not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_assign
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx)
      {
        // This operator is binary. `assign` is ignored.
        auto value = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        ctx.stack().top().dereference_mutable() = ::std::move(value);
        return air_status_next;
      }
  };

struct Traits_apply_xop_fma
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is ternary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        const auto& mid = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_real() && mid.is_real() && rhs.is_real()) {
          V_real& val = lhs.mut_real();
          V_real y_mul = mid.as_real();
          V_real z_add = rhs.as_real();

          // Perform fused multiply-add. This operation is always defined.
          val = ::std::fma(val, y_mul, z_add);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Fused multiply-add not applicable (operands were `$1`, `$2` and `$3`)"),
              lhs, mid, rhs);
      }
  };

struct Traits_apply_xop_head
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx)
      {
        // Push an array head modifier.
        Reference_Modifier::S_array_head xmod = { };
        ctx.stack().mut_top().push_modifier(::std::move(xmod));
        ctx.stack().top().dereference_readonly();
        return air_status_next;
      }
  };

struct Traits_apply_xop_tail
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx)
      {
        // Push an array tail modifier.
        Reference_Modifier::S_array_tail xmod = { };
        ctx.stack().mut_top().push_modifier(::std::move(xmod));
        ctx.stack().top().dereference_readonly();
        return air_status_next;
      }
  };

struct Traits_unpack_struct_array
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_unpack_struct_array& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_unpack_struct_array& altr)
      {
        AVMC_Queue::Uparam up;
        up.u32 = altr.nelems;
        up.u8v[0] = altr.immutable;
        return up;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this function.
        auto val = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();

        // Make sure it is really an `array`.
        V_array arr;
        if(!val.is_null() && !val.is_array())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Invalid argument for structured array binding (value was `$1`)"),
              val);

        if(val.is_array())
          arr = ::std::move(val.mut_array());

        for(uint32_t i = up.u32 - 1;  i != UINT32_MAX;  --i) {
          // Get the variable back.
          auto var = ctx.stack().top().unphase_variable_opt();
          ROCKET_ASSERT(var && !var->is_initialized());
          ctx.stack().pop();

          // Get its initializer.
          const Value* elem = arr.ptr(i);
          if(!elem)
            elem = &null_value;

          var->initialize(*elem);
          var->set_immutable(up.u8v[0]);
        }
        return air_status_next;
      }
  };

struct Traits_unpack_struct_object
  {
    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_unpack_struct_object& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.immutable;
        return up;
      }

    static
    const Source_Location&
    get_symbols(const AIR_Node::S_unpack_struct_object& altr)
      {
        return altr.sloc;
      }

    static
    cow_vector<phsh_string>
    make_sparam(bool& /*reachable*/, const AIR_Node::S_unpack_struct_object& altr)
      {
        return altr.keys;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const cow_vector<phsh_string>& keys)
      {
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this function.
        auto val = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();

        // Make sure it is really an `object`.
        V_object obj;
        if(!val.is_null() && !val.is_object())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Invalid argument for structured object binding (value was `$1`)"),
              val);

        if(val.is_object())
          obj = ::std::move(val.mut_object());

        for(auto it = keys.rbegin();  it != keys.rend();  ++it) {
          // Get the variable back.
          auto var = ctx.stack().top().unphase_variable_opt();
          ctx.stack().pop();
          ROCKET_ASSERT(var && !var->is_initialized());

          // Get its initializer.
          const Value* elem = obj.ptr(*it);
          if(!elem)
            elem = &null_value;

          var->initialize(*elem);
          var->set_immutable(up.u8v[0]);
        }
        return air_status_next;
      }
  };

struct Traits_define_null_variable
  {
    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_define_null_variable& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.immutable;
        return up;
      }

    static
    const Source_Location&
    get_symbols(const AIR_Node::S_define_null_variable& altr)
      {
        return altr.sloc;
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

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const Sparam_sloc_name& sp)
      {
        // Allocate a variable and inject it into the current context.
        const auto gcoll = ctx.global().garbage_collector();
        const auto var = gcoll->create_variable();
        ctx.insert_named_reference(sp.name).set_variable(var);
        ASTERIA_CALL_GLOBAL_HOOK(ctx.global(), on_variable_declare, sp.sloc, sp.name);

        // Initialize the variable to `null`.
        var->initialize(nullopt);
        var->set_immutable(up.u8v[0]);
        return air_status_next;
      }
  };

struct Traits_single_step_trap
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_single_step_trap& altr)
      {
        return altr.sloc;
      }

    static
    Source_Location
    make_sparam(bool& /*reachable*/, const AIR_Node::S_single_step_trap& altr)
      {
        return altr.sloc;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Source_Location& sloc)
      {
        ASTERIA_CALL_GLOBAL_HOOK(ctx.global(), on_single_step_trap, sloc);
        return air_status_next;
      }
  };

struct Traits_variadic_call
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_variadic_call& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_variadic_call& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = static_cast<uint8_t>(altr.ptc);
        return up;
      }

    static
    Source_Location
    make_sparam(bool& /*reachable*/, const AIR_Node::S_variadic_call& altr)
      {
        return altr.sloc;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const Source_Location& sloc)
      {
        const auto sentry = ctx.global().copy_recursion_sentry();
        ASTERIA_CALL_GLOBAL_HOOK(ctx.global(), on_single_step_trap, sloc);

        // Initialize arguments.
        auto& stack = ctx.stack();
        auto& alt_stack = ctx.alt_stack();
        alt_stack.clear();

        auto value = stack.top().dereference_readonly();
        switch(weaken_enum(value.type())) {
          case type_null:
            // Leave `stack` empty.
            break;

          case type_array: {
            auto& vals = value.mut_array();

            // Push all arguments backwards as temporaries.
            while(!vals.empty()) {
              alt_stack.push().set_temporary(::std::move(vals.mut_back()));
              vals.pop_back();
            }
            break;
          }

          case type_function: {
            const auto gfunc = ::std::move(value.mut_function());

            // Pass an empty argument stack to get the number of arguments to generate.
            // This destroys the `self` reference so we have to copy it first.
            auto gself = stack.mut_top().pop_modifier();
            do_invoke_nontail(stack.mut_top(), sloc, gfunc, ctx.global(), ::std::move(alt_stack));
            value = stack.top().dereference_readonly();

            // Verify the argument count.
            if(!value.is_integer())
              ASTERIA_THROW_RUNTIME_ERROR((
                  "Invalid number of variadic arguments (value `$1`)"),
                  value);

            int64_t nvargs = value.as_integer();
            if((nvargs < 0) || (nvargs > INT32_MAX))
              ASTERIA_THROW_RUNTIME_ERROR((
                  "Number of variadic arguments not acceptable (value `$1`)"),
                  nvargs);

            // Prepare `self` references for all upcoming  calls.
            for(int64_t k = 0;  k < nvargs;  ++k)
              stack.push() = gself;

            // Generate arguments and push them onto `stack`.
            // The top is the first argument.
            for(int64_t k = 0;  k < nvargs;  ++k) {
              // Initialize arguments for the generator function.
              alt_stack.clear();
              alt_stack.push().set_temporary(k);

              // Generate an argument. Ensure it is dereferenceable.
              auto& arg = stack.mut_top(static_cast<size_t>(k));
              do_invoke_nontail(arg, sloc, gfunc, ctx.global(), ::std::move(alt_stack));
              arg.dereference_readonly();
            }

            // Pop arguments and re-push them into the alternative stack.
            // This reverses all arguments so the top will be the last argument.
            alt_stack.clear();

            for(int64_t k = 0;  k < nvargs;  ++k) {
              alt_stack.push() = ::std::move(stack.mut_top());
              stack.pop();
            }
            break;
          }

          default:
            ASTERIA_THROW_RUNTIME_ERROR((
                "Invalid variadic argument generator (value `$1`)"),
                value);
        }
        stack.pop();

        // Copy the target, which shall be of type `function`.
        value = ctx.stack().top().dereference_readonly();
        if(!value.is_function())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Attempt to call a non-function (value `$1`)"),
              value);

        const auto& target = value.as_function();
        auto& self = ctx.stack().mut_top().pop_modifier();
        const auto ptc = static_cast<PTC_Aware>(up.u8v[0]);

        stack.clear_cache();
        alt_stack.clear_cache();

        return ROCKET_EXPECT(ptc == ptc_aware_none)
                 ? do_invoke_nontail(self, sloc, target, ctx.global(), ::std::move(alt_stack))
                 : do_invoke_tail(self, sloc, target, ptc, ::std::move(alt_stack));
      }
  };

struct Traits_defer_expression
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_defer_expression& altr)
      {
        return altr.sloc;
      }

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

struct Traits_import_call
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_import_call& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_import_call& altr)
      {
        AVMC_Queue::Uparam up;
        up.u32 = altr.nargs;
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
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up, const Sparam_import& sp)
      {
        const auto sentry = ctx.global().copy_recursion_sentry();
        ASTERIA_CALL_GLOBAL_HOOK(ctx.global(), on_single_step_trap, sp.sloc);

        // Pop arguments off the stack backwards.
        auto& alt_stack = ctx.alt_stack();
        auto& stack = ctx.stack();
        ROCKET_ASSERT(up.u32 != 0);
        do_pop_positional_arguments(alt_stack, stack, up.u32 - 1);

        // Copy the filename, which shall be of type `string`.
        auto value = stack.top().dereference_readonly();
        if(!value.is_string())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Invalid path specified for `import` (value `$1` not a string)"),
              value);

        auto path = value.as_string();
        if(path.empty())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Empty path specified for `import`"));

        const auto& src_path = sp.sloc.file();
        if((path[0] != '/') && (src_path[0] == '/'))
          path.insert(0, src_path, 0, src_path.rfind('/') + 1);

        unique_ptr<char, void (void*)> abspath(::free);
        abspath.reset(::realpath(path.safe_c_str(), nullptr));
        if(!abspath)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Could not open module file '$1'",
              "[`realpath()` failed: ${errno:full}]"),
              path);

        // Compile the script file into a function object.
        Module_Loader::Unique_Stream utext;
        path.assign(abspath);
        utext.reset(ctx.global().module_loader(), path.safe_c_str());

        // Parse source code.
        Token_Stream tstrm(sp.opts);
        tstrm.reload(path, 1, ::std::move(utext.get()));

        Statement_Sequence stmtq(sp.opts);
        stmtq.reload(::std::move(tstrm));

        // Instantiate the function.
        const Source_Location sloc(path, 0, 0);
        const cow_vector<phsh_string> params(1, sref("..."));

        AIR_Optimizer optmz(sp.opts);
        optmz.reload(nullptr, params, ctx.global(), stmtq);
        auto qtarget = optmz.create_function(sloc, sref("[file scope]"));

        stack.clear_cache();
        alt_stack.clear_cache();

        // Invoke the script.
        // `this` is null for imported scripts.
        auto& self = stack.mut_top();
        self.set_temporary(nullopt);
        do_invoke_nontail(self, sp.sloc, qtarget, ctx.global(), ::std::move(alt_stack));
        return air_status_next;
      }
  };

struct Traits_declare_reference
  {
    static
    Sparam_name
    make_sparam(bool& /*reachable*/, const AIR_Node::S_declare_reference& altr)
      {
        Sparam_name sp;
        sp.name = altr.name;
        return sp;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_name& sp)
      {
        ctx.insert_named_reference(sp.name).clear();
        return air_status_next;
      }
  };

struct Traits_initialize_reference
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_initialize_reference& altr)
      {
        return altr.sloc;
      }

    static
    Sparam_name
    make_sparam(bool& /*reachable*/, const AIR_Node::S_initialize_reference& altr)
      {
        Sparam_name sp;
        sp.name = altr.name;
        return sp;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, const Sparam_name& sp)
      {
        // Pop a reference from the stack. Ensure it is dereferenceable.
        ctx.insert_named_reference(sp.name) = ::std::move(ctx.stack().mut_top());
        ctx.stack().pop();
        return air_status_next;
      }
  };

struct Traits_catch_expression
  {
    static
    AVMC_Queue
    make_sparam(bool& /*reachable*/, const AIR_Node::S_catch_expression& altr)
      {
        AVMC_Queue queue;
        do_solidify_nodes(queue, altr.code_body);
        return queue;
      }

    static
    AIR_Status
    execute(Executive_Context& ctx, const AVMC_Queue& queue)
      {
        // Evaluate the body expression. If it effects an exception,
        // the exception is returned; otherwise `null` is returned.
        size_t old_size = ctx.stack().size();
        Value val;

        try {
          AIR_Status status = queue.execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
        }
        catch(Runtime_Error& except) {
          val = except.value();
        }

        while(ctx.stack().size() > old_size)
          ctx.stack().pop();

        ROCKET_ASSERT(ctx.stack().size() == old_size);
        ctx.stack().push().set_temporary(::std::move(val));
        return air_status_next;
      }
  };

struct Traits_apply_xop_lzcnt
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          V_integer& val = rhs.mut_integer();

          // Get the number of leading zero bits.
          val = (int64_t) ROCKET_LZCNT64((uint64_t) val);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__lzcnt` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_tzcnt
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          V_integer& val = rhs.mut_integer();

          // Get the number of trailing zero bits.
          val = (int64_t) ROCKET_TZCNT64((uint64_t) val);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__tzcnt` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_popcnt
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is unary.
        auto& top = ctx.stack().mut_top();
        auto& rhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(rhs.type() == type_integer) {
          V_integer& val = rhs.mut_integer();

          // Get the number of one bits.
          val = (int64_t) ROCKET_POPCNT64((uint64_t) val);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "`__popcnt` not applicable (operand was `$1`)"),
              rhs);
      }
  };

struct Traits_apply_xop_addm
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform modular addition.
          ROCKET_ADD_OVERFLOW(val, other, &val);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Modular addition not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_subm
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform modular subtraction.
          ROCKET_SUB_OVERFLOW(val, other, &val);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Modular subtraction not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_mulm
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform modular multiplication.
          ROCKET_MUL_OVERFLOW(val, other, &val);
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Modular multiplication not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_adds
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform saturating addition.
          if(ROCKET_ADD_OVERFLOW(val, other, &val))
            val = (other >> 63) ^ INT64_MAX;
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Saturating addition not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_subs
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform saturating subtraction.
          if(ROCKET_SUB_OVERFLOW(val, other, &val))
            val = (other >> 63) ^ INT64_MIN;
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Saturating subtraction not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_muls
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    static
    AVMC_Queue::Uparam
    make_uparam(bool& /*reachable*/, const AIR_Node::S_apply_operator& altr)
      {
        AVMC_Queue::Uparam up;
        up.u8v[0] = altr.assign;
        return up;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx, AVMC_Queue::Uparam up)
      {
        // This operator is binary.
        const auto& rhs = ctx.stack().top().dereference_readonly();
        ctx.stack().pop();
        auto& top = ctx.stack().mut_top();
        auto& lhs = up.u8v[0] ? top.dereference_mutable() : top.dereference_copy();

        if(lhs.is_integer() && rhs.is_integer()) {
          V_integer& val = lhs.mut_integer();
          V_integer other = rhs.as_integer();

          // Perform saturating multiplication.
          if(ROCKET_MUL_OVERFLOW(val, other, &val))
            val = (val >> 63) ^ (other >> 63) ^ INT64_MAX;
          return air_status_next;
        }
        else
          ASTERIA_THROW_RUNTIME_ERROR((
              "Saturating multiplication not applicable (operands were `$1` and `$2`)"),
              lhs, rhs);
      }
  };

struct Traits_apply_xop_random
  {
    static
    const Source_Location&
    get_symbols(const AIR_Node::S_apply_operator& altr)
      {
        return altr.sloc;
      }

    ROCKET_FLATTEN static
    AIR_Status
    execute(Executive_Context& ctx)
      {
        // Generate a random subscript and push it.
        Reference_Modifier::S_array_random xmod = { ctx.global().random_engine()->bump() };
        ctx.stack().mut_top().push_modifier(::std::move(xmod));
        ctx.stack().top().dereference_readonly();
        return air_status_next;
      }
  };

// Finally...
template<typename TraitsT, typename NodeT, typename = void>
struct symbol_getter
  {
    static constexpr
    const Source_Location*
    opt(const NodeT&) noexcept
      { return nullptr;  }
 };

template<typename TraitsT, typename NodeT>
struct symbol_getter<TraitsT, NodeT,
    ROCKET_VOID_DECLTYPE(
        TraitsT::get_symbols(
            ::std::declval<const NodeT&>()))>
  {
    static constexpr
    const Source_Location*
    opt(const NodeT& altr) noexcept
      { return ::std::addressof(TraitsT::get_symbols(altr));  }
  };

template<typename TraitsT, typename NodeT, typename = void>
struct has_uparam
  :
    ::std::false_type
  {
  };

template<typename TraitsT, typename NodeT>
struct has_uparam<TraitsT, NodeT,
    ROCKET_VOID_DECLTYPE(
        TraitsT::make_uparam(::std::declval<bool&>(),
            ::std::declval<const NodeT&>()))>
  :
    ::std::true_type
  {
  };

template<typename TraitsT, typename NodeT, typename = void>
struct has_sparam
  :
    ::std::false_type
  {
  };

template<typename TraitsT, typename NodeT>
struct has_sparam<TraitsT, NodeT,
    ROCKET_VOID_DECLTYPE(
        TraitsT::make_sparam(::std::declval<bool&>(),
            ::std::declval<const NodeT&>()))>
  :
    ::std::true_type
  {
  };

template<typename TraitsT, typename NodeT, bool, bool>
struct solidify_disp;

template<typename TraitsT, typename NodeT>
struct solidify_disp<TraitsT, NodeT, true, true>  // uparam, sparam
  {
    static
    AIR_Status
    thunk(Executive_Context& ctx, const AVMC_Queue::Header* head)
      {
        return TraitsT::execute(ctx,
            static_cast<typename ::std::decay<decltype(
                TraitsT::make_uparam(::std::declval<bool&>(),
                    ::std::declval<const NodeT&>()))>::type>(head->uparam),
            reinterpret_cast<const typename ::std::decay<decltype(
                TraitsT::make_sparam(::std::declval<bool&>(),
                    ::std::declval<const NodeT&>()))>::type&>(head->sparam));
      }

    static
    void
    append(bool& reachable, AVMC_Queue& queue, const NodeT& altr)
      {
        queue.append(thunk, symbol_getter<TraitsT, NodeT>::opt(altr),
            TraitsT::make_uparam(reachable, altr),
            TraitsT::make_sparam(reachable, altr));
      }
  };

template<typename TraitsT, typename NodeT>
struct solidify_disp<TraitsT, NodeT, false, true>  // uparam, sparam
  {
    static
    AIR_Status
    thunk(Executive_Context& ctx, const AVMC_Queue::Header* head)
      {
        return TraitsT::execute(ctx,
            reinterpret_cast<const typename ::std::decay<decltype(
                TraitsT::make_sparam(::std::declval<bool&>(),
                    ::std::declval<const NodeT&>()))>::type&>(head->sparam));
      }

    static
    void
    append(bool& reachable, AVMC_Queue& queue, const NodeT& altr)
      {
        queue.append(thunk, symbol_getter<TraitsT, NodeT>::opt(altr),
            AVMC_Queue::Uparam(),
            TraitsT::make_sparam(reachable, altr));
      }
  };

template<typename TraitsT, typename NodeT>
struct solidify_disp<TraitsT, NodeT, true, false>  // uparam, sparam
  {
    static
    AIR_Status
    thunk(Executive_Context& ctx, const AVMC_Queue::Header* head)
      {
        return TraitsT::execute(ctx,
            static_cast<typename ::std::decay<decltype(
                TraitsT::make_uparam(::std::declval<bool&>(),
                    ::std::declval<const NodeT&>()))>::type>(head->uparam));
      }

    static
    void
    append(bool& reachable, AVMC_Queue& queue, const NodeT& altr)
      {
        queue.append(thunk, symbol_getter<TraitsT, NodeT>::opt(altr),
            TraitsT::make_uparam(reachable, altr));
      }
  };

template<typename TraitsT, typename NodeT>
struct solidify_disp<TraitsT, NodeT, false, false>  // uparam, sparam
  {
    static
    AIR_Status
    thunk(Executive_Context& ctx, const AVMC_Queue::Header* /*head*/)
      {
        return TraitsT::execute(ctx);
      }

    static
    void
    append(bool& /*reachable*/, AVMC_Queue& queue, const NodeT& altr)
      {
        queue.append(thunk, symbol_getter<TraitsT, NodeT>::opt(altr));
      }
  };

template<typename TraitsT, typename NodeT>
inline
bool
do_solidify(AVMC_Queue& queue, const NodeT& altr)
  {
    constexpr bool has_up = has_uparam<TraitsT, NodeT>::value;
    constexpr bool has_sp = has_sparam<TraitsT, NodeT>::value;
    bool reachable = true;
    solidify_disp<TraitsT, NodeT, has_up, has_sp>::append(reachable, queue, altr);
    return reachable;
  }

}  // namespace

opt<AIR_Node>
AIR_Node::
rebind_opt(Abstract_Context& ctx) const
  {
    switch(this->index()) {
      case index_clear_stack:
        return nullopt;

      case index_execute_block: {
        const auto& altr = this->m_stor.as<index_execute_block>();

        // Rebind the body.
        Analytic_Context ctx_body(Analytic_Context::M_plain(), ctx);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx_body);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_declare_variable:
      case index_initialize_variable:
        return nullopt;

      case index_if_statement: {
        const auto& altr = this->m_stor.as<index_if_statement>();

        // Rebind both branches.
        Analytic_Context ctx_body(Analytic_Context::M_plain(), ctx);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_true, ctx_body);
        do_rebind_nodes(dirty, bound.code_false, ctx_body);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_switch_statement: {
        const auto& altr = this->m_stor.as<index_switch_statement>();

        // Rebind all clauses.
        Analytic_Context ctx_body(Analytic_Context::M_plain(), ctx);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_labels, ctx);  // this is not part of the body!
        do_rebind_nodes(dirty, bound.code_bodies, ctx_body);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_do_while_statement: {
        const auto& altr = this->m_stor.as<index_do_while_statement>();

        // Rebind the body and the condition.
        Analytic_Context ctx_body(Analytic_Context::M_plain(), ctx);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx_body);
        do_rebind_nodes(dirty, bound.code_cond, ctx);  // this is not part of the body!

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_while_statement: {
        const auto& altr = this->m_stor.as<index_while_statement>();

        // Rebind the condition and the body.
        Analytic_Context ctx_body(Analytic_Context::M_plain(), ctx);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_cond, ctx);  // this is not part of the body!
        do_rebind_nodes(dirty, bound.code_body, ctx_body);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_for_each_statement: {
        const auto& altr = this->m_stor.as<index_for_each_statement>();

        // Rebind the range initializer and the body.
        Analytic_Context ctx_for(Analytic_Context::M_plain(), ctx);
        Analytic_Context ctx_body(Analytic_Context::M_plain(), ctx_for);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_init, ctx_for);
        do_rebind_nodes(dirty, bound.code_body, ctx_body);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_for_statement: {
        const auto& altr = this->m_stor.as<index_for_statement>();

        // Rebind the initializer, the condition, the loop increment and the body.
        Analytic_Context ctx_for(Analytic_Context::M_plain(), ctx);
        Analytic_Context ctx_body(Analytic_Context::M_plain(), ctx_for);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_init, ctx_for);
        do_rebind_nodes(dirty, bound.code_cond, ctx_for);
        do_rebind_nodes(dirty, bound.code_step, ctx_for);
        do_rebind_nodes(dirty, bound.code_body, ctx_body);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_try_statement: {
        const auto& altr = this->m_stor.as<index_try_statement>();

        // Rebind the `try` and `catch` clauses.
        Analytic_Context ctx_body(Analytic_Context::M_plain(), ctx);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_try, ctx_body);
        do_rebind_nodes(dirty, bound.code_catch, ctx_body);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_throw_statement:
      case index_assert_statement:
      case index_simple_status:
      case index_check_argument:
      case index_push_global_reference:
        return nullopt;

      case index_push_local_reference: {
        const auto& altr = this->m_stor.as<index_push_local_reference>();

        // Get the context.
        Abstract_Context* qctx = &ctx;
        for(uint32_t k = 0;  k != altr.depth;  ++k)
          qctx = qctx->get_parent_opt();

        // Don't bind placeholder references in analytic contexts.
        if(qctx->is_analytic())
          return nullopt;

        // Look for the name in the context.
        auto qref = qctx->get_named_reference_opt(altr.name);
        if(!qref)
          return nullopt;

        // Check if control flow has bypassed its initialization.
        if(qref->is_invalid())
          ASTERIA_THROW_RUNTIME_ERROR(("Use of bypassed variable or reference `$1`"), altr.name);

        S_push_bound_reference xnode = { *qref };
        return ::std::move(xnode);
      }

      case index_push_bound_reference:
        return nullopt;

      case index_define_function: {
        const auto& altr = this->m_stor.as<index_define_function>();

        // Rebind the function body.
        Analytic_Context ctx_func(Analytic_Context::M_function(),
                                  &ctx, altr.params);
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx_func);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_branch_expression: {
        const auto& altr = this->m_stor.as<index_branch_expression>();

        // Rebind the expression.
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_true, ctx);
        do_rebind_nodes(dirty, bound.code_false, ctx);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();

        // Rebind the expression.
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_null, ctx);

        return do_return_rebound_opt(dirty, ::std::move(bound));
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
        return nullopt;

      case index_defer_expression: {
        const auto& altr = this->m_stor.as<index_defer_expression>();

        // Rebind the expression.
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_import_call:
      case index_declare_reference:
      case index_initialize_reference:
        return nullopt;

      case index_catch_expression: {
        const auto& altr = this->m_stor.as<index_catch_expression>();

        // Rebind the expression.
        bool dirty = false;
        auto bound = altr;

        do_rebind_nodes(dirty, bound.code_body, ctx);

        return do_return_rebound_opt(dirty, ::std::move(bound));
      }

      case index_return_statement:
        return nullopt;

      default:
        ASTERIA_TERMINATE((
            "Invalid AIR node type (index `$1`)"),
            this->index());
    }
  }

bool
AIR_Node::
solidify(AVMC_Queue& queue) const
  {
    switch(this->index()) {
      case index_clear_stack:
        return do_solidify<Traits_clear_stack>(queue,
                       this->m_stor.as<index_clear_stack>());

      case index_execute_block:
        return do_solidify<Traits_execute_block>(queue,
                       this->m_stor.as<index_execute_block>());

      case index_declare_variable:
        return do_solidify<Traits_declare_variable>(queue,
                       this->m_stor.as<index_declare_variable>());

      case index_initialize_variable:
        return do_solidify<Traits_initialize_variable>(queue,
                       this->m_stor.as<index_initialize_variable>());

      case index_if_statement:
        return do_solidify<Traits_if_statement>(queue,
                       this->m_stor.as<index_if_statement>());

      case index_switch_statement:
        return do_solidify<Traits_switch_statement>(queue,
                       this->m_stor.as<index_switch_statement>());

      case index_do_while_statement:
        return do_solidify<Traits_do_while_statement>(queue,
                       this->m_stor.as<index_do_while_statement>());

      case index_while_statement:
        return do_solidify<Traits_while_statement>(queue,
                       this->m_stor.as<index_while_statement>());

      case index_for_each_statement:
        return do_solidify<Traits_for_each_statement>(queue,
                       this->m_stor.as<index_for_each_statement>());

      case index_for_statement:
        return do_solidify<Traits_for_statement>(queue,
                       this->m_stor.as<index_for_statement>());

      case index_try_statement:
        return do_solidify<Traits_try_statement>(queue,
                       this->m_stor.as<index_try_statement>());

      case index_throw_statement:
        return do_solidify<Traits_throw_statement>(queue,
                       this->m_stor.as<index_throw_statement>());

      case index_assert_statement:
        return do_solidify<Traits_assert_statement>(queue,
                       this->m_stor.as<index_assert_statement>());

      case index_simple_status:
        return do_solidify<Traits_simple_status>(queue,
                       this->m_stor.as<index_simple_status>());

      case index_check_argument:
        return do_solidify<Traits_check_argument>(queue,
                       this->m_stor.as<index_check_argument>());

      case index_push_global_reference:
        return do_solidify<Traits_push_global_reference>(queue,
                       this->m_stor.as<index_push_global_reference>());

      case index_push_local_reference:
        return do_solidify<Traits_push_local_reference>(queue,
                       this->m_stor.as<index_push_local_reference>());

      case index_push_bound_reference:
        return do_solidify<Traits_push_bound_reference>(queue,
                       this->m_stor.as<index_push_bound_reference>());

      case index_define_function:
        return do_solidify<Traits_define_function>(queue,
                       this->m_stor.as<index_define_function>());

      case index_branch_expression:
        return do_solidify<Traits_branch_expression>(queue,
                       this->m_stor.as<index_branch_expression>());

      case index_coalescence:
        return do_solidify<Traits_coalescence>(queue,
                       this->m_stor.as<index_coalescence>());

      case index_function_call:
        return do_solidify<Traits_function_call>(queue,
                       this->m_stor.as<index_function_call>());

      case index_member_access:
        return do_solidify<Traits_member_access>(queue,
                       this->m_stor.as<index_member_access>());

      case index_push_unnamed_array:
        return do_solidify<Traits_push_unnamed_array>(queue,
                       this->m_stor.as<index_push_unnamed_array>());

      case index_push_unnamed_object:
        return do_solidify<Traits_push_unnamed_object>(queue,
                       this->m_stor.as<index_push_unnamed_object>());

      case index_apply_operator: {
        const auto& altr = this->m_stor.as<index_apply_operator>();
        switch(altr.xop) {
          case xop_inc:
            return do_solidify<Traits_apply_xop_inc>(queue, altr);

          case xop_dec:
            return do_solidify<Traits_apply_xop_dec>(queue, altr);

          case xop_subscr:
            return do_solidify<Traits_apply_xop_subscr>(queue, altr);

          case xop_pos:
            return do_solidify<Traits_apply_xop_pos>(queue, altr);

          case xop_neg:
            return do_solidify<Traits_apply_xop_neg>(queue, altr);

          case xop_notb:
            return do_solidify<Traits_apply_xop_notb>(queue, altr);

          case xop_notl:
            return do_solidify<Traits_apply_xop_notl>(queue, altr);

          case xop_unset:
            return do_solidify<Traits_apply_xop_unset>(queue, altr);

          case xop_countof:
            return do_solidify<Traits_apply_xop_countof>(queue, altr);

          case xop_typeof:
            return do_solidify<Traits_apply_xop_typeof>(queue, altr);

          case xop_sqrt:
            return do_solidify<Traits_apply_xop_sqrt>(queue, altr);

          case xop_isnan:
            return do_solidify<Traits_apply_xop_isnan>(queue, altr);

          case xop_isinf:
            return do_solidify<Traits_apply_xop_isinf>(queue, altr);

          case xop_abs:
            return do_solidify<Traits_apply_xop_abs>(queue, altr);

          case xop_sign:
            return do_solidify<Traits_apply_xop_sign>(queue, altr);

          case xop_round:
            return do_solidify<Traits_apply_xop_round>(queue, altr);

          case xop_floor:
            return do_solidify<Traits_apply_xop_floor>(queue, altr);

          case xop_ceil:
            return do_solidify<Traits_apply_xop_ceil>(queue, altr);

          case xop_trunc:
            return do_solidify<Traits_apply_xop_trunc>(queue, altr);

          case xop_iround:
            return do_solidify<Traits_apply_xop_iround>(queue, altr);

          case xop_ifloor:
            return do_solidify<Traits_apply_xop_ifloor>(queue, altr);

          case xop_iceil:
            return do_solidify<Traits_apply_xop_iceil>(queue, altr);

          case xop_itrunc:
            return do_solidify<Traits_apply_xop_itrunc>(queue, altr);

          case xop_cmp_eq:
            return do_solidify<Traits_apply_xop_cmp_eq>(queue, altr);

          case xop_cmp_ne:
            return do_solidify<Traits_apply_xop_cmp_ne>(queue, altr);

          case xop_cmp_lt:
            return do_solidify<Traits_apply_xop_cmp_lt>(queue, altr);

          case xop_cmp_gt:
            return do_solidify<Traits_apply_xop_cmp_gt>(queue, altr);

          case xop_cmp_lte:
            return do_solidify<Traits_apply_xop_cmp_lte>(queue, altr);

          case xop_cmp_gte:
            return do_solidify<Traits_apply_xop_cmp_gte>(queue, altr);

          case xop_cmp_3way:
            return do_solidify<Traits_apply_xop_cmp_3way>(queue, altr);

          case xop_cmp_un:
            return do_solidify<Traits_apply_xop_cmp_un>(queue, altr);

          case xop_add:
            return do_solidify<Traits_apply_xop_add>(queue, altr);

          case xop_sub:
            return do_solidify<Traits_apply_xop_sub>(queue, altr);

          case xop_mul:
            return do_solidify<Traits_apply_xop_mul>(queue, altr);

          case xop_div:
            return do_solidify<Traits_apply_xop_div>(queue, altr);

          case xop_mod:
            return do_solidify<Traits_apply_xop_mod>(queue, altr);

          case xop_sll:
            return do_solidify<Traits_apply_xop_sll>(queue, altr);

          case xop_srl:
            return do_solidify<Traits_apply_xop_srl>(queue, altr);

          case xop_sla:
            return do_solidify<Traits_apply_xop_sla>(queue, altr);

          case xop_sra:
            return do_solidify<Traits_apply_xop_sra>(queue, altr);

          case xop_andb:
            return do_solidify<Traits_apply_xop_andb>(queue, altr);

          case xop_orb:
            return do_solidify<Traits_apply_xop_orb>(queue, altr);

          case xop_xorb:
            return do_solidify<Traits_apply_xop_xorb>(queue, altr);

          case xop_assign:
            return do_solidify<Traits_apply_xop_assign>(queue, altr);

          case xop_fma:
            return do_solidify<Traits_apply_xop_fma>(queue, altr);

          case xop_head:
            return do_solidify<Traits_apply_xop_head>(queue, altr);

          case xop_tail:
            return do_solidify<Traits_apply_xop_tail>(queue, altr);

          case xop_lzcnt:
            return do_solidify<Traits_apply_xop_lzcnt>(queue, altr);

          case xop_tzcnt:
            return do_solidify<Traits_apply_xop_tzcnt>(queue, altr);

          case xop_popcnt:
            return do_solidify<Traits_apply_xop_popcnt>(queue, altr);

          case xop_addm:
            return do_solidify<Traits_apply_xop_addm>(queue, altr);

          case xop_subm:
            return do_solidify<Traits_apply_xop_subm>(queue, altr);

          case xop_mulm:
            return do_solidify<Traits_apply_xop_mulm>(queue, altr);

          case xop_adds:
            return do_solidify<Traits_apply_xop_adds>(queue, altr);

          case xop_subs:
            return do_solidify<Traits_apply_xop_subs>(queue, altr);

          case xop_muls:
            return do_solidify<Traits_apply_xop_muls>(queue, altr);

          case xop_random:
            return do_solidify<Traits_apply_xop_random>(queue, altr);

          default:
            ASTERIA_TERMINATE((
                "Invalid operator type (xop `$1`)"),
                altr.xop);
        }
      }

      case index_unpack_struct_array:
        return do_solidify<Traits_unpack_struct_array>(queue,
                       this->m_stor.as<index_unpack_struct_array>());

      case index_unpack_struct_object:
        return do_solidify<Traits_unpack_struct_object>(queue,
                       this->m_stor.as<index_unpack_struct_object>());

      case index_define_null_variable:
        return do_solidify<Traits_define_null_variable>(queue,
                       this->m_stor.as<index_define_null_variable>());

      case index_single_step_trap:
        return do_solidify<Traits_single_step_trap>(queue,
                       this->m_stor.as<index_single_step_trap>());

      case index_variadic_call:
        return do_solidify<Traits_variadic_call>(queue,
                       this->m_stor.as<index_variadic_call>());

      case index_defer_expression:
        return do_solidify<Traits_defer_expression>(queue,
                       this->m_stor.as<index_defer_expression>());

      case index_import_call:
        return do_solidify<Traits_import_call>(queue,
                       this->m_stor.as<index_import_call>());

      case index_declare_reference:
        return do_solidify<Traits_declare_reference>(queue,
                       this->m_stor.as<index_declare_reference>());

      case index_initialize_reference:
        return do_solidify<Traits_initialize_reference>(queue,
                       this->m_stor.as<index_initialize_reference>());

      case index_catch_expression:
        return do_solidify<Traits_catch_expression>(queue,
                       this->m_stor.as<index_catch_expression>());

      case index_return_statement:
        return do_solidify<Traits_return_statement>(queue,
                       this->m_stor.as<index_return_statement>());

      default:
        ASTERIA_TERMINATE((
            "Invalid AIR node type (index `$1`)"),
            this->index());
    }
  }

void
AIR_Node::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    switch(this->index()) {
      case index_clear_stack:
        return;

      case index_execute_block: {
        const auto& altr = this->m_stor.as<index_execute_block>();
        do_collect_variables_for_each(altr.code_body, staged, temp);
        return;
      }

      case index_declare_variable:
      case index_initialize_variable:
        return;

      case index_if_statement: {
        const auto& altr = this->m_stor.as<index_if_statement>();
        do_collect_variables_for_each(altr.code_true, staged, temp);
        do_collect_variables_for_each(altr.code_false, staged, temp);
        return;
      }

      case index_switch_statement: {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        for(size_t i = 0;  i < altr.code_labels.size();  ++i) {
          do_collect_variables_for_each(altr.code_labels.at(i), staged, temp);
          do_collect_variables_for_each(altr.code_bodies.at(i), staged, temp);
        }
        return;
      }

      case index_do_while_statement: {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        do_collect_variables_for_each(altr.code_body, staged, temp);
        do_collect_variables_for_each(altr.code_cond, staged, temp);
        return;
      }

      case index_while_statement: {
        const auto& altr = this->m_stor.as<index_while_statement>();
        do_collect_variables_for_each(altr.code_cond, staged, temp);
        do_collect_variables_for_each(altr.code_body, staged, temp);
        return;
      }

      case index_for_each_statement: {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        do_collect_variables_for_each(altr.code_init, staged, temp);
        do_collect_variables_for_each(altr.code_body, staged, temp);
        return;
      }

      case index_for_statement: {
        const auto& altr = this->m_stor.as<index_for_statement>();
        do_collect_variables_for_each(altr.code_init, staged, temp);
        do_collect_variables_for_each(altr.code_cond, staged, temp);
        do_collect_variables_for_each(altr.code_step, staged, temp);
        do_collect_variables_for_each(altr.code_body, staged, temp);
        return;
      }

      case index_try_statement: {
        const auto& altr = this->m_stor.as<index_try_statement>();
        do_collect_variables_for_each(altr.code_try, staged, temp);
        do_collect_variables_for_each(altr.code_catch, staged, temp);
        return;
      }

      case index_throw_statement:
      case index_assert_statement:
      case index_simple_status:
      case index_check_argument:
      case index_push_global_reference:
      case index_push_local_reference:
        return;

      case index_push_bound_reference: {
        const auto& altr = this->m_stor.as<index_push_bound_reference>();
        altr.ref.collect_variables(staged, temp);
        return;
      }

      case index_define_function: {
        const auto& altr = this->m_stor.as<index_define_function>();
        do_collect_variables_for_each(altr.code_body, staged, temp);
        return;
      }

      case index_branch_expression: {
        const auto& altr = this->m_stor.as<index_branch_expression>();
        do_collect_variables_for_each(altr.code_true, staged, temp);
        do_collect_variables_for_each(altr.code_false, staged, temp);
        return;
      }

      case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();
        do_collect_variables_for_each(altr.code_null, staged, temp);
        return;
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
        return;

      case index_defer_expression: {
        const auto& altr = this->m_stor.as<index_defer_expression>();
        do_collect_variables_for_each(altr.code_body, staged, temp);
        return;
      }

      case index_import_call:
      case index_declare_reference:
      case index_initialize_reference:
        return;

      case index_catch_expression: {
        const auto& altr = this->m_stor.as<index_catch_expression>();
        do_collect_variables_for_each(altr.code_body, staged, temp);
        return;
      }

      case index_return_statement:
        return;

      default:
        ASTERIA_TERMINATE((
            "Invalid AIR node type (index `$1`)"),
            this->index());
    }
  }

}  // namespace asteria
