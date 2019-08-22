// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "evaluation_stack.hpp"
#include "instantiated_function.hpp"
#include "exception.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    ///////////////////////////////////////////////////////////////////////////
    // Auxiliary functions
    ///////////////////////////////////////////////////////////////////////////

    AIR_Status do_execute_block(const AVMC_Queue& queue, /*const*/ Executive_Context& ctx)
      {
        if(ROCKET_EXPECT(queue.empty())) {
          // Don't bother creating a context.
          return air_status_next;
        }
        // Execute the queue on a new context.
        Executive_Context ctx_next(rocket::ref(ctx));
        return queue.execute(ctx_next);
      }

    AIR_Status do_evaluate_branch(const AVMC_Queue& queue, bool assign, /*const*/ Executive_Context& ctx)
      {
        if(ROCKET_EXPECT(queue.empty())) {
          // Leave the condition on the top of the stack.
          return air_status_next;
        }
        // Evaluate the branch.
        auto status = queue.execute(ctx);
        ROCKET_ASSERT(status == air_status_next);
        // Pop the result, then overwrite the top with it.
        ctx.stack().pop_next(assign);
        return status;
      }

    AIR_Status do_execute_catch(const AVMC_Queue& queue, const phsh_string& name_except, const Exception& except, const Executive_Context& ctx)
      {
        if(ROCKET_EXPECT(queue.empty())) {
          return air_status_next;
        }
        // Execute the queue on a new context.
        Executive_Context ctx_next(rocket::ref(ctx));
        // Set the exception reference.
        {
          Reference_Root::S_temporary xref = { except.get_value() };
          ctx_next.open_named_reference(name_except) = rocket::move(xref);
        }
        // Set backtrace frames.
        {
          G_array backtrace;
          G_object r;
          for(size_t i = 0; i != except.count_frames(); ++i) {
            const auto& frame = except.get_frame(i);
            // Translate each frame into a human-readable format.
            r.clear();
            r.try_emplace(rocket::sref("frame"), G_string(rocket::sref(frame.what_type())));
            r.try_emplace(rocket::sref("file"), G_string(frame.file()));
            r.try_emplace(rocket::sref("line"), G_integer(frame.line()));
            r.try_emplace(rocket::sref("value"), frame.value());
            // Append this frame.
            backtrace.emplace_back(rocket::move(r));
          }
          ASTERIA_DEBUG_LOG("Exception backtrace:\n", Value(backtrace));
          Reference_Root::S_constant xref = { rocket::move(backtrace) };
          ctx_next.open_named_reference(rocket::sref("__backtrace")) = rocket::move(xref);
        }
        return queue.execute(ctx_next);
      }

    template<typename TargetT> inline const TargetT& pcast(const void* p) noexcept
      {
        return static_cast<const TargetT*>(p)[0];
      }

        namespace Details {

        template<typename XnodeT, typename = void> struct AVMC_Appender
          {
            // Because the wrapper function is passed as a template argument, we need 'real' function pointers.
            // Those converted from non-capturing lambdas are not an option.
            static Variable_Callback& enumerate_wrapper(Variable_Callback& callback, uint32_t /*k*/, const void* p)
              {
                return static_cast<const typename std::remove_reference<XnodeT>::type*>(p)->enumerate_variables(callback);
              }
            template<AVMC_Queue::Executor execuT> static AVMC_Queue& append(AVMC_Queue& queue, uint32_t k, XnodeT&& xnode)
              {
                return queue.append<execuT, enumerate_wrapper>(k, rocket::forward<XnodeT>(xnode));
              }
          };
        template<typename XnodeT> struct AVMC_Appender<XnodeT, ASTERIA_VOID_T(typename rocket::remove_cvref<XnodeT>::type::contains_no_variable)>
          {
            template<AVMC_Queue::Executor execuT> static AVMC_Queue& append(AVMC_Queue& queue, uint32_t k, XnodeT&& xnode)
              {
                return queue.append<execuT>(k, rocket::forward<XnodeT>(xnode));
              }
          };

        }  // namespace Details

    template<AVMC_Queue::Executor execuT> AVMC_Queue& do_append(AVMC_Queue& queue, uint32_t k)
      {
        return queue.append<execuT>(k);
      }
    template<AVMC_Queue::Executor execuT, typename XnodeT> AVMC_Queue& do_append(AVMC_Queue& queue, uint32_t k, XnodeT&& xnode)
      {
        return Details::AVMC_Appender<XnodeT>::template append<execuT>(queue, k, rocket::forward<XnodeT>(xnode));
      }

    ///////////////////////////////////////////////////////////////////////////
    // Parameter structs and variable enumeration callbacks
    ///////////////////////////////////////////////////////////////////////////

    union SK_xrel
      {
        struct {
          uint32_t assign : 1;
          uint32_t expect : 2;
          uint32_t negative : 1;
        };
        uint32_t paramk;

        constexpr SK_xrel() noexcept
          : paramk(0)
          {
          }
        explicit constexpr SK_xrel(uint32_t xparamk = 0) noexcept
          : paramk(xparamk)
          {
          }
        constexpr SK_xrel(bool xassign, Compare xexpect, bool xnegative) noexcept
          : assign(xassign), expect(xexpect & 0b11), negative(xnegative)
          {
          }

        constexpr operator uint32_t () const noexcept
          {
            return this->paramk;
          }
      };

    template<size_t nqsT> struct SP_queues_fixed
      {
        AVMC_Queue queues[nqsT];

        Variable_Callback& enumerate_variables(Variable_Callback& callback) const
          {
            rocket::for_each(this->queues, [&](const AVMC_Queue& queue) { queue.enumerate_variables(callback);  });
            return callback;
          }
      };

    struct SP_switch
      {
        cow_vector<AVMC_Queue> queues_labels;
        cow_vector<AVMC_Queue> queues_bodies;
        cow_vector<cow_vector<phsh_string>> names_added;

        Variable_Callback& enumerate_variables(Variable_Callback& callback) const
          {
            rocket::for_each(this->queues_labels, [&](const AVMC_Queue& queue) { queue.enumerate_variables(callback);  });
            rocket::for_each(this->queues_bodies, [&](const AVMC_Queue& queue) { queue.enumerate_variables(callback);  });
            return callback;
          }
      };

    struct SP_for_each
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

    struct SP_try
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

    struct SP_name
      {
        phsh_string name;

        using contains_no_variable = std::true_type;
      };

    struct SP_names
      {
        cow_vector<phsh_string> names;

        using contains_no_variable = std::true_type;
      };

    struct SP_sloc
      {
        Source_Location sloc;

        using contains_no_variable = std::true_type;
      };

    struct SP_sloc_msg
      {
        Source_Location sloc;
        cow_string msg;

        using contains_no_variable = std::true_type;
      };

    struct SP_func
      {
        Compiler_Options options;
        Source_Location sloc;
        cow_string name;
        cow_vector<phsh_string> params;
        cow_vector<Statement> body;

        using contains_no_variable = std::true_type;
      };

    struct SP_call
      {
        Source_Location sloc;
        cow_vector<bool> args_by_refs;

        using contains_no_variable = std::true_type;
      };

    ///////////////////////////////////////////////////////////////////////////
    // Executor functions
    ///////////////////////////////////////////////////////////////////////////

    AIR_Status do_clear_stack(Executive_Context& ctx, uint32_t /*k*/, const void* /*p*/)
      {
        // Clear the stack.
        ctx.stack().clear();
        return air_status_next;
      }

    AIR_Status do_execute_block(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& queue_body = pcast<SP_queues_fixed<1>>(p).queues[0];
        // Execute the body on a new context.
        return do_execute_block(queue_body, ctx);
      }

    AIR_Status do_declare_variable(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& immutable = k != 0;
        const auto& name = pcast<SP_name>(p).name;
        // Allocate a variable and initialize it to `null`.
        auto var = ctx.global().create_variable();
        var->reset(G_null(), immutable);
        // Inject the variable into the current context.
        Reference_Root::S_variable xref = { rocket::move(var) };
        ctx.open_named_reference(name) = xref;
        // Push a copy of the reference onto the stack.
        ctx.stack().push(rocket::move(xref));
        return air_status_next;
      }

    AIR_Status do_initialize_variable(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& immutable = k != 0;
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this function.
        auto value = ctx.stack().top().read();
        ctx.stack().pop();
        // Get the variable back.
        auto var = ctx.stack().top().get_variable_opt();
        ROCKET_ASSERT(var);
        // Initialize it.
        var->reset(rocket::move(value), immutable);
        return air_status_next;
      }

    AIR_Status do_if_statement(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& negative = k != 0;
        const auto& queue_true = pcast<SP_queues_fixed<2>>(p).queues[0];
        const auto& queue_false = pcast<SP_queues_fixed<2>>(p).queues[1];
        // Check the value of the condition.
        if(ctx.stack().top().read().test() != negative) {
          // Execute the true branch.
          return do_execute_block(queue_true, ctx);
        }
        // Execute the false branch.
        return do_execute_block(queue_false, ctx);
      }

    AIR_Status do_switch_statement(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& queues_labels = pcast<SP_switch>(p).queues_labels;
        const auto& queues_bodies = pcast<SP_switch>(p).queues_bodies;
        const auto& names_added = pcast<SP_switch>(p).names_added;
        // Read the value of the condition.
        auto value = ctx.stack().top().read();
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
              ASTERIA_THROW_RUNTIME_ERROR("Multiple `default` clauses have been found in this `switch` statement.");
            }
            target = i;
            continue;
          }
          // This is a `case` label.
          // Evaluate the operand and check whether it equals `value`.
          auto status = queues_labels[i].execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
          if(ctx.stack().top().read().compare(value) == compare_equal) {
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
        Executive_Context ctx_body(rocket::ref(ctx));
        // Fly over all clauses that precede `target`.
        for(size_t i = 0; i != target; ++i) {
          rocket::for_each(names_added[i], [&](const phsh_string& name) { ctx_body.open_named_reference(name) = Reference_Root::S_null();  });
        }
        // Execute all clauses from `target`.
        for(size_t i = target; i != nclauses; ++i) {
          auto status = queues_bodies[i].execute(ctx_body);
          if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_switch })) {
            break;
          }
          if(status != air_status_next) {
            return status;
          }
        }
        return air_status_next;
      }

    AIR_Status do_do_while_statement(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& queue_body = pcast<SP_queues_fixed<2>>(p).queues[0];
        const auto& negative = k != 0;
        const auto& queue_cond = pcast<SP_queues_fixed<2>>(p).queues[1];
        // This is the same as the `do...while` statement in C.
        for(;;) {
          // Execute the body.
          auto status = do_execute_block(queue_body, ctx);
          if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while })) {
            break;
          }
          if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_while })) {
            return status;
          }
          // Check the condition.
          ctx.stack().clear();
          status = queue_cond.execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
          if(ctx.stack().top().read().test() == negative) {
            break;
          }
        }
        return air_status_next;
      }

    AIR_Status do_while_statement(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& negative = k != 0;
        const auto& queue_cond = pcast<SP_queues_fixed<2>>(p).queues[0];
        const auto& queue_body = pcast<SP_queues_fixed<2>>(p).queues[1];
        // This is the same as the `while` statement in C.
        for(;;) {
          // Check the condition.
          ctx.stack().clear();
          auto status = queue_cond.execute(ctx);
          ROCKET_ASSERT(status == air_status_next);
          if(ctx.stack().top().read().test() == negative) {
            break;
          }
          // Execute the body.
          status = do_execute_block(queue_body, ctx);
          if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while })) {
            break;
          }
          if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_while })) {
            return status;
          }
        }
        return air_status_next;
      }

    AIR_Status do_for_each_statement(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& name_key = pcast<SP_for_each>(p).name_key;
        const auto& name_mapped = pcast<SP_for_each>(p).name_mapped;
        const auto& queue_init = pcast<SP_for_each>(p).queue_init;
        const auto& queue_body = pcast<SP_for_each>(p).queue_body;
        // We have to create an outer context due to the fact that the key and mapped references outlast every iteration.
        Executive_Context ctx_for(rocket::ref(ctx));
        // Create a variable for the key.
        auto key = ctx_for.global().create_variable();
        key->reset(G_null(), true);
        {
          Reference_Root::S_variable xref = { key };
          ctx_for.open_named_reference(name_key) = rocket::move(xref);
        }
        // Create the mapped reference.
        auto& mapped = ctx_for.open_named_reference(name_mapped);
        mapped = Reference_Root::S_null();
        // Evaluate the range initializer.
        ctx_for.stack().clear();
        auto status = queue_init.execute(ctx_for);
        ROCKET_ASSERT(status == air_status_next);
        // Set the range up.
        mapped = rocket::move(ctx_for.stack().open_top());
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
              mapped.zoom_in(rocket::move(xmod));
            }
            // Execute the loop body.
            status = do_execute_block(queue_body, ctx_for);
            if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
              break;
            }
            if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for })) {
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
              mapped.zoom_in(rocket::move(xmod));
            }
            // Execute the loop body.
            status = do_execute_block(queue_body, ctx_for);
            if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
              break;
            }
            if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for })) {
              return status;
            }
            // Restore the mapped reference.
            mapped.zoom_out();
          }
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("The `for each` statement does not accept a range of type `", range.what_gtype(), "`.");
        }
        return air_status_next;
      }

    AIR_Status do_for_statement(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& infinite = k != 0;
        const auto& queue_init = pcast<SP_queues_fixed<4>>(p).queues[0];
        const auto& queue_cond = pcast<SP_queues_fixed<4>>(p).queues[1];
        const auto& queue_step = pcast<SP_queues_fixed<4>>(p).queues[2];
        const auto& queue_body = pcast<SP_queues_fixed<4>>(p).queues[3];
        // This is the same as the `for` statement in C.
        // We have to create an outer context due to the fact that names declared in the first segment outlast every iteration.
        Executive_Context ctx_for(rocket::ref(ctx));
        // Execute the loop initializer, which shall only be a definition or an expression statement.
        auto status = queue_init.execute(ctx_for);
        ROCKET_ASSERT(status == air_status_next);
        for(;;) {
          if(!infinite) {
            // Check the condition.
            ctx_for.stack().clear();
            status = queue_cond.execute(ctx_for);
            ROCKET_ASSERT(status == air_status_next);
            if(ctx_for.stack().top().read().test() == false) {
              break;
            }
          }
          // Execute the body.
          status = do_execute_block(queue_body, ctx_for);
          if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
            break;
          }
          if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for })) {
            return status;
          }
          // Execute the increment.
          ctx_for.stack().clear();
          status = queue_step.execute(ctx_for);
          ROCKET_ASSERT(status == air_status_next);
        }
        return air_status_next;
      }

    AIR_Status do_try_statement(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& queue_try = pcast<SP_try>(p).queue_try;
        const auto& sloc = pcast<SP_try>(p).sloc;
        const auto& name_except = pcast<SP_try>(p).name_except;
        const auto& queue_catch = pcast<SP_try>(p).queue_catch;
        // This is almost identical to JavaScript.
        try {
          // Execute the `try` clause. If no exception is thrown, this will have little overhead.
          return do_execute_block(queue_try, ctx);
        }
        catch(Exception& except) {
          // Reuse the exception object. Don't bother allocating a new one.
          except.push_frame_catch(sloc);
          ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", except);
          // This branch must be executed inside this `catch` block.
          // User-provided bindings may obtain the current exception using `std::current_exception`.
          return do_execute_catch(queue_catch, name_except, except, ctx);
        }
        catch(const std::exception& stdex) {
          // Translate the exception.
          Exception except(stdex);
          except.push_frame_catch(sloc);
          ASTERIA_DEBUG_LOG("Translated `std::exception`: ", except);
          // This branch must be executed inside this `catch` block.
          // User-provided bindings may obtain the current exception using `std::current_exception`.
          return do_execute_catch(queue_catch, name_except, except, ctx);
        }
      }

    AIR_Status do_throw_statement(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& sloc = pcast<SP_sloc>(p).sloc;
        // Read the value to throw.
        // Note that the operand must not have been empty for this code.
        auto value = ctx.stack().top().read();
        try {
          // Unpack the nested exception, if any.
          auto eptr = std::current_exception();
          if(eptr) {
            std::rethrow_exception(eptr);
          }
          // If no nested exception exists, construct a fresh one.
          Exception except(sloc, rocket::move(value));
          throw except;
        }
        catch(Exception& except) {
          // Modify it in place. Don't bother allocating a new one.
          except.push_frame_throw(sloc, rocket::move(value));
          throw;
        }
        catch(const std::exception& stdex) {
          // Translate the exception.
          Exception except(stdex);
          except.push_frame_throw(sloc, rocket::move(value));
          throw except;
        }
      }

    AIR_Status do_assert_statement(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& sloc = pcast<SP_sloc_msg>(p).sloc;
        const auto& negative = k != 0;
        const auto& msg = pcast<SP_sloc_msg>(p).msg;
        // Check the value of the condition.
        if(ctx.stack().top().read().test() != negative) {
          // When the assertion succeeds, there is nothing to do.
          return air_status_next;
        }
        // Throw a `Runtime_Error`.
        cow_osstream fmtss;
        fmtss.imbue(std::locale::classic());
        fmtss << "Assertion failed at \'" << sloc << "\': " << msg;
        throw_runtime_error(__func__, fmtss.extract_string());
      }

    AIR_Status do_simple_status(Executive_Context& /*ctx*/, uint32_t k, const void* /*p*/)
      {
        const auto& status = static_cast<AIR_Status>(k);
        return status;
      }

    AIR_Status do_return_by_value(Executive_Context& ctx, uint32_t /*k*/, const void* /*p*/)
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

    AIR_Status do_push_literal(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& val = pcast<Value>(p);
        // Push a constant.
        Reference_Root::S_constant xref = { val };
        ctx.stack().push(rocket::move(xref));
        return air_status_next;
      }

    AIR_Status do_push_global_reference(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& name = pcast<SP_name>(p).name;
        // Look for the name in the global context.
        auto qref = ctx.global().get_named_reference_opt(name);
        if(!qref) {
          ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared yet.");
        }
        // Push a copy of it.
        ctx.stack().push(*qref);
        return air_status_next;
      }

    AIR_Status do_push_local_reference(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& depth = k;
        const auto& name = pcast<SP_name>(p).name;
        // Get the context.
        const Executive_Context* qctx = std::addressof(ctx);
        rocket::ranged_for(uint32_t(0), depth, [&](size_t) { qctx = qctx->get_parent_opt();  });
        ROCKET_ASSERT(qctx);
        // Look for the name in the context.
        auto qref = qctx->get_named_reference_opt(name);
        if(!qref) {
          ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared yet.");
        }
        // Push a copy of it.
        ctx.stack().push(*qref);
        return air_status_next;
      }

    AIR_Status do_push_bound_reference(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& ref = pcast<Reference>(p);
        // Push a copy of the bound reference.
        ctx.stack().push(ref);
        return air_status_next;
      }

    AIR_Status do_define_function(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& options = pcast<SP_func>(p).options;
        const auto& sloc = pcast<SP_func>(p).sloc;
        const auto& name = pcast<SP_func>(p).name;
        const auto& params = pcast<SP_func>(p).params;
        const auto& body = pcast<SP_func>(p).body;
        // Instantiate the function.
        auto qtarget = rocket::make_refcnt<Instantiated_Function>(options, sloc, name, std::addressof(ctx), params, body);
        ASTERIA_DEBUG_LOG("New function: ", *qtarget);
        // Push the function as a temporary.
        Reference_Root::S_temporary xref = { G_function(rocket::move(qtarget)) };
        ctx.stack().push(rocket::move(xref));
        return air_status_next;
      }

    AIR_Status do_branch_expression(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& assign = k != 0;
        const auto& queue_true = pcast<SP_queues_fixed<2>>(p).queues[0];
        const auto& queue_false = pcast<SP_queues_fixed<2>>(p).queues[1];
        // Check the value of the condition.
        if(ctx.stack().top().read().test() != false) {
          // Evaluate the true branch.
          return do_evaluate_branch(queue_true, assign, ctx);
        }
        // Evaluate the false branch.
        return do_evaluate_branch(queue_false, assign, ctx);
      }

    AIR_Status do_coalescence(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& assign = k != 0;
        const auto& queue_null = pcast<SP_queues_fixed<1>>(p).queues[0];
        // Check the value of the condition.
        if(ctx.stack().top().read().is_null() != false) {
          // Evaluate the alternative.
          return do_evaluate_branch(queue_null, assign, ctx);
        }
        // Leave the condition on the stack.
        return air_status_next;
      }

    AIR_Status do_function_call(Executive_Context& ctx, uint32_t k, const void* p)
      {
        const auto& sloc = pcast<SP_call>(p).sloc;
        const auto& args_by_refs = pcast<SP_call>(p).args_by_refs;
        const auto& tco_aware = static_cast<TCO_Aware>(k);
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
          // Move and pop this argument.
          args.mut(i) = rocket::move(arg);
          ctx.stack().pop();
        }
        // Get the target reference.
        auto& self = ctx.stack().open_top();
        // Copy the target value, which shall be of type `function`.
        const auto val = self.read();
        if(!val.is_function()) {
          ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to invoke `", val, "` which is not a function.");
        }
        const auto& target = val.as_function();
        // Initialize the `this` reference.
        self.zoom_out();
        // Call the function now.
        const auto& func = ctx.zvarg()->get_function_signature();
        if(tco_aware != tco_aware_none) {
          // Pack arguments for this proper tail call.
          args.emplace_back(rocket::move(self));
          // Create a TCO wrapper.
          auto tca = rocket::make_refcnt<Tail_Call_Arguments>(sloc, func, tco_aware, target, rocket::move(args));
          // Return it. Note that we force `air_status_return` here for value-discarding TCO.
          Reference_Root::S_tail_call xref = { rocket::move(tca) };
          self = rocket::move(xref);
          return air_status_return;
        }
        // Perform a non-proper call.
        try {
          ASTERIA_DEBUG_LOG("Initiating function call at \'", sloc, "\' inside `", func, "`: target = ", target);
          target->invoke(self, ctx.global(), rocket::move(args));
          self.finish_call(ctx.global());
          ASTERIA_DEBUG_LOG("Returned from function call at \'", sloc, "\' inside `", func, "`: target = ", target);
          return air_status_next;
        }
        catch(Exception& except) {
          ASTERIA_DEBUG_LOG("Caught `Asteria::Exception` thrown inside function call at \'", sloc, "\' inside `", func, "`: ", except.get_value());
          // Append the current frame and rethrow the exception.
          except.push_frame_func(sloc, func);
          throw;
        }
        catch(const std::exception& stdex) {
          ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", sloc, "\' inside `", func, "`: ", stdex.what());
          // Translate the exception, append the current frame, and throw the new exception.
          Exception except(stdex);
          except.push_frame_func(sloc, func);
          throw except;
        }
      }

    AIR_Status do_member_access(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& name = pcast<SP_name>(p).name;
        // Append a modifier to the reference at the top.
        Reference_Modifier::S_object_key xmod = { name };
        ctx.stack().open_top().zoom_in(rocket::move(xmod));
        return air_status_next;
      }

    AIR_Status do_push_unnamed_array(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& nelems = k;
        // Pop elements from the stack and store them in an array backwards.
        G_array array;
        array.resize(nelems);
        for(auto it = array.mut_rbegin(); it != array.rend(); ++it) {
          *it = ctx.stack().top().read();
          ctx.stack().pop();
        }
        // Push the array as a temporary.
        Reference_Root::S_temporary xref = { rocket::move(array) };
        ctx.stack().push(rocket::move(xref));
        return air_status_next;
      }

    AIR_Status do_push_unnamed_object(Executive_Context& ctx, uint32_t /*k*/, const void* p)
      {
        const auto& keys = pcast<SP_names>(p).names;
        // Pop elements from the stack and store them in an object backwards.
        G_object object;
        object.reserve(keys.size());
        for(auto it = keys.rbegin(); it != keys.rend(); ++it) {
          object.insert_or_assign(*it, ctx.stack().top().read());
          ctx.stack().pop();
        }
        // Push the object as a temporary.
        Reference_Root::S_temporary xref = { rocket::move(object) };
        ctx.stack().push(rocket::move(xref));
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
          ASTERIA_THROW_RUNTIME_ERROR("The opposite of `", rhs, "` cannot be represented as an `integer`.");
        }
        return -rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sqrt(const G_integer& rhs)
      {
        return std::sqrt(G_real(rhs));
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
          ASTERIA_THROW_RUNTIME_ERROR("The absolute value of `", rhs, "` cannot be represented as an `integer`.");
        }
        return std::abs(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_signb(const G_integer& rhs)
      {
        return rhs >> 63;
      }

    [[noreturn]] void do_throw_integral_overflow(const char* op, const G_integer& lhs, const G_integer& rhs)
      {
        ASTERIA_THROW_RUNTIME_ERROR("Integral ", op, " of `", lhs, "` and `", rhs, "` would result in overflow.");
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_add(const G_integer& lhs, const G_integer& rhs)
      {
        if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
          do_throw_integral_overflow("addition", lhs, rhs);
        }
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sub(const G_integer& lhs, const G_integer& rhs)
      {
        if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
          do_throw_integral_overflow("subtraction", lhs, rhs);
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
          do_throw_integral_overflow("multiplication", lhs, rhs);
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
          do_throw_integral_overflow("multiplication", lhs, rhs);
        }
        return alhs * srhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_div(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          do_throw_integral_overflow("division", lhs, rhs);
        }
        return lhs / rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_mod(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          do_throw_integral_overflow("division", lhs, rhs);
        }
        return lhs % rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sll(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        return G_integer(static_cast<uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_srl(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        return G_integer(static_cast<uint64_t>(lhs) >> rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sla(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(lhs == 0) {
          return 0;
        }
        if(rhs >= 64) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        auto bc = static_cast<int>(63 - rhs);
        auto mask_out = static_cast<uint64_t>(lhs) >> bc << bc;
        auto mask_sbt = static_cast<uint64_t>(lhs >> 63) << bc;
        if(mask_out != mask_sbt) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        return G_integer(static_cast<uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sra(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
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
        return std::sqrt(rhs);
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isinf(const G_real& rhs)
      {
        return std::isinf(rhs);
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isnan(const G_real& rhs)
      {
        return std::isnan(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_abs(const G_real& rhs)
      {
        return std::fabs(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_signb(const G_real& rhs)
      {
        return std::signbit(rhs) ? -1 : 0;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_round(const G_real& rhs)
      {
        return std::round(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_floor(const G_real& rhs)
      {
        return std::floor(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_ceil(const G_real& rhs)
      {
        return std::ceil(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_trunc(const G_real& rhs)
      {
        return std::trunc(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_icast(const G_real& value)
      {
        if(!std::islessequal(INT64_MIN, value) || !std::islessequal(value, INT64_MAX)) {
          ASTERIA_THROW_RUNTIME_ERROR("The `real` value `", value, "` cannot be represented as an `integer`.");
        }
        return G_integer(value);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_iround(const G_real& rhs)
      {
        return do_icast(std::round(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_ifloor(const G_real& rhs)
      {
        return do_icast(std::floor(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_iceil(const G_real& rhs)
      {
        return do_icast(std::ceil(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_itrunc(const G_real& rhs)
      {
        return do_icast(std::trunc(rhs));
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
        return std::fmod(lhs, rhs);
      }

    ROCKET_PURE_FUNCTION G_string do_operator_add(const G_string& lhs, const G_string& rhs)
      {
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_mul(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String duplication count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        auto nchars = lhs.size();
        if((nchars == 0) || (rhs == 0)) {
          return res;
        }
        if(nchars > res.max_size() / static_cast<uint64_t>(rhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
        }
        auto times = static_cast<size_t>(rhs);
        if(nchars == 1) {
          // Fast fill.
          res.assign(times, lhs.front());
          return res;
        }
        // Reserve space for the result string.
        auto ptr = res.assign(nchars * times, '*').mut_data();
        // Copy the source string once.
        std::memcpy(ptr, lhs.data(), nchars);
        // Append the result string to itself, doubling its length, until more than half of the result string has been populated.
        while(nchars <= res.size() / 2) {
          std::memcpy(ptr + nchars, ptr, nchars);
          nchars *= 2;
        }
        // Copy remaining characters, if any.
        if(nchars < res.size()) {
          std::memcpy(ptr + nchars, ptr, res.size() - nchars);
        }
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_mul(const G_integer& lhs, const G_string& rhs)
      {
        return do_operator_mul(rhs, lhs);
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sll(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        // Reserve space for the result string.
        auto ptr = rocket::unfancy(res.insert(res.begin(), lhs.size(), ' '));
        if(static_cast<uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<size_t>(rhs);
        // Copy the substring in the right.
        std::memcpy(ptr, lhs.data() + count, lhs.size() - count);
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_srl(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        // Reserve space for the result string.
        auto ptr = rocket::unfancy(res.insert(res.begin(), lhs.size(), ' '));
        if(static_cast<uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<size_t>(rhs);
        // Copy the substring in the left.
        std::memcpy(ptr + count, lhs.data(), lhs.size() - count);
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sla(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        if(static_cast<uint64_t>(rhs) >= res.max_size() - lhs.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("Shifting `", lhs, "` to the left by `", rhs, "` bytes would result in an overlong string that cannot be allocated.");
        }
        auto count = static_cast<size_t>(rhs);
        // Append spaces in the right and return the result.
        res.assign(G_string::shallow_type(lhs));
        res.append(count, ' ');
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sra(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        if(static_cast<uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<size_t>(rhs);
        // Return the substring in the left.
        res.append(lhs.data(), lhs.size() - count);
        return res;
      }

    AIR_Status do_apply_xop_inc_post(Executive_Context& ctx, uint32_t /*k*/, const void* /*p*/)
      {
        // This operator is unary.
        auto& lhs = ctx.stack().top().open();
        // Increment the operand and return the old value. `assign` is ignored.
        if(lhs.is_integer()) {
          auto& reg = lhs.open_integer();
          ctx.stack().set_temporary(false, rocket::move(lhs));
          reg = do_operator_add(reg, G_integer(1));
        }
        else if(lhs.is_real()) {
          auto& reg = lhs.open_real();
          ctx.stack().set_temporary(false, rocket::move(lhs));
          reg = do_operator_add(reg, G_real(1));
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Postfix increment is not defined for `", lhs, "`.");
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_dec_post(Executive_Context& ctx, uint32_t /*k*/, const void* /*p*/)
      {
        // This operator is unary.
        auto& lhs = ctx.stack().top().open();
        // Decrement the operand and return the old value. `assign` is ignored.
        if(lhs.is_integer()) {
          auto& reg = lhs.open_integer();
          ctx.stack().set_temporary(false, rocket::move(lhs));
          reg = do_operator_sub(reg, G_integer(1));
        }
        else if(lhs.is_real()) {
          auto& reg = lhs.open_real();
          ctx.stack().set_temporary(false, rocket::move(lhs));
          reg = do_operator_sub(reg, G_real(1));
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Postfix decrement is not defined for `", lhs, "`.");
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_subscr(Executive_Context& ctx, uint32_t /*k*/, const void* /*p*/)
      {
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        auto& lref = ctx.stack().open_top();
        // Append a reference modifier. `assign` is ignored.
        if(rhs.is_integer()) {
          auto& reg = rhs.open_integer();
          Reference_Modifier::S_array_index xmod = { rocket::move(reg) };
          lref.zoom_in(rocket::move(xmod));
        }
        else if(rhs.is_string()) {
          auto& reg = rhs.open_string();
          Reference_Modifier::S_object_key xmod = { rocket::move(reg) };
          lref.zoom_in(rocket::move(xmod));
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("The value `", rhs, "` cannot be used as a subscript.");
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_pos(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
        // Copy the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_neg(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix negation is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_notb(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix bitwise NOT is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_notl(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        const auto& rhs = ctx.stack().top().read();
        // Perform logical NOT operation on the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        ctx.stack().set_temporary(assign, do_operator_not(rhs.test()));
        return air_status_next;
      }

    AIR_Status do_apply_xop_inc_pre(Executive_Context& ctx, uint32_t /*k*/, const void* /*p*/)
      {
        // This operator is unary.
        auto& rhs = ctx.stack().top().open();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix increment is not defined for `", rhs, "`.");
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_dec_pre(Executive_Context& ctx, uint32_t /*k*/, const void* /*p*/)
      {
        // This operator is unary.
        auto& rhs = ctx.stack().top().open();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix increment is not defined for `", rhs, "`.");
        }
        return air_status_next;
      }

    AIR_Status do_apply_xop_unset(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().unset();
        // Unset the reference and return the old value.
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_lengthof(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        const auto& rhs = ctx.stack().top().read();
        // Return the number of elements in the operand.
        size_t nelems;
        if(rhs.is_null()) {
          nelems = 0;
        }
        else if(rhs.is_string()) {
          nelems = rhs.as_string().size();
        }
        else if(rhs.is_array()) {
          nelems = rhs.as_array().size();
        }
        else if(rhs.is_object()) {
          nelems = rhs.as_object().size();
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `lengthof` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, G_integer(nelems));
        return air_status_next;
      }

    AIR_Status do_apply_xop_typeof(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        const auto& rhs = ctx.stack().top().read();
        // Return the type name of the operand.
        // N.B. This is one of the few operators that work on all types.
        ctx.stack().set_temporary(assign, G_string(rocket::sref(rhs.what_gtype())));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sqrt(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__sqrt` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_isnan(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__isnan` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_isinf(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__isinf` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_abs(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__abs` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_signb(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
        // Get the sign bit of the operand as a temporary value, then return it.
        if(rhs.is_integer()) {
          auto& reg = rhs.open_integer();
          reg = do_operator_signb(reg);
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_signb(rhs.as_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__signb` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_round(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__round` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_floor(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__floor` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_ceil(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__ceil` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_trunc(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__trunc` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_iround(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__iround` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_ifloor(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__ifloor` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_iceil(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__iceil` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_itrunc(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is unary.
        auto rhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__itrunc` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_cmp_xeq(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = SK_xrel(k).assign != 0;
        const auto& expect = static_cast<Compare>(SK_xrel(k).expect);
        const auto& negative = SK_xrel(k).negative != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        rhs = G_boolean((comp == expect) != negative);
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_cmp_xrel(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = SK_xrel(k).assign != 0;
        const auto& expect = static_cast<Compare>(SK_xrel(k).expect);
        const auto& negative = SK_xrel(k).negative != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        if(comp == compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", rhs, "` are unordered.");
        }
        rhs = G_boolean((comp == expect) != negative);
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_cmp_3way(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        switch(comp) {
        case compare_greater:
          rhs = G_integer(+1);
          break;
        case compare_less:
          rhs = G_integer(-1);
          break;
        case compare_equal:
          rhs = G_integer(0);
          break;
        case compare_unordered:
          rhs = G_string(rocket::sref("<unordered>"));
          break;
        default:
          ROCKET_ASSERT(false);
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_add(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix addition is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sub(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix subtraction is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_mul(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix multiplication is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_div(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix division is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_mod(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix modulo operation is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sll(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix logical shift to the left is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_srl(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix logical shift to the right is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sla(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix arithmetic shift to the left is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_sra(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix arithmetic shift to the right is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_andb(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix bitwise AND is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_orb(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix bitwise OR is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_xorb(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is binary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
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
          ASTERIA_THROW_RUNTIME_ERROR("Infix bitwise XOR is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_assign(Executive_Context& ctx, uint32_t /*k*/, const void* /*p*/)
      {
        // Pop the RHS operand.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        // Copy the value to the LHS operand which is write-only. `assign` is ignored.
        ctx.stack().set_temporary(true, rocket::move(rhs));
        return air_status_next;
      }

    AIR_Status do_apply_xop_fma(Executive_Context& ctx, uint32_t k, const void* /*p*/)
      {
        const auto& assign = k != 0;
        // This operator is ternary.
        auto rhs = ctx.stack().top().read();
        ctx.stack().pop();
        auto mid = ctx.stack().top().read();
        ctx.stack().pop();
        const auto& lhs = ctx.stack().top().read();
        if(lhs.is_convertible_to_real() && mid.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Calculate the fused multiply-add result of the operands.
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = std::fma(lhs.convert_to_real(), mid.convert_to_real(), rhs.convert_to_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Fused multiply-add is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary(assign, rocket::move(rhs));
        return air_status_next;
      }

    }

AVMC_Queue& AIR_Node::solidify(AVMC_Queue& queue, uint8_t ipass) const
  {
    switch(this->index()) {
    case index_clear_stack:
      {
        // There is no argument.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_clear_stack>(queue, 0);
      }
    case index_execute_block:
      {
        const auto& altr = this->m_stor.as<index_execute_block>();
        // `k` is unused. `p` points to the body.
        SP_queues_fixed<1> sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the body.
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 0);  });  // 1st pass
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 1);  });  // 2nd pass
        // Push a new node.
        return do_append<do_execute_block>(queue, 0, rocket::move(sp));
      }
    case index_declare_variable:
      {
        const auto& altr = this->m_stor.as<index_declare_variable>();
        // `k` is `immutable`. `p` points to the name.
        SP_name sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the name.
        sp.name = altr.name;
        // Push a new node.
        return do_append<do_declare_variable>(queue, altr.immutable, rocket::move(sp));
      }
    case index_initialize_variable:
      {
        const auto& altr = this->m_stor.as<index_initialize_variable>();
        // `k` is `immutable`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_initialize_variable>(queue, altr.immutable);
      }
    case index_if_statement:
      {
        const auto& altr = this->m_stor.as<index_if_statement>();
        // `k` is `negative`. `p` points to the two branches.
        SP_queues_fixed<2> sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the true branch.
        rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 0);  });  // 1st pass
        rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 1);  });  // 2nd pass
        // Solidify the false branch.
        rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 0);  });  // 1st pass
        rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 1);  });  // 2nd pass
        // Push a new node.
        return do_append<do_if_statement>(queue, altr.negative, rocket::move(sp));
      }
    case index_switch_statement:
      {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        // `k` is unused. `p` points to all clauses.
        SP_switch sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify all labels.
        for(const auto& code_label : altr.code_labels) {
          auto& queue_label = sp.queues_labels.emplace_back();
          rocket::for_each(code_label, [&](const AIR_Node& node) { node.solidify(queue_label, 0);  });  // 1st pass
          rocket::for_each(code_label, [&](const AIR_Node& node) { node.solidify(queue_label, 1);  });  // 2nd pass
        }
        // Solidify all clause bodies.
        for(const auto& code_body : altr.code_bodies) {
          auto& queue_body = sp.queues_bodies.emplace_back();
          rocket::for_each(code_body, [&](const AIR_Node& node) { node.solidify(queue_body, 0);  });  // 1st pass
          rocket::for_each(code_body, [&](const AIR_Node& node) { node.solidify(queue_body, 1);  });  // 2nd pass
        }
        // Solidify name lists.
        sp.names_added = altr.names_added;
        // Push a new node.
        return do_append<do_switch_statement>(queue, 0, rocket::move(sp));
      }
    case index_do_while_statement:
      {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        // `k` is `negative`. `p` points to the body and the condition.
        SP_queues_fixed<2> sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the body.
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 0);  });  // 1st pass
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 1);  });  // 2nd pass
        // Solidify the condition.
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 0);  });  // 1st pass
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 1);  });  // 2nd pass
        // Push a new node.
        return do_append<do_do_while_statement>(queue, altr.negative, rocket::move(sp));
      }
    case index_while_statement:
      {
        const auto& altr = this->m_stor.as<index_while_statement>();
        // `k` is `negative`. `p` points to the condition and the body.
        SP_queues_fixed<2> sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the condition.
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 0);  });  // 1st pass
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 1);  });  // 2nd pass
        // Solidify the body.
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 0);  });  // 1st pass
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 1);  });  // 2nd pass
        // Push a new node.
        return do_append<do_while_statement>(queue, altr.negative, rocket::move(sp));
      }
    case index_for_each_statement:
      {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        // `k` is unused. `p` points to the range initializer and the body.
        SP_for_each sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the name of references.
        sp.name_key = altr.name_key;
        sp.name_mapped = altr.name_mapped;
        // Solidify the range initializer.
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.solidify(sp.queue_init, 0);  });  // 1st pass
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.solidify(sp.queue_init, 1);  });  // 2nd pass
        // Solidify the body.
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queue_body, 0);  });  // 1st pass
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queue_body, 1);  });  // 2nd pass
        // Push a new node.
        return do_append<do_for_each_statement>(queue, 0, rocket::move(sp));
      }
    case index_for_statement:
      {
        const auto& altr = this->m_stor.as<index_for_statement>();
        // `k` denotes whether the loop has an empty condition. `p` points to the triplet and the body.
        SP_queues_fixed<4> sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the initializer.
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 0);  });  // 1st pass
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 1);  });  // 2nd pass
        // Solidify the condition.
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 0);  });  // 1st pass
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 1);  });  // 2nd pass
        // Solidify the increment.
        rocket::for_each(altr.code_step, [&](const AIR_Node& node) { node.solidify(sp.queues[2], 0);  });  // 1st pass
        rocket::for_each(altr.code_step, [&](const AIR_Node& node) { node.solidify(sp.queues[2], 1);  });  // 2nd pass
        // Solidify the body.
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queues[3], 0);  });  // 1st pass
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.solidify(sp.queues[3], 1);  });  // 2nd pass
        // Push a new node.
        return do_append<do_for_statement>(queue, altr.code_cond.empty(), rocket::move(sp));
      }
    case index_try_statement:
      {
        const auto& altr = this->m_stor.as<index_try_statement>();
        // `k` is unused. `p` points to the clauses.
        SP_try sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the `try` clause.
        rocket::for_each(altr.code_try, [&](const AIR_Node& node) { node.solidify(sp.queue_try, 0);  });  // 1st pass
        rocket::for_each(altr.code_try, [&](const AIR_Node& node) { node.solidify(sp.queue_try, 1);  });  // 2nd pass
        // Solidify `catch` parameters.
        sp.sloc = altr.sloc;
        sp.name_except = altr.name_except;
        // Solidify the `catch` clause.
        rocket::for_each(altr.code_catch, [&](const AIR_Node& node) { node.solidify(sp.queue_catch, 0);  });  // 1st pass
        rocket::for_each(altr.code_catch, [&](const AIR_Node& node) { node.solidify(sp.queue_catch, 1);  });  // 2nd pass
        // Push a new node.
        return do_append<do_try_statement>(queue, 0, rocket::move(sp));
      }
    case index_throw_statement:
      {
        const auto& altr = this->m_stor.as<index_throw_statement>();
        // `k` is unused. `p` points to the source location.
        SP_sloc sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the source location.
        sp.sloc = altr.sloc;
        // Push a new node.
        return do_append<do_throw_statement>(queue, 0, rocket::move(sp));
      }
    case index_assert_statement:
      {
        const auto& altr = this->m_stor.as<index_assert_statement>();
        // `k` is `negative`. `p` points to the source location and the message.
        SP_sloc_msg sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the source location and the message.
        sp.sloc = altr.sloc;
        sp.msg = altr.msg;
        // Push a new node.
        return do_append<do_assert_statement>(queue, altr.negative, rocket::move(sp));
      }
    case index_simple_status:
      {
        const auto& altr = this->m_stor.as<index_simple_status>();
        // `k` is `status`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_simple_status>(queue, altr.status);
      }
    case index_return_by_value:
      {
        // There is no argument.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_return_by_value>(queue, 0);
      }
    case index_push_literal:
      {
        const auto& altr = this->m_stor.as<index_push_literal>();
        // `k` is unused. `p` points to a copy of `val`.
        if(ipass == 0) {
          return queue.request(altr.val);
        }
        // Push a new node.
        return do_append<do_push_literal>(queue, 0, altr.val);
      }
    case index_push_global_reference:
      {
        const auto& altr = this->m_stor.as<index_push_global_reference>();
        // `k` is unused. `p` points to the name.
        SP_name sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the name.
        sp.name = altr.name;
        // Push a new node.
        return do_append<do_push_global_reference>(queue, 0, rocket::move(sp));
      }
    case index_push_local_reference:
      {
        const auto& altr = this->m_stor.as<index_push_local_reference>();
        // `k` is `depth`. `p` points to the name.
        SP_name sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the name.
        sp.name = altr.name;
        // Push a new node.
        return do_append<do_push_local_reference>(queue, altr.depth, rocket::move(sp));
      }
    case index_push_bound_reference:
      {
        const auto& altr = this->m_stor.as<index_push_bound_reference>();
        // `k` is unused. `p` points to a copy of `ref`.
        if(ipass == 0) {
          return queue.request(altr.ref);
        }
        // Push a new node.
        return do_append<do_push_bound_reference>(queue, 0, altr.ref);
      }
    case index_define_function:
      {
        const auto& altr = this->m_stor.as<index_define_function>();
        // `k` is unused. `p` points to the name, the parameter list, and the body of the function.
        SP_func sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify everything.
        sp.options = altr.options;
        sp.sloc = altr.sloc;
        sp.name = altr.name;
        sp.params = altr.params;
        sp.body = altr.body;
        // Push a new node.
        return do_append<do_define_function>(queue, 0, rocket::move(sp));
      }
    case index_branch_expression:
      {
        const auto& altr = this->m_stor.as<index_branch_expression>();
        // `k` is `assign`. `p` points to the two branches.
        SP_queues_fixed<2> sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the true branch.
        rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 0);  });  // 1st pass
        rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 1);  });  // 2nd pass
        // Solidify the false branch.
        rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 0);  });  // 1st pass
        rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.solidify(sp.queues[1], 1);  });  // 2nd pass
        // Push a new node.
        return do_append<do_branch_expression>(queue, altr.assign, rocket::move(sp));
      }
    case index_coalescence:
      {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // `k` is `assign`. `p` points to the alternative.
        SP_queues_fixed<1> sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the alternative.
        rocket::for_each(altr.code_null, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 0);  });  // 1st pass
        rocket::for_each(altr.code_null, [&](const AIR_Node& node) { node.solidify(sp.queues[0], 1);  });  // 2nd pass
        // Push a new node.
        return do_append<do_coalescence>(queue, altr.assign, rocket::move(sp));
      }
    case index_function_call:
      {
        const auto& altr = this->m_stor.as<index_function_call>();
        // `k` is `tco_aware`. `p` points to the source location and the argument specifier vector.
        SP_call sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the source location.
        sp.sloc = altr.sloc;
        // Solidify the argument specifier vector.
        sp.args_by_refs = altr.args_by_refs;
        // Push a new node.
        return do_append<do_function_call>(queue, altr.tco_aware, rocket::move(sp));
      }
    case index_member_access:
      {
        const auto& altr = this->m_stor.as<index_member_access>();
        // `k` is unused. `p` points to the name.
        SP_name sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the name.
        sp.name = altr.name;
        // Push a new node.
        return do_append<do_member_access>(queue, 0, rocket::move(sp));
      }
    case index_push_unnamed_array:
      {
        const auto& altr = this->m_stor.as<index_push_unnamed_array>();
        // `k` is `nelems`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_push_unnamed_array>(queue, altr.nelems);
      }
    case index_push_unnamed_object:
      {
        const auto& altr = this->m_stor.as<index_push_unnamed_object>();
        // `k` is unused. `p` points to the keys.
        SP_names sp;
        if(ipass == 0) {
          return queue.request(sp);
        }
        // Solidify the keys.
        sp.names = altr.keys;
        // Push a new node.
        return do_append<do_push_unnamed_object>(queue, 0, rocket::move(sp));
      }
    case index_apply_xop_inc_post:
      {
        // There is no argument.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_inc_post>(queue, 0);
      }
    case index_apply_xop_dec_post:
      {
        // There is no argument.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_dec_post>(queue, 0);
      }
    case index_apply_xop_subscr:
      {
        // There is no argument.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_subscr>(queue, 0);
      }
    case index_apply_xop_pos:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_pos>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_pos>(queue, altr.assign);
      }
    case index_apply_xop_neg:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_neg>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_neg>(queue, altr.assign);
      }
    case index_apply_xop_notb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_notb>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_notb>(queue, altr.assign);
      }
    case index_apply_xop_notl:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_notl>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_notl>(queue, altr.assign);
      }
    case index_apply_xop_inc_pre:
      {
        // There is no argument.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_inc_pre>(queue, 0);
      }
    case index_apply_xop_dec_pre:
      {
        // There is no argument.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_dec_pre>(queue, 0);
      }
    case index_apply_xop_unset:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_unset>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_unset>(queue, altr.assign);
      }
    case index_apply_xop_lengthof:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_lengthof>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_lengthof>(queue, altr.assign);
      }
    case index_apply_xop_typeof:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_typeof>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_typeof>(queue, altr.assign);
      }
    case index_apply_xop_sqrt:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sqrt>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_sqrt>(queue, altr.assign);
      }
    case index_apply_xop_isnan:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_isnan>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_isnan>(queue, altr.assign);
      }
    case index_apply_xop_isinf:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_isinf>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_isinf>(queue, altr.assign);
      }
    case index_apply_xop_abs:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_abs>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_abs>(queue, altr.assign);
      }
    case index_apply_xop_signb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_signb>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_signb>(queue, altr.assign);
      }
    case index_apply_xop_round:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_round>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_round>(queue, altr.assign);
      }
    case index_apply_xop_floor:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_floor>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_floor>(queue, altr.assign);
      }
    case index_apply_xop_ceil:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_ceil>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_ceil>(queue, altr.assign);
      }
    case index_apply_xop_trunc:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_trunc>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_trunc>(queue, altr.assign);
      }
    case index_apply_xop_iround:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_iround>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_iround>(queue, altr.assign);
      }
    case index_apply_xop_ifloor:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_ifloor>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_ifloor>(queue, altr.assign);
      }
    case index_apply_xop_iceil:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_iceil>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_iceil>(queue, altr.assign);
      }
    case index_apply_xop_itrunc:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_itrunc>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_itrunc>(queue, altr.assign);
      }
    case index_apply_xop_cmp_xeq:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_cmp_xeq>();
        // `k` is `assign` and `negative`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_cmp_xeq>(queue, SK_xrel(altr.assign, compare_equal, altr.negative));
      }
    case index_apply_xop_cmp_xrel:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_cmp_xrel>();
        // `k` is `assign`, `expect` and `negative`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_cmp_xrel>(queue, SK_xrel(altr.assign, altr.expect, altr.negative));
      }
    case index_apply_xop_cmp_3way:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_cmp_3way>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_cmp_3way>(queue, altr.assign);
      }
    case index_apply_xop_add:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_add>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_add>(queue, altr.assign);
      }
    case index_apply_xop_sub:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sub>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_sub>(queue, altr.assign);
      }
    case index_apply_xop_mul:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_mul>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_mul>(queue, altr.assign);
      }
    case index_apply_xop_div:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_div>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_div>(queue, altr.assign);
      }
    case index_apply_xop_mod:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_mod>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_mod>(queue, altr.assign);
      }
    case index_apply_xop_sll:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sll>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_sll>(queue, altr.assign);
      }
    case index_apply_xop_srl:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_srl>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_srl>(queue, altr.assign);
      }
    case index_apply_xop_sla:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sla>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_sla>(queue, altr.assign);
      }
    case index_apply_xop_sra:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sra>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_sra>(queue, altr.assign);
      }
    case index_apply_xop_andb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_andb>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_andb>(queue, altr.assign);
      }
    case index_apply_xop_orb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_orb>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_orb>(queue, altr.assign);
      }
    case index_apply_xop_xorb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_xorb>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_xorb>(queue, altr.assign);
      }
    case index_apply_xop_assign:
      {
        // There is no argument.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_assign>(queue, 0);
      }
    case index_apply_xop_fma:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_fma>();
        // `k` is `assign`. `p` is unused.
        if(ipass == 0) {
          return queue.request();
        }
        // Push a new node.
        return do_append<do_apply_xop_fma>(queue, altr.assign);
      }
    default:
      ASTERIA_TERMINATE("An unknown AIR node type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
