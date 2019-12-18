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
#include "../compiler/statement.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    DCE_Result do_dce_recurse(cow_vector<AIR_Node>& code)
      {
        size_t cur = 0;
        while(cur != code.size()) {
          // Run DCE on the node at `cur`.
          auto dce = code.mut(cur).optimize_dce();
          if(dce == dce_prune) {
            // Erase all nodes following the current one.
            code.erase(++cur);
            return dce_prune;
          }
          if(dce == dce_empty) {
            // Erase the empty node.
            code.erase(cur, 1);
            continue;
          }
          // Go to the next node.
          ROCKET_ASSERT(dce == dce_none);
          ++cur;
        }
        // Check whether `code` is empty now.
        if(cur == 0) {
          return dce_empty;
        }
        // No operation can be performed.
        return dce_none;
      }

    }

DCE_Result AIR_Node::optimize_dce()
  {
    switch(this->m_stor.index()) {
      {{
    case index_execute_block:
        auto& altr = this->m_stor.as<index_execute_block>();
        // The node has no effect if the body is empty.
        auto dce = do_dce_recurse(altr.code_body);
        return dce;
      }{
    case index_if_statement:
        auto& altr = this->m_stor.as<index_if_statement>();
        // The node has no effect if both branches are empty.
        // Note that the condition is not part of this node.
        auto dce_true = do_dce_recurse(altr.code_true);
        auto dce_false = do_dce_recurse(altr.code_false);
        if(dce_true == dce_false) {
          return dce_true;
        }
        return dce_none;
      }{
    case index_switch_statement:
        auto& altr = this->m_stor.as<index_switch_statement>();
        // The node has no effect if there are no clauses.
        // Note that the condition is not part of this node.
        if(altr.code_bodies.empty()) {
          return dce_empty;
        }
        for(size_t i = 0; i != altr.code_bodies.size(); ++i) {
          do_dce_recurse(altr.code_bodies.mut(i));
        }
        return dce_none;
      }{
    case index_while_statement:
        auto& altr = this->m_stor.as<index_while_statement>();
        // Loop statements cannot be DCE'd.
        do_dce_recurse(altr.code_body);
        return dce_none;
      }{
    case index_do_while_statement:
        auto& altr = this->m_stor.as<index_do_while_statement>();
        // Loop statements cannot be DCE'd.
        do_dce_recurse(altr.code_body);
        return dce_none;
      }{
    case index_for_each_statement:
        auto& altr = this->m_stor.as<index_for_each_statement>();
        // Loop statements cannot be DCE'd.
        do_dce_recurse(altr.code_body);
        return dce_none;
      }{
    case index_for_statement:
        auto& altr = this->m_stor.as<index_for_statement>();
        // Loop statements cannot be DCE'd.
        do_dce_recurse(altr.code_body);
        return dce_none;
      }{
    case index_try_statement:
        auto& altr = this->m_stor.as<index_try_statement>();
        // The node has no effect if the `try` block is empty.
        auto dce_try = do_dce_recurse(altr.code_try);
        if(dce_try == dce_empty) {
          return dce_empty;
        }
        auto dce_catch = do_dce_recurse(altr.code_catch);
        if(dce_try == dce_catch) {
          return dce_try;
        }
        return dce_none;
      }{
    case index_throw_statement:
        return dce_prune;
      }{
    case index_simple_status:
        auto& altr = this->m_stor.as<index_simple_status>();
        // The node has no effect if it equals `air_status_next`, which is effectively a no-op.
        if(altr.status == air_status_next) {
          return dce_empty;
        }
        return dce_prune;
      }{
    case index_return_by_value:
        return dce_prune;
      }{
    case index_function_call:
        auto& altr = this->m_stor.as<index_function_call>();
        // This node is part of an expression.
        if(altr.tco_aware != tco_aware_none) {
          return dce_prune;
        }
        return dce_none;
      }}
    default:
      return dce_none;
    }
  }

    namespace {

    using ParamU      = AVMC_Queue::ParamU;
    using Executor    = AVMC_Queue::Executor;
    using Enumerator  = AVMC_Queue::Enumerator;

    ///////////////////////////////////////////////////////////////////////////
    // Auxiliary functions
    ///////////////////////////////////////////////////////////////////////////

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
        // Be advised that you must forward the status code as is, because TCO'd expressions may return `air_status_return`.
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

    template<typename SparamT> inline const SparamT* do_pcast(const void* params) noexcept
      {
        return static_cast<const SparamT*>(params);
      }

    template<typename SparamT> Variable_Callback& do_pcast_enumerate(Variable_Callback& callback, ParamU /*paramu*/, const void* params)
      {
        return do_pcast<SparamT>(params)->enumerate_variables(callback);
      }

    // This is the trait struct for parameter types that implement `enumerate_variables()`.
    template<typename SparamT, typename = void> struct AVMC_Appender : SparamT
      {
        ParamU paramu;

        constexpr AVMC_Appender()
          :
            SparamT(), paramu()
          {
          }

        AVMC_Queue& request(AVMC_Queue& queue) const
          {
            return queue.request(sizeof(SparamT));
          }
        template<Executor executorT> AVMC_Queue& output(AVMC_Queue& queue)
          {
            return queue.append<executorT, do_pcast_enumerate<SparamT>>(this->paramu, static_cast<SparamT&&>(*this));
          }
      };

    // This is the trait struct for parameter types that do not implement `enumerate_variables()`.
    template<typename SparamT> struct AVMC_Appender<SparamT, ASTERIA_VOID_T(typename SparamT::nonenumerable)> : SparamT
      {
        ParamU paramu;

        constexpr AVMC_Appender()
          :
            SparamT(), paramu()
          {
          }

        AVMC_Queue& request(AVMC_Queue& queue) const
          {
            return queue.request(sizeof(SparamT));
          }
        template<Executor executorT> AVMC_Queue& output(AVMC_Queue& queue)
          {
            return queue.append<executorT>(this->paramu, static_cast<SparamT&&>(*this));
          }
      };

    // This is the trait struct when there is no parameter.
    template<> struct AVMC_Appender<void, void>
      {
        ParamU paramu;

        constexpr AVMC_Appender()
          :
            paramu()
          {
          }

        AVMC_Queue& request(AVMC_Queue& queue) const
          {
            return queue.request(0);
          }
        template<Executor executorT> AVMC_Queue& output(AVMC_Queue& queue)
          {
            return queue.append<executorT>(this->paramu);
          }
      };

    AVMC_Queue& do_solidify_queue(AVMC_Queue& queue, const cow_vector<AIR_Node>& code)
      {
        ::rocket::for_each(code, [&](const AIR_Node& node) { node.solidify(queue, 0);  });  // 1st pass
        ::rocket::for_each(code, [&](const AIR_Node& node) { node.solidify(queue, 1);  });  // 2nd pass
        return queue;
      }

    rcptr<Abstract_Function> do_instantiate_function(const AIR_Node::S_instantiate_function& xnode, const Abstract_Context* ctx_opt)
      {
        const auto& opts = xnode.opts;
        const auto& sloc = xnode.sloc;
        const auto& name = xnode.name;
        const auto& params = xnode.params;
        const auto& body = xnode.body;

        // Create the prototype string.
        cow_string func = name;
        size_t epos;
        // XXX: The parameter list is only appended if the name really looks like a function.
        //      Placeholders such as `<file>` or `<native>` do not precede parameter lists.
        static constexpr char s_idchars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
        if(!name.empty() && ::std::memchr(s_idchars, name.back(), ::rocket::countof(s_idchars) - 1)) {
          // Append the parameter list. Parameters are separated by commas.
          func << '(';
          epos = params.size() - 1;
          if(epos != SIZE_MAX) {
            for(size_t i = 0; i != epos; ++i) {
              func << params[i] << ", ";
            }
            func << params[epos];
          }
          func << ')';
        }
        // Create the zero-ary argument getter, which serves two purposes:
        // 0) It is copied as `__varg` whenever its parent function is called with no variadic argument as an optimization.
        // 1) It provides storage for `__file`, `__line` and `__func` for its parent function.
        auto zvarg = ::rocket::make_refcnt<Variadic_Arguer>(sloc, ::rocket::move(func));

        cow_vector<AIR_Node> code_func;
        Analytic_Context ctx_func(ctx_opt, params);
        // Generate IR nodes for the function body.
        epos = body.size() - 1;
        if(epos != SIZE_MAX) {
          for(size_t i = 0; i != epos; ++i) {
            body[i].generate_code(code_func, nullptr, ctx_func, opts, body[i+1].is_empty_return() ? tco_aware_nullify : tco_aware_none);
          }
          body[epos].generate_code(code_func, nullptr, ctx_func, opts, tco_aware_nullify);
        }
        // Optimize the body.
        // Note that `no_optimization`, if set, suppresses ALL optimizations, even those explicitly enabled.
        if(!opts.no_optimization) {
          // TODO: Insert more optimization passes here.
          if(!opts.no_dead_code_elimination) {
            do_dce_recurse(code_func);
          }
        }
        // Solidify IR nodes.
        AVMC_Queue queue;
        do_solidify_queue(queue, code_func);

        // Create the function now.
        return ::rocket::make_refcnt<Instantiated_Function>(params, ::rocket::move(zvarg), ::rocket::move(queue));
      }

    ///////////////////////////////////////////////////////////////////////////
    // Parameter structs and variable enumeration callbacks
    ///////////////////////////////////////////////////////////////////////////

    template<size_t nqsT> struct Params_queues_fixed
      {
        AVMC_Queue queues[nqsT];

        Variable_Callback& enumerate_variables(Variable_Callback& callback) const
          {
            ::rocket::for_each(this->queues, [&](const AVMC_Queue& queue) { queue.enumerate_variables(callback);  });
            return callback;
          }
      };

    struct Params_switch
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

    struct Params_for_each
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

    struct Params_try
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

    struct Params_name
      {
        phsh_string name;

        using nonenumerable = ::std::true_type;
      };

    struct Params_names
      {
        cow_vector<phsh_string> names;

        using nonenumerable = ::std::true_type;
      };

    struct Params_sloc
      {
        Source_Location sloc;

        using nonenumerable = ::std::true_type;
      };

    struct Params_sloc_name
      {
        Source_Location sloc;
        phsh_string name;

        using nonenumerable = ::std::true_type;
      };

    struct Params_sloc_msg
      {
        Source_Location sloc;
        cow_string msg;

        using nonenumerable = ::std::true_type;
      };

    struct Params_func
      {
        AIR_Node::S_instantiate_function xnode;

        using nonenumerable = ::std::true_type;
      };

    struct Params_call
      {
        Source_Location sloc;
        cow_vector<bool> args_by_refs;

        using nonenumerable = ::std::true_type;
      };

    ///////////////////////////////////////////////////////////////////////////
    // Executor functions
    ///////////////////////////////////////////////////////////////////////////

    AIR_Status do_clear_stack(Executive_Context& ctx, ParamU /*paramu*/, const void* /*params*/)
      {
        // Clear the stack.
        ctx.stack().clear();
        return air_status_next;
      }

    AIR_Status do_execute_block(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& queue_body = do_pcast<Params_queues_fixed<1>>(params)->queues[0];

        // Execute the body on a new context.
        return do_execute_block(queue_body, ctx);
      }

    AIR_Status do_declare_variable(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& sloc = do_pcast<Params_sloc_name>(params)->sloc;
        const auto& name = do_pcast<Params_sloc_name>(params)->name;
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

    AIR_Status do_initialize_variable(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& immutable = static_cast<bool>(paramu.u8s[0]);

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

    AIR_Status do_if_statement(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& negative = static_cast<bool>(paramu.u8s[0]);
        const auto& queue_true = do_pcast<Params_queues_fixed<2>>(params)->queues[0];
        const auto& queue_false = do_pcast<Params_queues_fixed<2>>(params)->queues[1];

        // Check the value of the condition.
        if(ctx.stack().get_top().read().test() != negative) {
          // Execute the true branch.
          return do_execute_block(queue_true, ctx);
        }
        // Execute the false branch.
        return do_execute_block(queue_false, ctx);
      }

    AIR_Status do_switch_statement(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& queues_labels = do_pcast<Params_switch>(params)->queues_labels;
        const auto& queues_bodies = do_pcast<Params_switch>(params)->queues_bodies;
        const auto& names_added = do_pcast<Params_switch>(params)->names_added;

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

    AIR_Status do_do_while_statement(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& queue_body = do_pcast<Params_queues_fixed<2>>(params)->queues[0];
        const auto& negative = static_cast<bool>(paramu.u8s[0]);
        const auto& queue_cond = do_pcast<Params_queues_fixed<2>>(params)->queues[1];

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

    AIR_Status do_while_statement(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& negative = static_cast<bool>(paramu.u8s[0]);
        const auto& queue_cond = do_pcast<Params_queues_fixed<2>>(params)->queues[0];
        const auto& queue_body = do_pcast<Params_queues_fixed<2>>(params)->queues[1];

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

    AIR_Status do_for_each_statement(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& name_key = do_pcast<Params_for_each>(params)->name_key;
        const auto& name_mapped = do_pcast<Params_for_each>(params)->name_mapped;
        const auto& queue_init = do_pcast<Params_for_each>(params)->queue_init;
        const auto& queue_body = do_pcast<Params_for_each>(params)->queue_body;

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

    AIR_Status do_for_statement(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& queue_init = do_pcast<Params_queues_fixed<4>>(params)->queues[0];
        const auto& queue_cond = do_pcast<Params_queues_fixed<4>>(params)->queues[1];
        const auto& queue_step = do_pcast<Params_queues_fixed<4>>(params)->queues[2];
        const auto& queue_body = do_pcast<Params_queues_fixed<4>>(params)->queues[3];

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

    AIR_Status do_try_statement(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& queue_try = do_pcast<Params_try>(params)->queue_try;
        const auto& sloc = do_pcast<Params_try>(params)->sloc;
        const auto& name_except = do_pcast<Params_try>(params)->name_except;
        const auto& queue_catch = do_pcast<Params_try>(params)->queue_catch;

        // This is almost identical to JavaScript.
        try {
          // Execute the `try` block. If no exception is thrown, this will have little overhead.
          auto status = do_execute_block(queue_try, ctx);
          if(status == air_status_return) {
            // This cannot be TCO'd, otherwise exceptions thrown from tail calls won't be caught.
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

    AIR_Status do_throw_statement(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& sloc = do_pcast<Params_sloc>(params)->sloc;

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

    AIR_Status do_assert_statement(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& sloc = do_pcast<Params_sloc_msg>(params)->sloc;
        const auto& negative = static_cast<bool>(paramu.u8s[0]);
        const auto& msg = do_pcast<Params_sloc_msg>(params)->msg;

        // Check the value of the condition.
        if(ROCKET_EXPECT(ctx.stack().get_top().read().test() != negative)) {
          // When the assertion succeeds, there is nothing to do.
          return air_status_next;
        }
        // Throw a `runtime_error`.
        ASTERIA_THROW("assertion failure: $1\n[declared at '$2']", msg, sloc);
      }

    AIR_Status do_simple_status(Executive_Context& /*ctx*/, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& status = static_cast<AIR_Status>(paramu.u8s[0]);

        // Return the status as is.
        return status;
      }

    AIR_Status do_return_by_value(Executive_Context& ctx, ParamU /*paramu*/, const void* /*params*/)
      {
        // The result will have been pushed onto the top.
        auto& self = ctx.stack().open_top();
        // Convert the result to an rvalue.
        // TCO wrappers are forwarded as is.
        if(ROCKET_UNEXPECT(self.is_lvalue())) {
          self.convert_to_rvalue();
        }
        return air_status_return;
      }

    AIR_Status do_push_literal(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& val = do_pcast<Value>(params)[0];

        // Push a constant.
        Reference_Root::S_constant xref = { val };
        ctx.stack().push(::rocket::move(xref));
        return air_status_next;
      }

    AIR_Status do_push_global_reference(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& name = do_pcast<Params_name>(params)->name;

        // Look for the name in the global context.
        auto qref = ctx.global().get_named_reference_opt(name);
        if(!qref) {
          ASTERIA_THROW("undeclared identifier `$1`", name);
        }
        // Push a copy of it.
        ctx.stack().push(*qref);
        return air_status_next;
      }

    AIR_Status do_push_local_reference(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& depth = paramu.x32;
        const auto& name = do_pcast<Params_name>(params)->name;

        // Get the context.
        const Executive_Context* qctx = ::std::addressof(ctx);
        ::rocket::ranged_for(uint32_t(0), depth, [&](size_t) { qctx = qctx->get_parent_opt();  });
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

    AIR_Status do_push_bound_reference(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& ref = do_pcast<Reference>(params)[0];

        // Push a copy of the bound reference.
        ctx.stack().push(ref);
        return air_status_next;
      }

    AIR_Status do_instantiate_function(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& xnode = do_pcast<Params_func>(params)->xnode;

        // Instantiate the function.
        auto qtarget = do_instantiate_function(xnode, ::std::addressof(ctx));
        // Push the function as a temporary.
        Reference_Root::S_temporary xref = { G_function(::rocket::move(qtarget)) };
        ctx.stack().push(::rocket::move(xref));
        return air_status_next;
      }

    AIR_Status do_branch_expression(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);
        const auto& queue_true = do_pcast<Params_queues_fixed<2>>(params)->queues[0];
        const auto& queue_false = do_pcast<Params_queues_fixed<2>>(params)->queues[1];

        // Check the value of the condition.
        if(ctx.stack().get_top().read().test() != false) {
          // Evaluate the true branch.
          return do_evaluate_branch(queue_true, assign, ctx);
        }
        // Evaluate the false branch.
        return do_evaluate_branch(queue_false, assign, ctx);
      }

    AIR_Status do_coalescence(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);
        const auto& queue_null = do_pcast<Params_queues_fixed<1>>(params)->queues[0];

        // Check the value of the condition.
        if(ctx.stack().get_top().read().is_null() != false) {
          // Evaluate the alternative.
          return do_evaluate_branch(queue_null, assign, ctx);
        }
        // Leave the condition on the stack.
        return air_status_next;
      }

    AIR_Status do_function_call(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& sloc = do_pcast<Params_call>(params)->sloc;
        const auto& args_by_refs = do_pcast<Params_call>(params)->args_by_refs;
        const auto& tco_aware = static_cast<TCO_Aware>(paramu.u8s[0]);
        const auto& inside = ctx.zvarg().func();
        const auto& qhooks = ctx.global().get_hooks_opt();

        // Check for stack overflows.
        const auto sentry = ctx.global().copy_recursion_sentry();
        // Generate a single-step trap.
        if(qhooks) {
          qhooks->on_single_step_trap(sloc, inside, &ctx);
        }
        // Pop arguments off the stack backwards.
        cow_vector<Reference> args;
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
        // Get the target reference.
        auto& self = ctx.stack().open_top();
        // Copy the target value, which shall be of type `function`.
        const auto value = self.read();
        if(!value.is_function()) {
          ASTERIA_THROW("attempt to call a non-function (value `$1`)", value);
        }
        const auto& target = value.as_function();
        // Initialize the `this` reference.
        self.zoom_out();

        // Call the function now.
        if(tco_aware != tco_aware_none) {
          // Pack arguments for this proper tail call.
          args.emplace_back(::rocket::move(self));
          // Create a TCO wrapper.
          auto tca = ::rocket::make_refcnt<Tail_Call_Arguments>(sloc, inside, tco_aware, target, ::rocket::move(args));
          // Return it.
          Reference_Root::S_tail_call xref = { ::rocket::move(tca) };
          self = ::rocket::move(xref);
          // Force `air_status_return` if control flow reaches the end of a function.
          // Otherwise a null reference is returned instead of this TCO wrapper, which can then never be unpacked.
          return air_status_return;
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
          // Append the current frame and rethrow the exception.
          except.push_frame_func(sloc, inside);
          // Call the hook function if any.
          if(qhooks) {
            qhooks->on_function_except(sloc, inside, except);
          }
          throw;
        }
        catch(::std::exception& stdex) {
          // Translate the exception, append the current frame, and throw the new exception.
          Runtime_Error except(stdex);
          except.push_frame_func(sloc, inside);
          // Call the hook function if any.
          if(qhooks) {
            qhooks->on_function_except(sloc, inside, except);
          }
          throw except;
        }
        // Call the hook function if any.
        if(qhooks) {
          qhooks->on_function_return(sloc, inside, self);
        }
        return air_status_next;
      }

    AIR_Status do_member_access(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& name = do_pcast<Params_name>(params)->name;

        // Append a modifier to the reference at the top.
        Reference_Modifier::S_object_key xmod = { name };
        ctx.stack().open_top().zoom_in(::rocket::move(xmod));
        return air_status_next;
      }

    AIR_Status do_push_unnamed_array(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& nelems = paramu.x32;

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

    AIR_Status do_push_unnamed_object(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& keys = do_pcast<Params_names>(params)->names;

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

    ROCKET_PURE_FUNCTION G_boolean do_operator_not(const G_boolean& rhs)
      {
        return !rhs;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_and(const G_boolean& lhs, const G_boolean& rhs)
      {
        return lhs & rhs;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_or(const G_boolean& lhs, const G_boolean& rhs)
      {
        return lhs | rhs;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_xor(const G_boolean& lhs, const G_boolean& rhs)
      {
        return lhs ^ rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_neg(const G_integer& rhs)
      {
        if(rhs == INT64_MIN) {
          ASTERIA_THROW("integer negation overflow (operand was `$1`)", rhs);
        }
        return -rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sqrt(const G_integer& rhs)
      {
        return ::std::sqrt(G_real(rhs));
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isinf(const G_integer& /*rhs*/)
      {
        return false;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isnan(const G_integer& /*rhs*/)
      {
        return false;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_abs(const G_integer& rhs)
      {
        if(rhs == INT64_MIN) {
          ASTERIA_THROW("integer absolute value overflow (operand was `$1`)", rhs);
        }
        return ::std::abs(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sign(const G_integer& rhs)
      {
        return rhs >> 63;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_add(const G_integer& lhs, const G_integer& rhs)
      {
        if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
          ASTERIA_THROW("integer addition overflow (operands were `$1` and `$2`)", lhs, rhs);
        }
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sub(const G_integer& lhs, const G_integer& rhs)
      {
        if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
          ASTERIA_THROW("integer subtraction overflow (operands were `$1` and `$2`)", lhs, rhs);
        }
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_mul(const G_integer& lhs, const G_integer& rhs)
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

    ROCKET_PURE_FUNCTION G_integer do_operator_div(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW("integer divided by zero (operands were `$1` and `$2`)", lhs, rhs);
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          ASTERIA_THROW("integer division overflow (operands were `$1` and `$2`)", lhs, rhs);
        }
        return lhs / rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_mod(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW("integer divided by zero (operands were `$1` and `$2`)", lhs, rhs);
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          ASTERIA_THROW("integer division overflow (operands were `$1` and `$2`)", lhs, rhs);
        }
        return lhs % rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sll(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
        }
        if(rhs >= 64) {
          return 0;
        }
        return G_integer(static_cast<uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_srl(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
        }
        if(rhs >= 64) {
          return 0;
        }
        return G_integer(static_cast<uint64_t>(lhs) >> rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sla(const G_integer& lhs, const G_integer& rhs)
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

    ROCKET_PURE_FUNCTION G_integer do_operator_sra(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW("negative shift count (operands were `$1` and `$2`)", lhs, rhs);
        }
        if(rhs >= 64) {
          return lhs >> 63;
        }
        return lhs >> rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_not(const G_integer& rhs)
      {
        return ~rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_and(const G_integer& lhs, const G_integer& rhs)
      {
        return lhs & rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_or(const G_integer& lhs, const G_integer& rhs)
      {
        return lhs | rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_xor(const G_integer& lhs, const G_integer& rhs)
      {
        return lhs ^ rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_neg(const G_real& rhs)
      {
        return -rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sqrt(const G_real& rhs)
      {
        return ::std::sqrt(rhs);
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isinf(const G_real& rhs)
      {
        return ::std::isinf(rhs);
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isnan(const G_real& rhs)
      {
        return ::std::isnan(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_abs(const G_real& rhs)
      {
        return ::std::fabs(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sign(const G_real& rhs)
      {
        return ::std::signbit(rhs) ? -1 : 0;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_round(const G_real& rhs)
      {
        return ::std::round(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_floor(const G_real& rhs)
      {
        return ::std::floor(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_ceil(const G_real& rhs)
      {
        return ::std::ceil(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_trunc(const G_real& rhs)
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

    ROCKET_PURE_FUNCTION G_integer do_operator_iround(const G_real& rhs)
      {
        return do_cast_to_integer(::std::round(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_ifloor(const G_real& rhs)
      {
        return do_cast_to_integer(::std::floor(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_iceil(const G_real& rhs)
      {
        return do_cast_to_integer(::std::ceil(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_itrunc(const G_real& rhs)
      {
        return do_cast_to_integer(::std::trunc(rhs));
      }

    ROCKET_PURE_FUNCTION G_real do_operator_add(const G_real& lhs, const G_real& rhs)
      {
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sub(const G_real& lhs, const G_real& rhs)
      {
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_mul(const G_real& lhs, const G_real& rhs)
      {
        return lhs * rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_div(const G_real& lhs, const G_real& rhs)
      {
        return lhs / rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_mod(const G_real& lhs, const G_real& rhs)
      {
        return ::std::fmod(lhs, rhs);
      }

    ROCKET_PURE_FUNCTION G_string do_operator_add(const G_string& lhs, const G_string& rhs)
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

    ROCKET_PURE_FUNCTION G_string do_operator_mul(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW("negative duplicate count (operands were `$1` and `$2`)", lhs, rhs);
        }
        return do_duplicate_string(lhs, static_cast<uint64_t>(rhs));
      }

    ROCKET_PURE_FUNCTION G_string do_operator_mul(const G_integer& lhs, const G_string& rhs)
      {
        if(lhs < 0) {
          ASTERIA_THROW("negative duplicate count (operands were `$1` and `$2`)", lhs, rhs);
        }
        return do_duplicate_string(rhs, static_cast<uint64_t>(lhs));
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sll(const G_string& lhs, const G_integer& rhs)
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

    ROCKET_PURE_FUNCTION G_string do_operator_srl(const G_string& lhs, const G_integer& rhs)
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

    ROCKET_PURE_FUNCTION G_string do_operator_sla(const G_string& lhs, const G_integer& rhs)
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

    ROCKET_PURE_FUNCTION G_string do_operator_sra(const G_string& lhs, const G_integer& rhs)
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

    AIR_Status do_apply_xop_inc_post(Executive_Context& ctx, ParamU /*paramu*/, const void* /*params*/)
      {
        // This operator is unary.
        auto& lhs = ctx.stack().get_top().open();
        // Increment the operand and return the old value. `assign` is ignored.
        if(lhs.is_integer()) {
          auto& reg = lhs.open_integer();
          do_set_temporary(ctx.stack(), false, ::rocket::move(lhs));
          reg = do_operator_add(reg, G_integer(1));
        }
        else if(lhs.is_real()) {
          auto& reg = lhs.open_real();
          do_set_temporary(ctx.stack(), false, ::rocket::move(lhs));
          reg = do_operator_add(reg, G_real(1));
        }
        else {
          ASTERIA_THROW("postfix increment not applicable (operand was `$1`)", lhs);
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_dec_post(Executive_Context& ctx, ParamU /*paramu*/, const void* /*params*/)
      {
        // This operator is unary.
        auto& lhs = ctx.stack().get_top().open();
        // Decrement the operand and return the old value. `assign` is ignored.
        if(lhs.is_integer()) {
          auto& reg = lhs.open_integer();
          do_set_temporary(ctx.stack(), false, ::rocket::move(lhs));
          reg = do_operator_sub(reg, G_integer(1));
        }
        else if(lhs.is_real()) {
          auto& reg = lhs.open_real();
          do_set_temporary(ctx.stack(), false, ::rocket::move(lhs));
          reg = do_operator_sub(reg, G_real(1));
        }
        else {
          ASTERIA_THROW("postfix decrement not applicable (operand was `$1`)", lhs);
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_subscr(Executive_Context& ctx, ParamU /*paramu*/, const void* /*params*/)
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

    AIR_Status do_apply_xop_pos(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Copy the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_neg(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Get the opposite of the operand as a temporary value, then return it.
        if(rhs.is_integer()) {
          auto& reg = rhs.open_integer();
          reg = do_operator_neg(reg);
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.open_real();
          reg = do_operator_neg(reg);
        }
        else {
          ASTERIA_THROW("prefix negation not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_notb(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Perform bitwise NOT operation on the operand to create a temporary value, then return it.
        if(rhs.is_boolean()) {
          auto& reg = rhs.open_boolean();
          reg = do_operator_not(reg);
        }
        else if(rhs.is_integer()) {
          auto& reg = rhs.open_integer();
          reg = do_operator_not(reg);
        }
        else {
          ASTERIA_THROW("prefix bitwise NOT not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_notl(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        const auto& rhs = ctx.stack().get_top().read();
        // Perform logical NOT operation on the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        do_set_temporary(ctx.stack(), assign, do_operator_not(rhs.test()));
        return air_status_next;
      }

    AIR_Status do_apply_xop_inc_pre(Executive_Context& ctx, ParamU /*paramu*/, const void* /*params*/)
      {
        // This operator is unary.
        auto& rhs = ctx.stack().get_top().open();
        // Increment the operand and return it. `assign` is ignored.
        if(rhs.is_integer()) {
          auto& reg = rhs.open_integer();
          reg = do_operator_add(reg, G_integer(1));
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.open_real();
          reg = do_operator_add(reg, G_real(1));
        }
        else {
          ASTERIA_THROW("prefix increment not applicable (operand was `$1`)", rhs);
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_dec_pre(Executive_Context& ctx, ParamU /*paramu*/, const void* /*params*/)
      {
        // This operator is unary.
        auto& rhs = ctx.stack().get_top().open();
        // Decrement the operand and return it. `assign` is ignored.
        if(rhs.is_integer()) {
          auto& reg = rhs.open_integer();
          reg = do_operator_sub(reg, G_integer(1));
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.open_real();
          reg = do_operator_sub(reg, G_real(1));
        }
        else {
          ASTERIA_THROW("prefix decrement not applicable (operand was `$1`)", rhs);
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_unset(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().unset();
        // Unset the reference and return the old value.
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_lengthof(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        const auto& rhs = ctx.stack().get_top().read();
        // Return the number of elements in the operand.
        size_t nelems;
        switch(::rocket::weaken_enum(rhs.gtype())) {
          {{
        case gtype_null:
            nelems = 0;
            break;
          }{
        case gtype_string:
            nelems = rhs.as_string().size();
            break;
          }{
        case gtype_array:
            nelems = rhs.as_array().size();
            break;
          }{
        case gtype_object:
            nelems = rhs.as_object().size();
            break;
          }}
        default:
          ASTERIA_THROW("prefix `lengthof` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, G_integer(nelems));
        return air_status_next;
      }

    AIR_Status do_apply_xop_typeof(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        const auto& rhs = ctx.stack().get_top().read();
        // Return the type name of the operand.
        // N.B. This is one of the few operators that work on all types.
        do_set_temporary(ctx.stack(), assign, G_string(::rocket::sref(rhs.what_gtype())));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sqrt(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Get the square root of the operand as a temporary value, then return it.
        if(rhs.is_integer()) {
          // Note that `rhs` does not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_sqrt(rhs.as_integer());
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.open_real();
          reg = do_operator_sqrt(reg);
        }
        else {
          ASTERIA_THROW("prefix `__sqrt` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_isnan(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Check whether the operand is a NaN, store the result in a temporary value, then return it.
        if(rhs.is_integer()) {
          // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
          rhs = do_operator_isnan(rhs.as_integer());
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
          rhs = do_operator_isnan(rhs.as_real());
        }
        else {
          ASTERIA_THROW("prefix `__isnan` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_isinf(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Check whether the operand is an infinity, store the result in a temporary value, then return it.
        if(rhs.is_integer()) {
          // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
          rhs = do_operator_isinf(rhs.as_integer());
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
          rhs = do_operator_isinf(rhs.as_real());
        }
        else {
          ASTERIA_THROW("prefix `__isinf` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_abs(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Get the absolute value of the operand as a temporary value, then return it.
        if(rhs.is_integer()) {
          auto& reg = rhs.open_integer();
          reg = do_operator_abs(reg);
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.open_real();
          reg = do_operator_abs(reg);
        }
        else {
          ASTERIA_THROW("prefix `__abs` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sign(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Get the sign bit of the operand as a temporary value, then return it.
        if(rhs.is_integer()) {
          auto& reg = rhs.open_integer();
          reg = do_operator_sign(reg);
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_sign(rhs.as_real());
        }
        else {
          ASTERIA_THROW("prefix `__sign` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_round(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Round the operand to the nearest integer as a temporary value, then return it.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.open_real();
          reg = do_operator_round(reg);
        }
        else {
          ASTERIA_THROW("prefix `__round` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_floor(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Round the operand towards negative infinity as a temporary value, then return it.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.open_real();
          reg = do_operator_floor(reg);
        }
        else {
          ASTERIA_THROW("prefix `__floor` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_ceil(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Round the operand towards negative infinity as a temporary value, then return it.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.open_real();
          reg = do_operator_ceil(reg);
        }
        else {
          ASTERIA_THROW("prefix `__ceil` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_trunc(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Round the operand towards negative infinity as a temporary value, then return it.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.open_real();
          reg = do_operator_trunc(reg);
        }
        else {
          ASTERIA_THROW("prefix `__trunc` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_iround(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Round the operand to the nearest integer as a temporary value, then return it as an `integer`.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_iround(rhs.as_real());
        }
        else {
          ASTERIA_THROW("prefix `__iround` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_ifloor(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_ifloor(rhs.as_real());
        }
        else {
          ASTERIA_THROW("prefix `__ifloor` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_iceil(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_iceil(rhs.as_real());
        }
        else {
          ASTERIA_THROW("prefix `__iceil` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_itrunc(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is unary.
        auto rhs = ctx.stack().get_top().read();
        // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_itrunc(rhs.as_real());
        }
        else {
          ASTERIA_THROW("prefix `__itrunc` not applicable (operand was `$1`)", rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_cmp_xeq(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);
        const auto& expect = static_cast<Compare>(paramu.u8s[1]);
        const auto& negative = static_cast<bool>(paramu.u8s[2]);

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

    AIR_Status do_apply_xop_cmp_xrel(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);
        const auto& expect = static_cast<Compare>(paramu.u8s[1]);
        const auto& negative = static_cast<bool>(paramu.u8s[2]);

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

    AIR_Status do_apply_xop_cmp_3way(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        switch(comp) {
          {{
        case compare_greater:
            do_set_temporary(ctx.stack(), assign, G_integer(+1));
            break;
          }{
        case compare_less:
            do_set_temporary(ctx.stack(), assign, G_integer(-1));
            break;
          }{
        case compare_equal:
            do_set_temporary(ctx.stack(), assign, G_integer(0));
            break;
          }{
        case compare_unordered:
            do_set_temporary(ctx.stack(), assign, G_string(::rocket::sref("<unordered>")));
            break;
          }}
        default:
          ROCKET_ASSERT(false);
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_add(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical OR'd result of both operands.
          auto& reg = rhs.open_boolean();
          reg = do_operator_or(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the sum of both operands.
          auto& reg = rhs.open_integer();
          reg = do_operator_add(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_add(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else if(lhs.is_string() && rhs.is_string()) {
          // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
          auto& reg = rhs.open_string();
          reg = do_operator_add(lhs.as_string(), reg);
        }
        else {
          ASTERIA_THROW("infix addition not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sub(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical XOR'd result of both operands.
          auto& reg = rhs.open_boolean();
          reg = do_operator_xor(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the difference of both operands.
          auto& reg = rhs.open_integer();
          reg = do_operator_sub(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_sub(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else {
          ASTERIA_THROW("infix subtraction not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_mul(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical AND'd result of both operands.
          auto& reg = rhs.open_boolean();
          reg = do_operator_and(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the product of both operands.
          auto& reg = rhs.open_integer();
          reg = do_operator_mul(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_mul(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If either operand has type `string` and the other has type `integer`, duplicate the string up to the specified number of times and return the result.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_mul(lhs.as_string(), rhs.as_integer());
        }
        else if(lhs.is_integer() && rhs.is_string()) {
          auto& reg = rhs.open_string();
          reg = do_operator_mul(lhs.as_integer(), reg);
        }
        else {
          ASTERIA_THROW("infix multiplication not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_div(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the quotient of both operands.
          auto& reg = rhs.open_integer();
          reg = do_operator_div(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_div(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else {
          ASTERIA_THROW("infix division not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_mod(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the remainder of both operands.
          auto& reg = rhs.open_integer();
          reg = do_operator_mod(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_mod(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else {
          ASTERIA_THROW("infix modulo not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sll(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // If the LHS operand has type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
          // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
          auto& reg = rhs.open_integer();
          reg = do_operator_sll(lhs.as_integer(), reg);
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If the LHS operand has type `string`, fill space characters in the right and discard characters from the left.
          // The number of bytes in the LHS operand will be preserved.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_sll(lhs.as_string(), rhs.as_integer());
        }
        else {
          ASTERIA_THROW("infix logical left shift not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_srl(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // If the LHS operand has type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
          // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
          auto& reg = rhs.open_integer();
          reg = do_operator_srl(lhs.as_integer(), reg);
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If the LHS operand has type `string`, fill space characters in the left and discard characters from the right.
          // The number of bytes in the LHS operand will be preserved.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_srl(lhs.as_string(), rhs.as_integer());
        }
        else {
          ASTERIA_THROW("infix logical right shift not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sla(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // If the LHS operand is of type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
          // Bits shifted out that are equal to the sign bit are discarded. Bits shifted in are filled with zeroes.
          // If any bits that are different from the sign bit would be shifted out, an exception is thrown.
          auto& reg = rhs.open_integer();
          reg = do_operator_sla(lhs.as_integer(), reg);
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If the LHS operand has type `string`, fill space characters in the right.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_sla(lhs.as_string(), rhs.as_integer());
        }
        else {
          ASTERIA_THROW("infix arithmetic left shift not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sra(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // If the LHS operand is of type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
          // Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
          auto& reg = rhs.open_integer();
          reg = do_operator_sra(lhs.as_integer(), reg);
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If the LHS operand has type `string`, discard characters from the right.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_sra(lhs.as_string(), rhs.as_integer());
        }
        else {
          ASTERIA_THROW("infix arithmetic right shift not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_andb(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical AND'd result of both operands.
          auto& reg = rhs.open_boolean();
          reg = do_operator_and(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` type, return bitwise AND'd result of both operands.
          auto& reg = rhs.open_integer();
          reg = do_operator_and(lhs.as_integer(), reg);
        }
        else {
          ASTERIA_THROW("infix bitwise AND not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_orb(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical OR'd result of both operands.
          auto& reg = rhs.open_boolean();
          reg = do_operator_or(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` type, return bitwise OR'd result of both operands.
          auto& reg = rhs.open_integer();
          reg = do_operator_or(lhs.as_integer(), reg);
        }
        else {
          ASTERIA_THROW("infix bitwise OR not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_xorb(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

        // This operator is binary.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().get_top().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical XOR'd result of both operands.
          auto& reg = rhs.open_boolean();
          reg = do_operator_xor(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` type, return bitwise XOR'd result of both operands.
          auto& reg = rhs.open_integer();
          reg = do_operator_xor(lhs.as_integer(), reg);
        }
        else {
          ASTERIA_THROW("infix bitwise XOR not defined (operands were `$1` and `$2`)", lhs, rhs);
        }
        do_set_temporary(ctx.stack(), assign, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_assign(Executive_Context& ctx, ParamU /*paramu*/, const void* /*params*/)
      {
        // Pop the RHS operand.
        auto rhs = ctx.stack().get_top().read();
        ctx.stack().pop();
        // Copy the value to the LHS operand which is write-only. `assign` is ignored.
        do_set_temporary(ctx.stack(), true, ::rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_fma(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& assign = static_cast<bool>(paramu.u8s[0]);

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

    AIR_Status do_apply_xop_tail(Executive_Context& ctx, ParamU /*paramu*/, const void* /*params*/)
      {
        // This operator is unary.
        auto& lref = ctx.stack().open_top();
        Reference_Modifier::S_array_tail xmod = { };
        lref.zoom_in(::rocket::move(xmod));
        return air_status_next;
      }

    AIR_Status do_unpack_struct_array(Executive_Context& ctx, ParamU paramu, const void* /*params*/)
      {
        // Unpack arguments.
        const auto& immutable = static_cast<bool>(paramu.y8s[0]);
        const auto& nelems = paramu.y32;

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

    AIR_Status do_unpack_struct_object(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& immutable = static_cast<bool>(paramu.u8s[0]);
        const auto& keys = do_pcast<Params_names>(params)->names;

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

    AIR_Status do_define_null_variable(Executive_Context& ctx, ParamU paramu, const void* params)
      {
        // Unpack arguments.
        const auto& immutable = static_cast<bool>(paramu.u8s[0]);
        const auto& sloc = do_pcast<Params_sloc_name>(params)->sloc;
        const auto& name = do_pcast<Params_sloc_name>(params)->name;
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

    AIR_Status do_single_step_trap(Executive_Context& ctx, ParamU /*paramu*/, const void* params)
      {
        // Unpack arguments.
        const auto& sloc = do_pcast<Params_sloc>(params)->sloc;
        const auto& inside = ctx.zvarg().func();
        const auto& qhooks = ctx.global().get_hooks_opt();

        // Call the hook function if any.
        if(qhooks) {
          qhooks->on_single_step_trap(sloc, inside, &ctx);
        }
        return air_status_next;
      }

    }

AVMC_Queue& AIR_Node::solidify(AVMC_Queue& queue, uint8_t ipass) const
  {
    switch(this->index()) {
      {{
    case index_clear_stack:
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
      }{
    case index_execute_block:
        const auto& altr = this->m_stor.as<index_execute_block>();
        // `paramu` is unused.
        // `params` points to the body.
        AVMC_Appender<Params_queues_fixed<1>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queues[0], altr.code_body);
        // Push a new node.
        return avmcp.output<do_execute_block>(queue);
      }{
    case index_declare_variable:
        const auto& altr = this->m_stor.as<index_declare_variable>();
        // `paramu` is unused.
        // `params` points to the source location and name.
        AVMC_Appender<Params_sloc_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_declare_variable>(queue);
      }{
    case index_initialize_variable:
        const auto& altr = this->m_stor.as<index_initialize_variable>();
        // `paramu.u8s[0]` is `immutable`.
        // `params` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.paramu.u8s[0] = altr.immutable;
        // Push a new node.
        return avmcp.output<do_initialize_variable>(queue);
      }{
    case index_if_statement:
        const auto& altr = this->m_stor.as<index_if_statement>();
        // `paramu.u8s[0]` is `negative`.
        // `params` points to the two branches.
        AVMC_Appender<Params_queues_fixed<2>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.paramu.u8s[0] = altr.negative;
        do_solidify_queue(avmcp.queues[0], altr.code_true);
        do_solidify_queue(avmcp.queues[1], altr.code_false);
        // Push a new node.
        return avmcp.output<do_if_statement>(queue);
      }{
    case index_switch_statement:
        const auto& altr = this->m_stor.as<index_switch_statement>();
        // `paramu` is unused.
        // `params` points to all clauses.
        AVMC_Appender<Params_switch> avmcp;
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
      }{
    case index_do_while_statement:
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        // `paramu.u8s[0]` is `negative`.
        // `params` points to the body and the condition.
        AVMC_Appender<Params_queues_fixed<2>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queues[0], altr.code_body);
        avmcp.paramu.u8s[0] = altr.negative;
        do_solidify_queue(avmcp.queues[1], altr.code_cond);
        // Push a new node.
        return avmcp.output<do_do_while_statement>(queue);
      }{
    case index_while_statement:
        const auto& altr = this->m_stor.as<index_while_statement>();
        // `paramu.u8s[0]` is `negative`.
        // `params` points to the condition and the body.
        AVMC_Appender<Params_queues_fixed<2>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.paramu.u8s[0] = altr.negative;
        do_solidify_queue(avmcp.queues[0], altr.code_cond);
        do_solidify_queue(avmcp.queues[1], altr.code_body);
        // Push a new node.
        return avmcp.output<do_while_statement>(queue);
      }{
    case index_for_each_statement:
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        // `paramu` is unused.
        // `params` points to the range initializer and the body.
        AVMC_Appender<Params_for_each> avmcp;
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
      }{
    case index_for_statement:
        const auto& altr = this->m_stor.as<index_for_statement>();
        // `paramu` is unused.
        // `params` points to the triplet and the body.
        AVMC_Appender<Params_queues_fixed<4>> avmcp;
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
      }{
    case index_try_statement:
        const auto& altr = this->m_stor.as<index_try_statement>();
        // `paramu` is unused.
        // `params` points to the clauses.
        AVMC_Appender<Params_try> avmcp;
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
      }{
    case index_throw_statement:
        const auto& altr = this->m_stor.as<index_throw_statement>();
        // `paramu` is unused.
        // `params` points to the source location.
        AVMC_Appender<Params_sloc> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        // Push a new node.
        return avmcp.output<do_throw_statement>(queue);
      }{
    case index_assert_statement:
        const auto& altr = this->m_stor.as<index_assert_statement>();
        // `paramu.u8s[0]` is `negative`.
        // `params` points to the source location and the message.
        AVMC_Appender<Params_sloc_msg> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.paramu.u8s[0] = altr.negative;
        avmcp.msg = altr.msg;
        // Push a new node.
        return avmcp.output<do_assert_statement>(queue);
      }{
    case index_simple_status:
        const auto& altr = this->m_stor.as<index_simple_status>();
        // `paramu.u8s[0] ` is `status`.
        // `params` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.paramu.u8s[0] = altr.status;
        // Push a new node.
        return avmcp.output<do_simple_status>(queue);
      }{
    case index_return_by_value:
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
      }{
    case index_push_literal:
        const auto& altr = this->m_stor.as<index_push_literal>();
        // `paramu` is unused.
        // `params` points to a copy of `val`.
        AVMC_Appender<Value> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        static_cast<Value&>(avmcp) = altr.val;
        // Push a new node.
        return avmcp.output<do_push_literal>(queue);
      }{
    case index_push_global_reference:
        const auto& altr = this->m_stor.as<index_push_global_reference>();
        // `paramu` is unused.
        // `params` points to the name.
        AVMC_Appender<Params_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_push_global_reference>(queue);
      }{
    case index_push_local_reference:
        const auto& altr = this->m_stor.as<index_push_local_reference>();
        // `paramu.x32` is `depth`.
        // `params` points to the name.
        AVMC_Appender<Params_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.paramu.x32 = altr.depth;
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_push_local_reference>(queue);
      }{
    case index_push_bound_reference:
        const auto& altr = this->m_stor.as<index_push_bound_reference>();
        // `paramu` is unused.
        // `params` points to a copy of `ref`.
        AVMC_Appender<Reference> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        static_cast<Reference&>(avmcp) = altr.ref;
        // Push a new node.
        return avmcp.output<do_push_bound_reference>(queue);
      }{
    case index_instantiate_function:
        const auto& altr = this->m_stor.as<index_instantiate_function>();
        // `paramu` is unused.
        // `params` points to the name, the parameter list, and the body of the function.
        AVMC_Appender<Params_func> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.xnode = altr;
        // Push a new node.
        return avmcp.output<do_instantiate_function>(queue);
      }{
    case index_branch_expression:
        const auto& altr = this->m_stor.as<index_branch_expression>();
        // `param.u8s[0]` is `assign`.
        // `params` points to the two branches.
        AVMC_Appender<Params_queues_fixed<2>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queues[0], altr.code_true);
        do_solidify_queue(avmcp.queues[1], altr.code_false);
        avmcp.paramu.u8s[0] = altr.assign;
        // Push a new node.
        return avmcp.output<do_branch_expression>(queue);
      }{
    case index_coalescence:
        const auto& altr = this->m_stor.as<index_coalescence>();
        // `param.u8s[0]` is `assign`.
        // `params` points to the alternative.
        AVMC_Appender<Params_queues_fixed<1>> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        do_solidify_queue(avmcp.queues[0], altr.code_null);
        avmcp.paramu.u8s[0] = altr.assign;
        // Push a new node.
        return avmcp.output<do_coalescence>(queue);
      }{
    case index_function_call:
        const auto& altr = this->m_stor.as<index_function_call>();
        // `paramu.u8s[0]` is `tco_aware`.
        // `params` points to the source location and the argument avmcp.cifier vector.
        AVMC_Appender<Params_call> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        avmcp.args_by_refs = altr.args_by_refs;
        avmcp.paramu.u8s[0] = altr.tco_aware;
        // Push a new node.
        return avmcp.output<do_function_call>(queue);
      }{
    case index_member_access:
        const auto& altr = this->m_stor.as<index_member_access>();
        // `paramu` is unused.
        // `params` points to the name.
        AVMC_Appender<Params_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_member_access>(queue);
      }{
    case index_push_unnamed_array:
        const auto& altr = this->m_stor.as<index_push_unnamed_array>();
        // `paramu.x32` is `nelems`.
        // `params` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.paramu.x32 = altr.nelems;
        // Push a new node.
        return avmcp.output<do_push_unnamed_array>(queue);
      }{
    case index_push_unnamed_object:
        const auto& altr = this->m_stor.as<index_push_unnamed_object>();
        // `paramu` is unused.
        // `params` points to the keys.
        AVMC_Appender<Params_names> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.names = altr.keys;
        // Push a new node.
        return avmcp.output<do_push_unnamed_object>(queue);
      }{
    case index_apply_operator:
        const auto& altr = this->m_stor.as<index_apply_operator>();
        // `paramu.u8s[0]` is `assign`. Other fields may be used depending on the operator.
        // `params` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        switch(::rocket::weaken_enum(altr.xop)) {
          {{
        case xop_cmp_eq:
            avmcp.paramu.u8s[0] = altr.assign;
            avmcp.paramu.u8s[1] = compare_equal;
            avmcp.paramu.u8s[2] = false;
            break;
          }{
        case xop_cmp_ne:
            avmcp.paramu.u8s[0] = altr.assign;
            avmcp.paramu.u8s[1] = compare_equal;
            avmcp.paramu.u8s[2] = true;
            break;
          }{
        case xop_cmp_lt:
            avmcp.paramu.u8s[0] = altr.assign;
            avmcp.paramu.u8s[1] = compare_less;
            avmcp.paramu.u8s[2] = false;
            break;
          }{
        case xop_cmp_gt:
            avmcp.paramu.u8s[0] = altr.assign;
            avmcp.paramu.u8s[1] = compare_greater;
            avmcp.paramu.u8s[2] = false;
            break;
          }{
        case xop_cmp_lte:
            avmcp.paramu.u8s[0] = altr.assign;
            avmcp.paramu.u8s[1] = compare_greater;
            avmcp.paramu.u8s[2] = true;
            break;
          }{
        case xop_cmp_gte:
            avmcp.paramu.u8s[0] = altr.assign;
            avmcp.paramu.u8s[1] = compare_less;
            avmcp.paramu.u8s[2] = true;
            break;
          }{
        default:
            avmcp.paramu.u8s[0] = altr.assign;
            break;
          }}
        }
        // Push a new node.
        switch(altr.xop) {
          {{
        case xop_inc_post:
            return avmcp.output<do_apply_xop_inc_post>(queue);
          }{
        case xop_dec_post:
            return avmcp.output<do_apply_xop_dec_post>(queue);
          }{
        case xop_subscr:
            return avmcp.output<do_apply_xop_subscr>(queue);
          }{
        case xop_pos:
            return avmcp.output<do_apply_xop_pos>(queue);
          }{
        case xop_neg:
            return avmcp.output<do_apply_xop_neg>(queue);
          }{
        case xop_notb:
            return avmcp.output<do_apply_xop_notb>(queue);
          }{
        case xop_notl:
            return avmcp.output<do_apply_xop_notl>(queue);
          }{
        case xop_inc_pre:
            return avmcp.output<do_apply_xop_inc_pre>(queue);
          }{
        case xop_dec_pre:
            return avmcp.output<do_apply_xop_dec_pre>(queue);
          }{
        case xop_unset:
            return avmcp.output<do_apply_xop_unset>(queue);
          }{
        case xop_lengthof:
            return avmcp.output<do_apply_xop_lengthof>(queue);
          }{
        case xop_typeof:
            return avmcp.output<do_apply_xop_typeof>(queue);
          }{
        case xop_sqrt:
            return avmcp.output<do_apply_xop_sqrt>(queue);
          }{
        case xop_isnan:
            return avmcp.output<do_apply_xop_isnan>(queue);
          }{
        case xop_isinf:
            return avmcp.output<do_apply_xop_isinf>(queue);
          }{
        case xop_abs:
            return avmcp.output<do_apply_xop_abs>(queue);
          }{
        case xop_sign:
            return avmcp.output<do_apply_xop_sign>(queue);
          }{
        case xop_round:
            return avmcp.output<do_apply_xop_round>(queue);
          }{
        case xop_floor:
            return avmcp.output<do_apply_xop_floor>(queue);
          }{
        case xop_ceil:
            return avmcp.output<do_apply_xop_ceil>(queue);
          }{
        case xop_trunc:
            return avmcp.output<do_apply_xop_trunc>(queue);
          }{
        case xop_iround:
            return avmcp.output<do_apply_xop_iround>(queue);
          }{
        case xop_ifloor:
            return avmcp.output<do_apply_xop_ifloor>(queue);
          }{
        case xop_iceil:
            return avmcp.output<do_apply_xop_iceil>(queue);
          }{
        case xop_itrunc:
            return avmcp.output<do_apply_xop_itrunc>(queue);
          }{
        case xop_cmp_eq:
            return avmcp.output<do_apply_xop_cmp_xeq>(queue);
          }{
        case xop_cmp_ne:
            return avmcp.output<do_apply_xop_cmp_xeq>(queue);
          }{
        case xop_cmp_lt:
            return avmcp.output<do_apply_xop_cmp_xrel>(queue);
          }{
        case xop_cmp_gt:
            return avmcp.output<do_apply_xop_cmp_xrel>(queue);
          }{
        case xop_cmp_lte:
            return avmcp.output<do_apply_xop_cmp_xrel>(queue);
          }{
        case xop_cmp_gte:
            return avmcp.output<do_apply_xop_cmp_xrel>(queue);
          }{
        case xop_cmp_3way:
            return avmcp.output<do_apply_xop_cmp_3way>(queue);
          }{
        case xop_add:
            return avmcp.output<do_apply_xop_add>(queue);
          }{
        case xop_sub:
            return avmcp.output<do_apply_xop_sub>(queue);
          }{
        case xop_mul:
            return avmcp.output<do_apply_xop_mul>(queue);
          }{
        case xop_div:
            return avmcp.output<do_apply_xop_div>(queue);
          }{
        case xop_mod:
            return avmcp.output<do_apply_xop_mod>(queue);
          }{
        case xop_sll:
            return avmcp.output<do_apply_xop_sll>(queue);
          }{
        case xop_srl:
            return avmcp.output<do_apply_xop_srl>(queue);
          }{
        case xop_sla:
            return avmcp.output<do_apply_xop_sla>(queue);
          }{
        case xop_sra:
            return avmcp.output<do_apply_xop_sra>(queue);
          }{
        case xop_andb:
            return avmcp.output<do_apply_xop_andb>(queue);
          }{
        case xop_orb:
            return avmcp.output<do_apply_xop_orb>(queue);
          }{
        case xop_xorb:
            return avmcp.output<do_apply_xop_xorb>(queue);
          }{
        case xop_assign:
            return avmcp.output<do_apply_xop_assign>(queue);
          }{
        case xop_fma_3:
            return avmcp.output<do_apply_xop_fma>(queue);
          }{
        case xop_tail:
            return avmcp.output<do_apply_xop_tail>(queue);
          }}
        default:
          ASTERIA_TERMINATE("invalid operator type (xop `$1`)", altr.xop);
        }
      }{
    case index_unpack_struct_array:
        const auto& altr = this->m_stor.as<index_unpack_struct_array>();
        // `paramu.y8s[0]` is `immutable`.
        // `paramu.y32` is `nelems`.
        // `params` is unused.
        AVMC_Appender<void> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.paramu.y8s[0] = altr.immutable;
        avmcp.paramu.y32 = altr.nelems;
        // Push a new node.
        return avmcp.output<do_unpack_struct_array>(queue);
      }{
    case index_unpack_struct_object:
        const auto& altr = this->m_stor.as<index_unpack_struct_object>();
        // `paramu.u8s[0]` is `immutable`.
        // `params` points to the keys.
        AVMC_Appender<Params_names> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.paramu.u8s[0] = altr.immutable;
        avmcp.names = altr.keys;
        // Push a new node.
        return avmcp.output<do_unpack_struct_object>(queue);
      }{
    case index_define_null_variable:
        const auto& altr = this->m_stor.as<index_define_null_variable>();
        // `paramu.u8s[0]` is `immutable`.
        // `params` points to the source location and name.
        AVMC_Appender<Params_sloc_name> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.paramu.u8s[0] = altr.immutable;
        avmcp.sloc = altr.sloc;
        avmcp.name = altr.name;
        // Push a new node.
        return avmcp.output<do_define_null_variable>(queue);
      }{
    case index_single_step_trap:
        const auto& altr = this->m_stor.as<index_single_step_trap>();
        // `paramu.u8s[0]` is unused.
        // `params` points to the source location.
        AVMC_Appender<Params_sloc> avmcp;
        if(ipass == 0) {
          return avmcp.request(queue);
        }
        // Encode arguments.
        avmcp.sloc = altr.sloc;
        // Push a new node.
        return avmcp.output<do_single_step_trap>(queue);
      }}
    default:
      ASTERIA_TERMINATE("invalid AIR node type (index `$1`)", this->index());
    }
  }

rcptr<Abstract_Function> AIR_Node::instantiate_function(const Abstract_Context* parent_opt) const
  {
    return do_instantiate_function(this->m_stor.as<index_instantiate_function>(), parent_opt);
  }

}  // namespace Asteria
