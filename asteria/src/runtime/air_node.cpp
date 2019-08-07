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

    AIR_Node::Status do_execute_statement_list(Executive_Context& ctx, const cow_vector<AIR_Node>& code)
      {
        // Execute all nodes sequentially.
        auto status = AIR_Node::status_next;
        rocket::any_of(code, [&](const AIR_Node& node) { return (status = node.execute(ctx)) != AIR_Node::status_next;  });
        return status;
      }

    AIR_Node::Status do_execute_block(const cow_vector<AIR_Node>& code, const Executive_Context& ctx)
      {
        // Execute the block on a new context.
        Executive_Context ctx_body(1, ctx);
        auto status = do_execute_statement_list(ctx_body, code);
        return status;
      }

    AIR_Node::Status do_execute_catch(const cow_vector<AIR_Node>& code, const phsh_string& name_except, const Exception& except, const Executive_Context& ctx)
      {
        // Create a fresh context.
        Executive_Context ctx_catch(1, ctx);
        // Set the exception reference.
        Reference_Root::S_temporary xref_except = { except.get_value() };
        ctx_catch.open_named_reference(name_except) = rocket::move(xref_except);
        // Set backtrace frames.
        G_array backtrace;
        for(size_t i = 0; i != except.count_frames(); ++i) {
          const auto& frame = except.get_frame(i);
          G_object r;
          // Translate each frame into a human-readable format.
          r.try_emplace(rocket::sref("frame"), G_string(rocket::sref(Backtrace_Frame::stringify_ftype(frame.ftype()))));
          r.try_emplace(rocket::sref("file"), G_string(frame.file()));
          r.try_emplace(rocket::sref("line"), G_integer(frame.line()));
          r.try_emplace(rocket::sref("value"), frame.value());
          // Append this frame.
          backtrace.emplace_back(rocket::move(r));
        }
        ASTERIA_DEBUG_LOG("Exception backtrace:\n", Value(backtrace));
        Reference_Root::S_constant xref_bt = { rocket::move(backtrace) };
        ctx_catch.open_named_reference(rocket::sref("__backtrace")) = rocket::move(xref_bt);
        // Execute the block now.
        auto status = do_execute_statement_list(ctx_catch, code);
        return status;
      }

    const Value& do_evaluate(Executive_Context& ctx, const cow_vector<AIR_Node>& code)
      {
        if(code.empty()) {
          // Return a static `null`.
          return Value::null();
        }
        // Clear the stack.
        ctx.stack().clear_references();
        // Evaluate all nodes.
        rocket::for_each(code, [&](const AIR_Node& node) { node.execute(ctx);  });
        // The result will have been pushed onto the top.
        return ctx.stack().get_top_reference().read();
      }

    }

