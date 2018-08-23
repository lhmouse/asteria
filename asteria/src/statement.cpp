// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "statement.hpp"
#include "context.hpp"
#include "variable.hpp"
#include "instantiated_function.hpp"
#include "exception.hpp"
#include "backtracer.hpp"
#include "utilities.hpp"

namespace Asteria {

Statement::Statement(Statement &&) noexcept
  = default;
Statement & Statement::operator=(Statement &&) noexcept
  = default;
Statement::~Statement()
  = default;

namespace {
  template<typename ...ParamsT>
    void do_safe_set_named_reference(Spref<Context> ctx_inout, const char *desc, const String &name, ParamsT &&...params)
      {
        if(is_name_reserved(name)) {
          ASTERIA_THROW_RUNTIME_ERROR("The ", desc, " name `", name, "` is reserved and cannot be used.");
        }
        if(name.empty()) {
          return;
        }
        ctx_inout->set_named_reference(name, Reference(std::forward<ParamsT>(params)...));
      }
}

Statement bind_statement_partial(Spref<Context> ctx_inout, const Statement &stmt)
  {
    if(ctx_inout->is_feigned() == false) {
      ASTERIA_THROW_RUNTIME_ERROR("`bind_statement_partial()` cannot be called on a genuine context.");
    }
    switch(stmt.index()) {
    case Statement::index_expression:
      {
        const auto &cand = stmt.as<Statement::S_expression>();
        // Bind the expression recursively.
        auto expr_bnd = bind_expression(cand.expr, ctx_inout);
        Statement::S_expression cand_bnd = { std::move(expr_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_var_def:
      {
        const auto &cand = stmt.as<Statement::S_var_def>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_inout, "variable", cand.name);
        // Bind the initializer recursively.
        auto init_bnd = bind_expression(cand.init, ctx_inout);
        Statement::S_var_def cand_bnd = { cand.name, cand.immutable, std::move(init_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_func_def:
      {
        const auto &cand = stmt.as<Statement::S_func_def>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_inout, "function", cand.name);
        // Bind the function body recursively.
        const auto ctx_feigned = allocate<Context>(ctx_inout, true);
        initialize_function_context(ctx_feigned, cand.params, cand.file, cand.line, { }, { });
        auto body_bnd = bind_block_in_place(ctx_feigned, cand.body);
        Statement::S_func_def cand_bnd = { cand.name, cand.params, cand.file, cand.line, std::move(body_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_if:
      {
        const auto &cand = stmt.as<Statement::S_if>();
        // Bind the condition and both branches recursively.
        auto cond_bnd = bind_expression(cand.cond, ctx_inout);
        auto branch_true_bnd = bind_block(cand.branch_true, ctx_inout);
        auto branch_false_bnd = bind_block(cand.branch_false, ctx_inout);
        Statement::S_if cand_bnd = { std::move(cond_bnd), std::move(branch_true_bnd), std::move(branch_false_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_switch:
      {
        const auto &cand = stmt.as<Statement::S_switch>();
        // Bind the control expression and all clauses recursively.
        auto ctrl_bnd = bind_expression(cand.ctrl, ctx_inout);
        // Note that all `switch` clauses share the same context.
        const auto ctx_feigned = allocate<Context>(ctx_inout, true);
        Bivector<Vector<Xpnode>, Vector<Statement>> clauses_bnd;
        clauses_bnd.reserve(cand.clauses.size());
        for(const auto &pair : cand.clauses) {
          auto first_bnd = bind_expression(pair.first, ctx_feigned);
          auto second_bnd = bind_block_in_place(ctx_feigned, pair.second);
          clauses_bnd.emplace_back(std::move(first_bnd), std::move(second_bnd));
        }
        Statement::S_switch cand_bnd = { std::move(ctrl_bnd), std::move(clauses_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_do_while:
      {
        const auto &cand = stmt.as<Statement::S_do_while>();
        // Bind the loop body and condition recursively.
        auto body_bnd = bind_block(cand.body, ctx_inout);
        auto cond_bnd = bind_expression(cand.cond, ctx_inout);
        Statement::S_do_while cand_bnd = { std::move(body_bnd), std::move(cond_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_while:
      {
        const auto &cand = stmt.as<Statement::S_while>();
        // Bind the condition and loop body recursively.
        auto cond_bnd = bind_expression(cand.cond, ctx_inout);
        auto body_bnd = bind_block(cand.body, ctx_inout);
        Statement::S_while cand_bnd = { std::move(cond_bnd), std::move(body_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_for:
      {
        const auto &cand = stmt.as<Statement::S_for>();
        // If the initialization part is a variable definition, the variable defined shall not outlast the loop body.
        const auto ctx_feigned = allocate<Context>(ctx_inout, true);
        do_safe_set_named_reference(ctx_feigned, "`for` variable", cand.var_name);
        // Bind the loop variable initializer, condition, step expression and loop loop body recursively.
        auto var_init_bnd = bind_expression(cand.var_init, ctx_feigned);
        auto cond_bnd = bind_expression(cand.cond, ctx_feigned);
        auto step_bnd = bind_expression(cand.step, ctx_feigned);
        // It should be safe to use `bind_block_in_place()` in place of `bind_block()` here, as the body is only ever iterated once.
        // Otherwises a fresh context has to be created for each iteration.
        auto body_bnd = bind_block_in_place(ctx_feigned, cand.body);
        Statement::S_for cand_bnd = { cand.var_name, cand.var_immutable, std::move(var_init_bnd), std::move(cond_bnd), std::move(step_bnd), std::move(body_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_for_each:
      {
        const auto &cand = stmt.as<Statement::S_for_each>();
        // The key and mapped variables shall not outlast the loop body.
        const auto ctx_feigned = allocate<Context>(ctx_inout, true);
        do_safe_set_named_reference(ctx_feigned, "`for each` key", cand.key_name);
        do_safe_set_named_reference(ctx_feigned, "`for each` reference", cand.mapped_name);
        // Bind the range initializer and loop body recursively.
        auto range_init_bnd = bind_expression(cand.range_init, ctx_feigned);
        // It should be safe to use `bind_block_in_place()` in place of `bind_block()` here, as the body is only ever iterated once.
        // Otherwises a fresh context has to be created for each iteration.
        auto body_bnd = bind_block(cand.body, ctx_feigned);
        Statement::S_for_each cand_bnd = { cand.key_name, cand.mapped_name, std::move(range_init_bnd), std::move(body_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_try:
      {
        const auto &cand = stmt.as<Statement::S_try>();
        // The `try` branch needs no special treatement.
        auto body_try_bnd = bind_block(cand.body_try, ctx_inout);
        // The exception variable shall not outlast the `catch` body.
        const auto ctx_feigned = allocate<Context>(ctx_inout, true);
        do_safe_set_named_reference(ctx_feigned, "exception", cand.except_name);
        // Bind the `catch` branch recursively.
        auto body_catch_bnd = bind_block_in_place(ctx_feigned, cand.body_catch);
        Statement::S_try cand_bnd = { std::move(body_try_bnd), cand.except_name, std::move(body_catch_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_break:
      {
        const auto &cand = stmt.as<Statement::S_break>();
        // Copy it as-is.
        Statement::S_break cand_bnd = { cand.target };
        return std::move(cand_bnd);
      }
    case Statement::index_continue:
      {
        const auto &cand = stmt.as<Statement::S_continue>();
        // Copy it as-is.
        Statement::S_continue cand_bnd = { cand.target };
        return std::move(cand_bnd);
      }
    case Statement::index_throw:
      {
        const auto &cand = stmt.as<Statement::S_throw>();
        // Bind the exception initializer recursively.
        auto expr_bnd = bind_expression(cand.expr, ctx_inout);
        Statement::S_throw cand_bnd = { std::move(expr_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_return:
      {
        const auto &cand = stmt.as<Statement::S_return>();
        // Bind the result initializer recursively.
        auto expr_bnd = bind_expression(cand.expr, ctx_inout);
        Statement::S_return cand_bnd = { std::move(expr_bnd) };
        return std::move(cand_bnd);
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", stmt.index(), "` has been encountered.");
    }
  }
Vector<Statement> bind_block_in_place(Spref<Context> ctx_inout, const Vector<Statement> &block)
  {
    if(ctx_inout->is_feigned() == false) {
      ASTERIA_THROW_RUNTIME_ERROR("`bind_block_in_place()` cannot be called on a genuine context.");
    }
    Vector<Statement> block_bnd;
    block_bnd.reserve(block.size());
    for(const auto &stmt : block) {
      auto cand_bnd = bind_statement_partial(ctx_inout, stmt);
      block_bnd.emplace_back(std::move(cand_bnd));
    }
    return block_bnd;
  }
Statement::Status execute_statement_partial(Reference &ref_out, Spref<Context> ctx_inout, const Statement &stmt)
  {
    if(ctx_inout->is_feigned() != false) {
      ASTERIA_THROW_RUNTIME_ERROR("`execute_statement_partial()` cannot be called on a feigned context.");
    }
    switch(stmt.index()) {
    case Statement::index_expression:
      {
        const auto &cand = stmt.as<Statement::S_expression>();
        // Evaluate the expression.
        ref_out = evaluate_expression(cand.expr, ctx_inout);
        return Statement::status_next;
      }
    case Statement::index_var_def:
      {
        const auto &cand = stmt.as<Statement::S_var_def>();
        // Create a dummy reference for further name lookups.
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_inout, "variable", cand.name);
        // Create a variable using the initializer.
        ref_out = evaluate_expression(cand.init, ctx_inout);
        auto value = read_reference(ref_out);
        auto var = allocate<Variable>(std::move(value), cand.immutable);
        // Reset the reference.
        Reference_root::S_variable ref_c = { std::move(var) };
        ref_out.set_root(std::move(ref_c));
        do_safe_set_named_reference(ctx_inout, "variable", cand.name, ref_out);
        ASTERIA_DEBUG_LOG("Created named variable: name = ", cand.name, ", immutable = ", cand.immutable);
        return Statement::status_next;
      }
    case Statement::index_func_def:
      {
        const auto &cand = stmt.as<Statement::S_func_def>();
        // Create a dummy reference for further name lookups.
        // A function becomes visible before its definition, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_inout, "function", cand.name);
        // Bind the function body recursively.
        const auto ctx_feigned = allocate<Context>(ctx_inout, true);
        initialize_function_context(ctx_feigned, cand.params, cand.file, cand.line, { }, { });
        auto body_bnd = bind_block_in_place(ctx_feigned, cand.body);
        auto func = allocate<Instantiated_function>(cand.params, cand.file, cand.line, std::move(body_bnd));
        auto var = allocate<Variable>(D_function(std::move(func)), true);
        // Reset the reference.
        Reference_root::S_variable ref_c = { std::move(var) };
        ref_out.set_root(std::move(ref_c));
        do_safe_set_named_reference(ctx_inout, "function", cand.name, ref_out);
        ASTERIA_DEBUG_LOG("Created named function: name = ", cand.name, ", file:line = ", cand.file, ':', cand.line);
        return Statement::status_next;
      }
    case Statement::index_if:
      {
        const auto &cand = stmt.as<Statement::S_if>();
        // Evaluate the condition and pick a branch.
        ref_out = evaluate_expression(cand.cond, ctx_inout);
        const auto branch_taken = test_value(read_reference(ref_out)) ? std::ref(cand.branch_true) : std::ref(cand.branch_false);
        const auto status = execute_block(ref_out, branch_taken, ctx_inout);
        if(status != Statement::status_next) {
          return status;
        }
        return Statement::status_next;
      }
    case Statement::index_switch:
      {
        const auto &cand = stmt.as<Statement::S_switch>();
        // Evaluate the control expression.
        ref_out = evaluate_expression(cand.ctrl, ctx_inout);
        const auto value_ctrl = read_reference(ref_out);
        // Note that all `switch` clauses share the same context.
        // We will iterate from the first clause to the last one. If a `default` clause is encountered in the middle
        // and there is no match `case` clause, we will have to jump back into half of the scope. To simplify design,
        // a nested scope is created when a `default` clause is encountered. When jumping to the `default` scope, we
        // simply discard the new scope.
        auto ctx_next = allocate<Context>(ctx_inout, false);
        auto ctx_probing = ctx_next;
        auto match = cand.clauses.end();
        for(auto it = cand.clauses.begin(); it != cand.clauses.end(); ++it) {
          if(it->first.empty()) {
            // This is a `default` clause.
            if(match != cand.clauses.end()) {
              ASTERIA_THROW_RUNTIME_ERROR("Multiple `default` clauses exist in the same `switch` statement.");
            }
            ctx_probing = allocate<Context>(ctx_next, false);
            match = it;
          } else {
            // This is a `case` clause.
            ref_out = evaluate_expression(it->first, ctx_next);
            const auto value_comp = read_reference(ref_out);
            if(compare_values(value_ctrl, value_comp) == Value::compare_equal) {
              ctx_next = ctx_probing;
              match = it;
              break;
            }
          }
          // Skip the clause.
          // If there are any definitions in it, initialize the declared objects to `null`.
          for(const auto &kstmt : it->second) {
            if(kstmt.index() == Statement::index_var_def) {
              const auto &kcand = kstmt.as<Statement::S_var_def>();
              do_safe_set_named_reference(ctx_next, "variable", kcand.name);
              ASTERIA_DEBUG_LOG("Skipped named variable: name = ", kcand.name, ", immutable = ", kcand.immutable);
            }
            else if(kstmt.index() == Statement::index_func_def) {
              const auto &kcand = kstmt.as<Statement::S_func_def>();
              do_safe_set_named_reference(ctx_next, "function", kcand.name);
              ASTERIA_DEBUG_LOG("Skipped named function: name = ", kcand.name, ", file:line = ", kcand.file, ':', kcand.line);
            }
          }
        }
        // Iterate from the match clause to the end of the body, falling through clause boundaries if any.
        for(auto it = match; it != cand.clauses.end(); ++it) {
          const auto status = execute_block_in_place(ref_out, ctx_next, it->second);
          if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_switch })) {
            // Break out of the body as requested.
            break;
          }
          if(status != Statement::status_next) {
            // Forward anything unexpected to the caller.
            return status;
          }
        }
        return Statement::status_next;
      }
    case Statement::index_do_while:
      {
        const auto &cand = stmt.as<Statement::S_do_while>();
        for(;;) {
          // Execute the loop body.
          const auto status = execute_block(ref_out, cand.body, ctx_inout);
          if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_while })) {
            // Break out of the body as requested.
            break;
          }
          if(rocket::is_none_of(status, { Statement::status_next, Statement::status_continue_unspec, Statement::status_continue_while })) {
            // Forward anything unexpected to the caller.
            return status;
          }
          // Check the loop condition.
          ref_out = evaluate_expression(cand.cond, ctx_inout);
          if(test_value(read_reference(ref_out)) == false) {
             break;
          }
        }
        return Statement::status_next;
      }
    case Statement::index_while:
      {
        const auto &cand = stmt.as<Statement::S_while>();
        for(;;) {
          // Check the loop condition.
          ref_out = evaluate_expression(cand.cond, ctx_inout);
          if(test_value(read_reference(ref_out)) == false) {
            break;
          }
          // Execute the loop body.
          const auto status = execute_block(ref_out, cand.body, ctx_inout);
          if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_while })) {
            // Break out of the body as requested.
            break;
          }
          if(rocket::is_none_of(status, { Statement::status_next, Statement::status_continue_unspec, Statement::status_continue_while })) {
            // Forward anything unexpected to the caller.
            return status;
          }
        }
        return Statement::status_next;
      }
    case Statement::index_for:
      {
        const auto &cand = stmt.as<Statement::S_for>();
        // If the initialization part is a variable definition, the variable defined shall not outlast the loop body.
        auto ctx_next = allocate<Context>(ctx_inout, false);
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_next, "`for` variable", cand.var_name);
        // Create a variable using the initializer.
        ref_out = evaluate_expression(cand.var_init, ctx_next);
        if(cand.var_name.empty() == false) {
          auto value = read_reference(ref_out);
          auto var = allocate<Variable>(std::move(value), cand.var_immutable);
          // Reset the reference.
          Reference_root::S_variable ref_c = { std::move(var) };
          do_safe_set_named_reference(ctx_next, "`for` variable", cand.var_name, std::move(ref_c));
          ASTERIA_DEBUG_LOG("Created named variable with `for` scope: name = ", cand.var_name, ", immutable = ", cand.var_immutable);
        }
        for(;;) {
          // Check the loop condition.
          if(cand.cond.empty() == false) {
            ref_out = evaluate_expression(cand.cond, ctx_next);
            if(test_value(read_reference(ref_out)) == false) {
              break;
            }
          }
          // Execute the loop body.
          const auto status = execute_block(ref_out, cand.body, ctx_next);
          if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_for })) {
            // Break out of the body as requested.
            break;
          }
          if(rocket::is_none_of(status, { Statement::status_next, Statement::status_continue_unspec, Statement::status_continue_for })) {
            // Forward anything unexpected to the caller.
            return status;
          }
          // Evaluate the loop step expression.
          evaluate_expression(cand.step, ctx_next);
        }
        return Statement::status_next;
      }
    case Statement::index_for_each:
      {
        const auto &cand = stmt.as<Statement::S_for_each>();
        // The key and mapped variables shall not outlast the loop body.
        auto ctx_next = allocate<Context>(ctx_inout, false);
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_next, "`for each` key", cand.key_name);
        do_safe_set_named_reference(ctx_next, "`for each` reference", cand.mapped_name);
        // Calculate the range using the initializer.
        ref_out = evaluate_expression(cand.range_init, ctx_next);
        const auto mapped_base = ref_out;
        const auto range_value = read_reference(ref_out);
        if(range_value.type() == Value::type_array) {
          const auto &array = range_value.as<D_array>();
          for(auto it = array.begin(); it != array.end(); ++it) {
            if(!ctx_next) {
              // Create a fresh context for this loop.
              ctx_next = allocate<Context>(ctx_inout, false);
            }
            const auto index = static_cast<Signed>(it - array.begin());
            // Initialize the per-loop key constant.
            ref_out = reference_constant(D_integer(index));
            do_safe_set_named_reference(ctx_next, "`for each` key", cand.key_name, ref_out);
            ASTERIA_DEBUG_LOG("Created key constant with `for each` scope: name = ", cand.key_name);
            // Initialize the per-loop value reference.
            Reference_modifier::S_array_index refmod_c = { index };
            ref_out = indirect_reference_from(mapped_base, std::move(refmod_c));
            do_safe_set_named_reference(ctx_next, "`for each` reference", cand.mapped_name, ref_out);
            ASTERIA_DEBUG_LOG("Created value reference with `for each` scope: name = ", cand.mapped_name);
            // Execute the loop body.
            const auto status = execute_block_in_place(ref_out, ctx_next, cand.body);
            if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_for })) {
              // Break out of the body as requested.
              break;
            }
            if(rocket::is_none_of(status, { Statement::status_next, Statement::status_continue_unspec, Statement::status_continue_for })) {
              // Forward anything unexpected to the caller.
              return status;
            }
            ctx_next.reset();
          }
        }
        else if(range_value.type() == Value::type_object) {
          const auto &object = range_value.as<D_object>();
          for(auto it = object.begin(); it != object.end(); ++it) {
            if(!ctx_next) {
              // Create a fresh context for this loop.
              ctx_next = allocate<Context>(ctx_inout, false);
            }
            const auto &key = it->first;
            // Initialize the per-loop key constant.
            ref_out = reference_constant(D_string(key));
            do_safe_set_named_reference(ctx_next, "`for each` key", cand.key_name, ref_out);
            ASTERIA_DEBUG_LOG("Created key constant with `for each` scope: name = ", cand.key_name);
            // Initialize the per-loop value reference.
            Reference_modifier::S_object_key refmod_c = { key };
            ref_out = indirect_reference_from(mapped_base, std::move(refmod_c));
            do_safe_set_named_reference(ctx_next, "`for each` reference", cand.mapped_name, ref_out);
            ASTERIA_DEBUG_LOG("Created value reference with `for each` scope: name = ", cand.mapped_name);
            // Execute the loop body.
            const auto status = execute_block_in_place(ref_out, ctx_next, cand.body);
            if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_for })) {
              // Break out of the body as requested.
              break;
            }
            if(rocket::is_none_of(status, { Statement::status_next, Statement::status_continue_unspec, Statement::status_continue_for })) {
              // Forward anything unexpected to the caller.
              return status;
            }
            ctx_next.reset();
          }
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("The `for each` statement does not accept a range of type `", get_type_name(range_value.type()), "`.");
        }
        return Statement::status_next;
      }
    case Statement::index_try:
      {
        const auto &cand = stmt.as<Statement::S_try>();
        try {
          // Execute the `try` body.
          // This is straightforward and hopefully zero-cost if no exception is thrown.
          const auto status = execute_block(ref_out, cand.body_try, ctx_inout);
          if(status != Statement::status_next) {
            // Forward anything unexpected to the caller.
            return status;
          }
        } catch(...) {
          // The exception variable shall not outlast the loop body.
          auto ctx_next = allocate<Context>(ctx_inout, false);
          // Identify the dynamic type of the exception.
          Bivector<String, Unsigned> btv;
          try {
            unpack_backtrace_and_rethrow(btv, std::current_exception());
          }
          catch(Exception &e) {
            ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", read_reference(e.get_reference()));
            // Copy the reference into the scope.
            ref_out = e.get_reference();
          }
          catch(std::exception &e) {
            ASTERIA_DEBUG_LOG("Caught `std::exception`: ", e.what());
            // Create a temporary string.
            ref_out = reference_temp_value(D_string(e.what()));
          }
          do_safe_set_named_reference(ctx_next, "exception", cand.except_name, ref_out);
          ASTERIA_DEBUG_LOG("Created exception reference with `catch` scope: name = ", cand.except_name);
          // Initialize the backtrace array.
          D_array backtrace;
          backtrace.reserve(btv.size());
          for(auto &pair : btv) {
            D_object elem;
            elem.set(String::shallow("file"), D_string(std::move(pair.first)));
            elem.set(String::shallow("line"), D_integer(pair.second));
            backtrace.emplace_back(std::move(elem));
          }
          ASTERIA_DEBUG_LOG("Exception backtrace:\n", backtrace);
          ref_out = reference_temp_value(std::move(backtrace));
          do_safe_set_named_reference(ctx_next, "backtrace array", String::shallow("__backtrace"), ref_out);
          // Execute the `catch` body.
          const auto status = execute_block(ref_out, cand.body_catch, ctx_next);
          if(status != Statement::status_next) {
            // Forward anything unexpected to the caller.
            return status;
          }
        }
        return Statement::status_next;
      }
    case Statement::index_break:
      {
        const auto &cand = stmt.as<Statement::S_break>();
        if(cand.target == Statement::target_scope_switch) {
          return Statement::status_break_switch;
        }
        else if(cand.target == Statement::target_scope_while) {
          return Statement::status_break_while;
        }
        else if(cand.target == Statement::target_scope_for) {
          return Statement::status_break_for;
        }
        return Statement::status_break_unspec;
      }
    case Statement::index_continue:
      {
        const auto &cand = stmt.as<Statement::S_continue>();
        if(cand.target == Statement::target_scope_while) {
          return Statement::status_continue_while;
        }
        else if(cand.target == Statement::target_scope_for) {
          return Statement::status_continue_for;
        }
        return Statement::status_continue_unspec;
      }
    case Statement::index_throw:
      {
        const auto &cand = stmt.as<Statement::S_throw>();
        // Evaluate the expression.
        ref_out = evaluate_expression(cand.expr, ctx_inout);
        ASTERIA_DEBUG_LOG("Throwing exception: ", read_reference(ref_out));
        throw Exception(ref_out);
      }
    case Statement::index_return:
      {
        const auto &cand = stmt.as<Statement::S_return>();
        // Evaluate the expression.
        ref_out = evaluate_expression(cand.expr, ctx_inout);
        return Statement::status_return;
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", stmt.index(), "` has been encountered.");
    }
  }
Statement::Status execute_block_in_place(Reference &ref_out, Spref<Context> ctx_inout, const Vector<Statement> &block)
  {
    if(ctx_inout->is_feigned() != false) {
      ASTERIA_THROW_RUNTIME_ERROR("`execute_block_in_place()` cannot be called on a feigned context.");
    }
    for(const auto &stmt : block) {
      const auto exrs = execute_statement_partial(ref_out, ctx_inout, stmt);
      if(exrs != Statement::status_next) {
        // Forward anything unexpected to the caller.
        return exrs;
      }
    }
    return Statement::status_next;
  }

Vector<Statement> bind_block(const Vector<Statement> &block, Spref<const Context> ctx)
  {
    if(block.empty()) {
      return { };
    }
    // Feign a context and bind the block onto it.
    // The context is destroyed before this function returns.
    const auto ctx_working = std::make_shared<Context>(ctx, true);
    return bind_block_in_place(ctx_working, block);
  }
Statement::Status execute_block(Reference &ref_out, const Vector<Statement> &block, Spref<const Context> ctx)
  {
    if(block.empty()) {
      return Statement::status_next;
    }
    // Create a context and execute the block in it.
    // The context is destroyed before this function returns.
    const auto ctx_working = std::make_shared<Context>(ctx, false);
    return execute_block_in_place(ref_out, ctx_working, block);
  }

}