AIR_Node::Status AIR_Node::execute(Executive_Context& ctx) const
  {
    switch(this->index()) {
    case index_clear_stack:
      {
        // Clear the stack.
        ctx.stack().clear_references();
        return status_next;
      }
    case index_execute_block:
      {
        const auto& altr = this->m_stor.as<index_execute_block>();
        // Execute the block in a new context.
        return do_execute_block(altr.code_body, ctx);
      }
    case index_declare_variable:
      {
        const auto& altr = this->m_stor.as<index_declare_variable>();
        // Allocate a variable and initialize it to `null`.
        auto var = ctx.global().create_variable();
        var->reset(altr.sloc, G_null(), altr.immutable);
        // Inject the variable into the current context.
        Reference_Root::S_variable xref = { rocket::move(var) };
        ctx.open_named_reference(altr.name) = xref;
        // Push a copy of the reference onto the stack.
        ctx.stack().push_reference(rocket::move(xref));
        return status_next;
      }
    case index_initialize_variable:
      {
        const auto& altr = this->m_stor.as<index_initialize_variable>();
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this code.
        auto value = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        // Get the variable back.
        auto var = ctx.stack().get_top_reference().get_variable_opt();
        ROCKET_ASSERT(var);
        // Initialize it.
        var->reset(rocket::move(value), altr.immutable);
        return status_next;
      }
    case index_if_statement:
      {
        const auto& altr = this->m_stor.as<index_if_statement>();
        // Pick a branch basing on the condition.
        bool cond = ctx.stack().get_top_reference().read().test() != altr.negative;
        if(cond) {
          return do_execute_block(altr.code_true, ctx);
        }
        return do_execute_block(altr.code_false, ctx);
      }
    case index_switch_statement:
      {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        // Read the value of the control expression.
        auto value = ctx.stack().get_top_reference().read();
        // Find a target clause.
        // This is different from a C `switch` statement where `case` labels must have constant operands.
        size_t tpos = SIZE_MAX;
        for(size_t i = 0; i != altr.clauses.size(); ++i) {
          const auto& code_case = altr.clauses[i].first;
          if(code_case.empty()) {
            // This is a `default` label.
            if(tpos != SIZE_MAX) {
              ASTERIA_THROW_RUNTIME_ERROR("Multiple `default` clauses have been found in this `switch` statement.");
            }
            tpos = i;
            continue;
          }
          // This is a `case` label.
          // Evaluate the operand and check whether it equals `value`.
          if(value.compare(do_evaluate(ctx, code_case)) == Value::compare_equal) {
            tpos = i;
            break;
          }
        }
        if(tpos != SIZE_MAX) {
          // Jump to the clause denoted by `tpos`.
          // Note that all clauses share the same context.
          Executive_Context ctx_body(1, ctx);
          // Skip clauses that precede `tpos`.
          for(size_t i = 0; i != tpos; ++i) {
            const auto& names = altr.clauses[i].second.second;
            rocket::for_each(names, [&](const phsh_string& name) { ctx_body.open_named_reference(name) = Reference_Root::S_null();  });
          }
          // Execute all clauses from `tpos`.
          for(size_t i = tpos; i != altr.clauses.size(); ++i) {
            const auto& code_clause = altr.clauses[i].second.first;
            auto status = do_execute_statement_list(ctx_body, code_clause);
            if(rocket::is_any_of(status, { status_break_unspec, status_break_switch })) {
              break;
            }
            if(status != status_next) {
              // Forward any status codes unexpected to the caller.
              return status;
            }
          }
        }
        return status_next;
      }
    case index_do_while_statement:
      {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        do {
          // Execute the body.
          auto status = do_execute_block(altr.code_body, ctx);
          if(rocket::is_any_of(status, { status_break_unspec, status_break_while })) {
            break;
          }
          if(rocket::is_none_of(status, { status_next, status_continue_unspec, status_continue_while })) {
            // Forward any status codes unexpected to the caller.
            return status;
          }
          // Check the condition after each iteration.
        } while(do_evaluate(ctx, altr.code_cond).test() != altr.negative);
        return status_next;
      }
    case index_while_statement:
      {
        const auto& altr = this->m_stor.as<index_while_statement>();
        // Check the condition before every iteration.
        while(do_evaluate(ctx, altr.code_cond).test() != altr.negative) {
          // Execute the body.
          auto status = do_execute_block(altr.code_body, ctx);
          if(rocket::is_any_of(status, { status_break_unspec, status_break_while })) {
            break;
          }
          if(rocket::is_none_of(status, { status_next, status_continue_unspec, status_continue_while })) {
            // Forward any status codes unexpected to the caller.
            return status;
          }
        }
        return status_next;
      }
    case index_for_each_statement:
      {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        // Note that the key and value references outlasts every iteration, so we have to create an outer contexts here.
        Executive_Context ctx_for(1, ctx);
        // Create a variable for the key.
        auto key = ctx_for.global().create_variable();
        key->reset(Source_Location(rocket::sref("<ranged-for>"), 0), G_null(), true);
        Reference_Root::S_variable xref_key = { key };
        ctx_for.open_named_reference(altr.name_key) = rocket::move(xref_key);
        // Create the mapped reference.
        auto& mapped = ctx_for.open_named_reference(altr.name_mapped);
        mapped = Reference_Root::S_null();
        // Clear the stack.
        ctx_for.stack().clear_references();
        // Evaluate the range initializer.
        ROCKET_ASSERT(!altr.code_init.empty());
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.execute(ctx_for);  });
        // Set the range up.
        mapped = rocket::move(ctx_for.stack().open_top_reference());
        auto range = mapped.read();
        // The range value has been saved. This ensures we are immune to dangling pointers if the loop body attempts to modify it.
        // Also be advised that the mapped parameter is a reference rather than a value.
        if(range.is_array()) {
          const auto& array = range.as_array();
          // The key is the subscript of an element of the array.
          for(size_t i = 0; i != array.size(); ++i) {
            // Set up the key and mapped arguments.
            key->reset(G_integer(i), true);
            Reference_Modifier::S_array_index xmod = { static_cast<int64_t>(i) };
            mapped.zoom_in(rocket::move(xmod));
            // Execute the loop body.
            auto status = do_execute_block(altr.code_body, ctx_for);
            if(rocket::is_any_of(status, { status_break_unspec, status_break_for })) {
              break;
            }
            if(rocket::is_none_of(status, { status_next, status_continue_unspec, status_continue_for })) {
              // Forward any status codes unexpected to the caller.
              return status;
            }
            // Restore the mapped reference.
            mapped.zoom_out();
          }
          return status_next;
        }
        else if(range.is_object()) {
          const auto& object = range.as_object();
          // The key is a string.
          for(auto q = object.begin(); q != object.end(); ++q) {
            // Set up the key and mapped arguments.
            key->reset(G_string(q->first), true);
            Reference_Modifier::S_object_key xmod = { q->first };
            mapped.zoom_in(rocket::move(xmod));
            // Execute the loop body.
            auto status = do_execute_block(altr.code_body, ctx_for);
            if(rocket::is_any_of(status, { status_break_unspec, status_break_for })) {
              break;
            }
            if(rocket::is_none_of(status, { status_next, status_continue_unspec, status_continue_for })) {
              // Forward any status codes unexpected to the caller.
              return status;
            }
            // Restore the mapped reference.
            mapped.zoom_out();
          }
          return status_next;
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("The `for each` statement does not accept a range of type `", range.gtype_name(), "`.");
        }
      }
    case index_for_statement:
      {
        const auto& altr = this->m_stor.as<index_for_statement>();
        // Note that names declared in the first segment of a for-statement outlasts every iteration, so we have to create an outer contexts here.
        Executive_Context ctx_for(1, ctx);
        // Execute the loop initializer.
        // XXX: Techinically it should only be a definition or an expression statement.
        auto status = do_execute_statement_list(ctx_for, altr.code_init);
        if(status != status_next) {
          // Forward any status codes unexpected to the caller.
          return status;
        }
        // If the condition is empty, the loop is infinite.
        while(altr.code_cond.empty() || do_evaluate(ctx_for, altr.code_cond).test()) {
          // Execute the body.
          status = do_execute_block(altr.code_body, ctx_for);
          if(rocket::is_any_of(status, { status_break_unspec, status_break_for })) {
            break;
          }
          if(rocket::is_none_of(status, { status_next, status_continue_unspec, status_continue_for })) {
            // Forward any status codes unexpected to the caller.
            return status;
          }
          // Execute the loop increment.
          do_evaluate(ctx_for, altr.code_step);
        }
        return status_next;
      }
    case index_try_statement:
      {
        const auto& altr = this->m_stor.as<index_try_statement>();
        // This is almost identical to JavaScript.
        try {
          // Execute the `try` clause. If no exception is thrown, this will have little overhead.
          return do_execute_block(altr.code_try, ctx);
        }
        catch(Exception& except) {
          // Reuse the exception object. Don't bother allocating a new one.
          except.push_frame_catch(altr.sloc);
          ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", except);
          return do_execute_catch(altr.code_catch, altr.name_except, except, ctx);
        }
        catch(const std::exception& stdex) {
          // Translate the exception.
          Exception except(stdex);
          except.push_frame_catch(altr.sloc);
          ASTERIA_DEBUG_LOG("Translated `std::exception`: ", except);
          return do_execute_catch(altr.code_catch, altr.name_except, except, ctx);
        }
      }
    case index_throw_statement:
      {
        const auto& altr = this->m_stor.as<index_throw_statement>();
        // Read the value to throw.
        // Note that the operand must not have been empty for this code.
        auto value = ctx.stack().get_top_reference().read();
        // Unpack the nested exception, if any.
        opt<Exception> qnested;
        // Rethrow the current exception to get its effective type.
        auto eptr = std::current_exception();
        if(eptr) {
          try {
            std::rethrow_exception(eptr);
          }
          catch(Exception& except) {
            // Modify the exception in place. Don't bother allocating a new one.
            except.push_frame_throw(altr.sloc, rocket::move(value));
            throw;
          }
          catch(const std::exception& stdex) {
            // Translate the exception.
            qnested.emplace(stdex);
          }
        }
        if(!qnested) {
          // If no nested exception exists, construct a fresh one.
          qnested.emplace(altr.sloc, rocket::move(value));
        }
        throw *qnested;
      }
    case index_assert_statement:
      {
        const auto& altr = this->m_stor.as<index_assert_statement>();
        // Read the value to check.
        bool cond = ctx.stack().get_top_reference().read().test() != altr.negative;
        if(ROCKET_EXPECT(cond)) {
          // The assertion has succeeded.
          return status_next;
        }
        // The assertion has failed.
        cow_osstream fmtss;
        fmtss.imbue(std::locale::classic());
        fmtss << "Assertion failed at \'" << altr.sloc << '\'';
        // Append the message if one is provided.
        if(!altr.msg.empty())
          fmtss << ": " << altr.msg;
        else
          fmtss << '!';
        // Throw a `Runtime_Error`.
        throw_runtime_error(__func__, fmtss.extract_string());
      }
    case index_simple_status:
      {
        const auto& altr = this->m_stor.as<index_simple_status>();
        // Just return the status.
        return altr.status;
      }
    case index_return_by_value:
      {
        // The result will have been pushed onto the top.
        auto& ref = ctx.stack().open_top_reference();
        // Convert the result to an rvalue.
        // TCO wrappers are forwarded as is.
        if(ROCKET_UNEXPECT(ref.is_lvalue())) {
          ref.convert_to_rvalue();
        }
        return status_return;
      }
    default:
      ASTERIA_TERMINATE("An unknown AIR node type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

Variable_Callback& AIR_Node::enumerate_variables(Variable_Callback& callback) const
  {
    switch(this->index()) {
    case index_clear_stack:
      {
        return callback;
      }
    case index_execute_block:
      {
        const auto& altr = this->m_stor.as<index_execute_block>();
        // Enumerate all nodes of the body.
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_declare_variable:
    case index_initialize_variable:
      {
        return callback;
      }
    case index_if_statement:
      {
        const auto& altr = this->m_stor.as<index_if_statement>();
        // Enumerate all nodes of both branches.
        rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_switch_statement:
      {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        for(size_t i = 0; i != altr.clauses.size(); ++i) {
          // Enumerate all nodes of both the label and the clause.
          rocket::for_each(altr.clauses[i].first, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
          rocket::for_each(altr.clauses[i].second.first, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        }
        return callback;
      }
    case index_do_while_statement:
      {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        // Enumerate all nodes of both the body and the condition.
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_while_statement:
      {
        const auto& altr = this->m_stor.as<index_while_statement>();
        // Enumerate all nodes of both the condition and the body.
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_for_each_statement:
      {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        // Enumerate all nodes of both the range initializer and the body.
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_for_statement:
      {
        const auto& altr = this->m_stor.as<index_for_statement>();
        // Enumerate all nodes of both the triplet and the body.
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_step, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_try_statement:
      {
        const auto& altr = this->m_stor.as<index_try_statement>();
        // Enumerate all nodes of both clauses.
        rocket::for_each(altr.code_try, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_catch, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_throw_statement:
    case index_assert_statement:
    case index_simple_status:
    case index_return_by_value:
    case index_push_literal:
    case index_push_global_reference:
    case index_push_local_reference:
      {
        return callback;
      }
    case index_push_bound_reference:
      {
        const auto& altr = this->m_stor.as<index_push_bound_reference>();
        // Descend into the bound reference.
        altr.ref.enumerate_variables(callback);
        return callback;
      }
    case index_define_function:
      {
        return callback;
      }
    case index_branch_expression:
      {
        const auto& altr = this->m_stor.as<index_branch_expression>();
        // Enumerate all nodes of both branches.
        rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_coalescence:
      {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // Enumerate all nodes of the null branch.
        rocket::for_each(altr.code_null, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_function_call_tail:
    case index_function_call_plain:
    case index_member_access:
    case index_push_unnamed_array:
    case index_push_unnamed_object:
    case index_apply_xop_inc_post:
    case index_apply_xop_dec_post:
    case index_apply_xop_subscr:
    case index_apply_xop_pos:
    case index_apply_xop_neg:
    case index_apply_xop_notb:
    case index_apply_xop_notl:
    case index_apply_xop_inc:
    case index_apply_xop_dec:
    case index_apply_xop_unset:
    case index_apply_xop_lengthof:
    case index_apply_xop_typeof:
    case index_apply_xop_sqrt:
    case index_apply_xop_isnan:
    case index_apply_xop_isinf:
    case index_apply_xop_abs:
    case index_apply_xop_signb:
    case index_apply_xop_round:
    case index_apply_xop_floor:
    case index_apply_xop_ceil:
    case index_apply_xop_trunc:
    case index_apply_xop_iround:
    case index_apply_xop_ifloor:
    case index_apply_xop_iceil:
    case index_apply_xop_itrunc:
    case index_apply_xop_cmp_xeq:
    case index_apply_xop_cmp_xrel:
    case index_apply_xop_cmp_3way:
    case index_apply_xop_add:
    case index_apply_xop_sub:
    case index_apply_xop_mul:
    case index_apply_xop_div:
    case index_apply_xop_mod:
    case index_apply_xop_sll:
    case index_apply_xop_srl:
    case index_apply_xop_sla:
    case index_apply_xop_sra:
    case index_apply_xop_andb:
    case index_apply_xop_orb:
    case index_apply_xop_xorb:
    case index_apply_xop_assign:
    case index_apply_xop_fma:
      {
        return callback;
      }
    default:
      ASTERIA_TERMINATE("An unknown AIR node type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
