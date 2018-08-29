// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "statement.hpp"
#include "analytic_context.hpp"
#include "executive_context.hpp"
#include "variable.hpp"
#include "instantiated_function.hpp"
#include "exception.hpp"
#include "backtracer.hpp"
#include "utilities.hpp"

namespace Asteria {

Statement::~Statement()
  {
  }

Statement::Statement(Statement &&) noexcept
  = default;
Statement & Statement::operator=(Statement &&) noexcept
  = default;

namespace {

  void do_safe_set_named_reference(Abstract_context &ctx_inout, const char *desc, const String &name, Reference ref)
    {
      if(ctx_inout.is_name_reserved(name)) {
        ASTERIA_THROW_RUNTIME_ERROR("The name `", name, "` of this ", desc, " is reserved and cannot be used.");
      }
      if(name.empty()) {
        return;
      }
      ctx_inout.set_named_reference(name, std::move(ref));
    }

}

Statement Statement::bind(Analytic_context &ctx_inout) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case Statement::index_expression:
      {
        const auto &alt = this->m_stor.as<Statement::S_expression>();
        // Bind the expression recursively.
        auto expr_bnd = bind_expression(alt.expr, ctx_inout);
        Statement::S_expression cand_bnd = { std::move(expr_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_var_def:
      {
        const auto &alt = this->m_stor.as<Statement::S_var_def>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_inout, "variable", alt.name, { });
        // Bind the initializer recursively.
        auto init_bnd = bind_expression(alt.init, ctx_inout);
        Statement::S_var_def cand_bnd = { alt.name, alt.immutable, std::move(init_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_func_def:
      {
        const auto &alt = this->m_stor.as<Statement::S_func_def>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_inout, "function", alt.name, { });
        // Bind the function body recursively.
        Analytic_context ctx_next(&ctx_inout);
        ctx_next.initialize_for_function(alt.params);
        auto body_bnd = bind_block_in_place(ctx_next, alt.body);
        Statement::S_func_def cand_bnd = { alt.name, alt.params, alt.file, alt.line, std::move(body_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_if:
      {
        const auto &alt = this->m_stor.as<Statement::S_if>();
        // Bind the condition and both branches recursively.
        auto cond_bnd = bind_expression(alt.cond, ctx_inout);
        auto branch_true_bnd = bind_block(alt.branch_true, ctx_inout);
        auto branch_false_bnd = bind_block(alt.branch_false, ctx_inout);
        Statement::S_if cand_bnd = { std::move(cond_bnd), std::move(branch_true_bnd), std::move(branch_false_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_switch:
      {
        const auto &alt = this->m_stor.as<Statement::S_switch>();
        // Bind the control expression and all clauses recursively.
        auto ctrl_bnd = bind_expression(alt.ctrl, ctx_inout);
        // Note that all `switch` clauses share the same context.
        Analytic_context ctx_next(&ctx_inout);
        Bivector<Vector<Xpnode>, Vector<Statement>> clauses_bnd;
        clauses_bnd.reserve(alt.clauses.size());
        for(const auto &pair : alt.clauses) {
          auto first_bnd = bind_expression(pair.first, ctx_next);
          auto second_bnd = bind_block_in_place(ctx_next, pair.second);
          clauses_bnd.emplace_back(std::move(first_bnd), std::move(second_bnd));
        }
        Statement::S_switch cand_bnd = { std::move(ctrl_bnd), std::move(clauses_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_do_while:
      {
        const auto &alt = this->m_stor.as<Statement::S_do_while>();
        // Bind the loop body and condition recursively.
        auto body_bnd = bind_block(alt.body, ctx_inout);
        auto cond_bnd = bind_expression(alt.cond, ctx_inout);
        Statement::S_do_while cand_bnd = { std::move(body_bnd), std::move(cond_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_while:
      {
        const auto &alt = this->m_stor.as<Statement::S_while>();
        // Bind the condition and loop body recursively.
        auto cond_bnd = bind_expression(alt.cond, ctx_inout);
        auto body_bnd = bind_block(alt.body, ctx_inout);
        Statement::S_while cand_bnd = { std::move(cond_bnd), std::move(body_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_for:
      {
        const auto &alt = this->m_stor.as<Statement::S_for>();
        // If the initialization part is a variable definition, the variable defined shall not outlast the loop body.
        Analytic_context ctx_next(&ctx_inout);
        do_safe_set_named_reference(ctx_next, "`for` variable", alt.var_name, { });
        // Bind the loop variable initializer, condition, step expression and loop body recursively.
        auto var_init_bnd = bind_expression(alt.var_init, ctx_next);
        auto cond_bnd = bind_expression(alt.cond, ctx_next);
        auto step_bnd = bind_expression(alt.step, ctx_next);
        auto body_bnd = bind_block(alt.body, ctx_next);
        Statement::S_for cand_bnd = { alt.var_name, alt.var_immutable, std::move(var_init_bnd), std::move(cond_bnd), std::move(step_bnd), std::move(body_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_for_each:
      {
        const auto &alt = this->m_stor.as<Statement::S_for_each>();
        // The key and mapped variables shall not outlast the loop body.
        Analytic_context ctx_next(&ctx_inout);
        do_safe_set_named_reference(ctx_next, "`for each` key", alt.key_name, { });
        do_safe_set_named_reference(ctx_next, "`for each` reference", alt.mapped_name, { });
        // Bind the range initializer and loop body recursively.
        auto range_init_bnd = bind_expression(alt.range_init, ctx_next);
        auto body_bnd = bind_block(alt.body, ctx_next);
        Statement::S_for_each cand_bnd = { alt.key_name, alt.mapped_name, std::move(range_init_bnd), std::move(body_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_try:
      {
        const auto &alt = this->m_stor.as<Statement::S_try>();
        // The `try` branch needs no special treatement.
        auto body_try_bnd = bind_block(alt.body_try, ctx_inout);
        // The exception variable shall not outlast the `catch` body.
        Analytic_context ctx_next(&ctx_inout);
        do_safe_set_named_reference(ctx_next, "exception", alt.except_name, { });
        // Bind the `catch` branch recursively.
        auto body_catch_bnd = bind_block_in_place(ctx_next, alt.body_catch);
        Statement::S_try cand_bnd = { std::move(body_try_bnd), alt.except_name, std::move(body_catch_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_break:
      {
        const auto &alt = this->m_stor.as<Statement::S_break>();
        // Copy it as-is.
        Statement::S_break cand_bnd = { alt.target };
        return std::move(cand_bnd);
      }
    case Statement::index_continue:
      {
        const auto &alt = this->m_stor.as<Statement::S_continue>();
        // Copy it as-is.
        Statement::S_continue cand_bnd = { alt.target };
        return std::move(cand_bnd);
      }
    case Statement::index_throw:
      {
        const auto &alt = this->m_stor.as<Statement::S_throw>();
        // Bind the exception initializer recursively.
        auto expr_bnd = bind_expression(alt.expr, ctx_inout);
        Statement::S_throw cand_bnd = { std::move(expr_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_return:
      {
        const auto &alt = this->m_stor.as<Statement::S_return>();
        // Bind the result initializer recursively.
        auto expr_bnd = bind_expression(alt.expr, ctx_inout);
        Statement::S_return cand_bnd = { std::move(expr_bnd) };
        return std::move(cand_bnd);
      }
    case Statement::index_export:
      {
        const auto &alt = this->m_stor.as<Statement::S_export>();
        // Copy it as-is.
        Statement::S_export cand_bnd = { alt.name };
        return std::move(cand_bnd);
      }
    case Statement::index_import:
      {
        const auto &alt = this->m_stor.as<Statement::S_import>();
        // Copy it as-is.
        Statement::S_import cand_bnd = { alt.path };
        return std::move(cand_bnd);
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

Statement::Status Statement::execute(Reference &ref_out, Executive_context &ctx_inout) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case Statement::index_expression:
      {
        const auto &alt = this->m_stor.as<Statement::S_expression>();
        // Evaluate the expression.
        ref_out = evaluate_expression(alt.expr, ctx_inout);
        return Statement::status_next;
      }
    case Statement::index_var_def:
      {
        const auto &alt = this->m_stor.as<Statement::S_var_def>();
        // Create a dummy reference for further name lookups.
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_inout, "variable", alt.name, { });
        // Create a variable using the initializer.
        ref_out = evaluate_expression(alt.init, ctx_inout);
        auto value = ref_out.read();
        auto var = rocket::make_refcounted<Variable>(std::move(value), alt.immutable);
        // Reset the reference.
        Reference_root::S_variable ref_c = { std::move(var) };
        ref_out.set_root(std::move(ref_c));
        do_safe_set_named_reference(ctx_inout, "variable", alt.name, ref_out);
        ASTERIA_DEBUG_LOG("Created named variable: name = ", alt.name, ", immutable = ", alt.immutable);
        return Statement::status_next;
      }
    case Statement::index_func_def:
      {
        const auto &alt = this->m_stor.as<Statement::S_func_def>();
        // Create a dummy reference for further name lookups.
        // A function becomes visible before its definition, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_inout, "function", alt.name, { });
        // Bind the function body recursively.
        Analytic_context ctx_next(&ctx_inout);
        ctx_next.initialize_for_function(alt.params);
        auto body_bnd = bind_block_in_place(ctx_next, alt.body);
        auto func = rocket::make_refcounted<Instantiated_function>(alt.params, alt.file, alt.line, std::move(body_bnd));
        auto var = rocket::make_refcounted<Variable>(D_function(std::move(func)), true);
        // Reset the reference.
        Reference_root::S_variable ref_c = { std::move(var) };
        ref_out.set_root(std::move(ref_c));
        do_safe_set_named_reference(ctx_inout, "function", alt.name, ref_out);
        ASTERIA_DEBUG_LOG("Created named function: name = ", alt.name, ", file:line = ", alt.file, ':', alt.line);
        return Statement::status_next;
      }
    case Statement::index_if:
      {
        const auto &alt = this->m_stor.as<Statement::S_if>();
        // Evaluate the condition and pick a branch.
        ref_out = evaluate_expression(alt.cond, ctx_inout);
        const auto branch_taken = ref_out.read().test() ? std::ref(alt.branch_true) : std::ref(alt.branch_false);
        const auto status = execute_block(ref_out, branch_taken, ctx_inout);
        if(status != Statement::status_next) {
          return status;
        }
        return Statement::status_next;
      }
    case Statement::index_switch:
      {
        const auto &alt = this->m_stor.as<Statement::S_switch>();
        // Evaluate the control expression.
        ref_out = evaluate_expression(alt.ctrl, ctx_inout);
        const auto value_ctrl = ref_out.read();
        // Note that all `switch` clauses share the same context.
        // We will iterate from the first clause to the last one. If a `default` clause is encountered in the middle
        // and there is no match `case` clause, we will have to jump back into half of the scope. To simplify design,
        // a nested scope is created when a `default` clause is encountered. When jumping to the `default` scope, we
        // simply discard the new scope.
        Executive_context ctx_first(&ctx_inout);
        Executive_context ctx_second(&ctx_first);
        auto ctx_next = std::ref(ctx_first);
        // There is a 'match' at the end of the clause array initially.
        auto match = alt.clauses.end();
        // This is a pointer to where new references are created.
        auto ctx_test = ctx_next;
        for(auto it = alt.clauses.begin(); it != alt.clauses.end(); ++it) {
          if(it->first.empty()) {
            // This is a `default` clause.
            if(match != alt.clauses.end()) {
              ASTERIA_THROW_RUNTIME_ERROR("Multiple `default` clauses exist in the same `switch` statement.");
            }
            // From now on, all declarations go into the second context.
            match = it;
            ctx_test = std::ref(ctx_second);
          } else {
            // This is a `case` clause.
            ref_out = evaluate_expression(it->first, ctx_next);
            const auto value_comp = ref_out.read();
            if(value_ctrl.compare(value_comp) == Value::compare_equal) {
              // If this is a match, we resume from wherever `ctx_test` is pointing.
              match = it;
              ctx_next = ctx_test;
              break;
            }
          }
          // Create null references for declarations in the clause skipped.
          for(const auto &kstmt : it->second) {
            if(kstmt.m_stor.index() == Statement::index_var_def) {
              const auto &kcand = kstmt.m_stor.as<Statement::S_var_def>();
              do_safe_set_named_reference(ctx_test, "skipped variable", kcand.name, { });
              ASTERIA_DEBUG_LOG("Skipped named variable: name = ", kcand.name, ", immutable = ", kcand.immutable);
              continue;
            }
            if(kstmt.m_stor.index() == Statement::index_func_def) {
              const auto &kcand = kstmt.m_stor.as<Statement::S_func_def>();
              do_safe_set_named_reference(ctx_test, "skipped function", kcand.name, { });
              ASTERIA_DEBUG_LOG("Skipped named function: name = ", kcand.name, ", file:line = ", kcand.file, ':', kcand.line);
              continue;
            }
          }
        }
        // Iterate from the match clause to the end of the body, falling through clause boundaries if any.
        for(auto it = match; it != alt.clauses.end(); ++it) {
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
        const auto &alt = this->m_stor.as<Statement::S_do_while>();
        for(;;) {
          // Execute the loop body.
          const auto status = execute_block(ref_out, alt.body, ctx_inout);
          if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_while })) {
            // Break out of the body as requested.
            break;
          }
          if(rocket::is_none_of(status, { Statement::status_next, Statement::status_continue_unspec, Statement::status_continue_while })) {
            // Forward anything unexpected to the caller.
            return status;
          }
          // Check the loop condition.
          ref_out = evaluate_expression(alt.cond, ctx_inout);
          if(ref_out.read().test() == false) {
             break;
          }
        }
        return Statement::status_next;
      }
    case Statement::index_while:
      {
        const auto &alt = this->m_stor.as<Statement::S_while>();
        for(;;) {
          // Check the loop condition.
          ref_out = evaluate_expression(alt.cond, ctx_inout);
          if(ref_out.read().test() == false) {
            break;
          }
          // Execute the loop body.
          const auto status = execute_block(ref_out, alt.body, ctx_inout);
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
        const auto &alt = this->m_stor.as<Statement::S_for>();
        // If the initialization part is a variable definition, the variable defined shall not outlast the loop body.
        Executive_context ctx_next(&ctx_inout);
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_next, "`for` variable", alt.var_name, { });
        // Create a variable using the initializer.
        ref_out = evaluate_expression(alt.var_init, ctx_next);
        if(alt.var_name.empty() == false) {
          auto value = ref_out.read();
          auto var = rocket::make_refcounted<Variable>(std::move(value), alt.var_immutable);
          // Reset the reference.
          Reference_root::S_variable ref_c = { std::move(var) };
          do_safe_set_named_reference(ctx_next, "`for` variable", alt.var_name, std::move(ref_c));
          ASTERIA_DEBUG_LOG("Created named variable with `for` scope: name = ", alt.var_name, ", immutable = ", alt.var_immutable);
        }
        for(;;) {
          // Check the loop condition.
          if(alt.cond.empty() == false) {
            ref_out = evaluate_expression(alt.cond, ctx_next);
            if(ref_out.read().test() == false) {
              break;
            }
          }
          // Execute the loop body.
          const auto status = execute_block(ref_out, alt.body, ctx_next);
          if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_for })) {
            // Break out of the body as requested.
            break;
          }
          if(rocket::is_none_of(status, { Statement::status_next, Statement::status_continue_unspec, Statement::status_continue_for })) {
            // Forward anything unexpected to the caller.
            return status;
          }
          // Evaluate the loop step expression.
          evaluate_expression(alt.step, ctx_next);
        }
        return Statement::status_next;
      }
    case Statement::index_for_each:
      {
        const auto &alt = this->m_stor.as<Statement::S_for_each>();
        // The key and mapped variables shall not outlast the loop body.
        Executive_context ctx_for(&ctx_inout);
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_for, "`for each` key", alt.key_name, { });
        do_safe_set_named_reference(ctx_for, "`for each` reference", alt.mapped_name, { });
        // Calculate the range using the initializer.
        ref_out = evaluate_expression(alt.range_init, ctx_for);
        const auto mapped_base = ref_out;
        const auto range_value = ref_out.read();
        if(range_value.type() == Value::type_array) {
          const auto &array = range_value.check<D_array>();
          for(auto it = array.begin(); it != array.end(); ++it) {
            Executive_context ctx_next(&ctx_for);
            const auto index = static_cast<Signed>(it - array.begin());
            // Initialize the per-loop key constant.
            ref_out = Reference::make_constant(D_integer(index));
            do_safe_set_named_reference(ctx_next, "`for each` key", alt.key_name, ref_out);
            ASTERIA_DEBUG_LOG("Created key constant with `for each` scope: name = ", alt.key_name);
            // Initialize the per-loop value reference.
            Reference_modifier::S_array_index refmod_c = { index };
            ref_out = mapped_base;
            ref_out.push_modifier(std::move(refmod_c));
            do_safe_set_named_reference(ctx_next, "`for each` reference", alt.mapped_name, ref_out);
            ASTERIA_DEBUG_LOG("Created value reference with `for each` scope: name = ", alt.mapped_name);
            // Execute the loop body.
            const auto status = execute_block_in_place(ref_out, ctx_next, alt.body);
            if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_for })) {
              // Break out of the body as requested.
              break;
            }
            if(rocket::is_none_of(status, { Statement::status_next, Statement::status_continue_unspec, Statement::status_continue_for })) {
              // Forward anything unexpected to the caller.
              return status;
            }
          }
        } else if(range_value.type() == Value::type_object) {
          const auto &object = range_value.check<D_object>();
          for(auto it = object.begin(); it != object.end(); ++it) {
            Executive_context ctx_next(&ctx_for);
            const auto key = std::ref(it->first);
            // Initialize the per-loop key constant.
            ref_out = Reference::make_constant(D_string(key));
            do_safe_set_named_reference(ctx_next, "`for each` key", alt.key_name, ref_out);
            ASTERIA_DEBUG_LOG("Created key constant with `for each` scope: name = ", alt.key_name);
            // Initialize the per-loop value reference.
            Reference_modifier::S_object_key refmod_c = { key };
            ref_out = mapped_base;
            ref_out.push_modifier(std::move(refmod_c));
            do_safe_set_named_reference(ctx_next, "`for each` reference", alt.mapped_name, ref_out);
            ASTERIA_DEBUG_LOG("Created value reference with `for each` scope: name = ", alt.mapped_name);
            // Execute the loop body.
            const auto status = execute_block_in_place(ref_out, ctx_next, alt.body);
            if(rocket::is_any_of(status, { Statement::status_break_unspec, Statement::status_break_for })) {
              // Break out of the body as requested.
              break;
            }
            if(rocket::is_none_of(status, { Statement::status_next, Statement::status_continue_unspec, Statement::status_continue_for })) {
              // Forward anything unexpected to the caller.
              return status;
            }
          }
        } else {
          ASTERIA_THROW_RUNTIME_ERROR("The `for each` statement does not accept a range of type `", Value::get_type_name(range_value.type()), "`.");
        }
        return Statement::status_next;
      }
    case Statement::index_try:
      {
        const auto &alt = this->m_stor.as<Statement::S_try>();
        try {
          // Execute the `try` body.
          // This is straightforward and hopefully zero-cost if no exception is thrown.
          const auto status = execute_block(ref_out, alt.body_try, ctx_inout);
          if(status != Statement::status_next) {
            // Forward anything unexpected to the caller.
            return status;
          }
        } catch(...) {
          // The exception variable shall not outlast the loop body.
          Executive_context ctx_next(&ctx_inout);
          // Identify the dynamic type of the exception.
          Bivector<String, Unsigned> btv;
          try {
            Backtracer::unpack_backtrace_and_rethrow(btv, std::current_exception());
          } catch(Exception &e) {
            ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", e.get_reference().read());
            // Copy the reference into the scope.
            ref_out = e.get_reference().dematerialize();
          } catch(std::exception &e) {
            ASTERIA_DEBUG_LOG("Caught `std::exception`: ", e.what());
            // Create a temporary string.
            ref_out = Reference::make_temporary(D_string(e.what()));
          }
          do_safe_set_named_reference(ctx_next, "exception", alt.except_name, ref_out);
          ASTERIA_DEBUG_LOG("Created exception reference with `catch` scope: name = ", alt.except_name);
          // Initialize the backtrace array.
          D_array backtrace;
          backtrace.reserve(btv.size());
          for(auto &pair : btv) {
            D_object elem;
            elem.insert_or_assign(String::shallow("file"), D_string(std::move(pair.first)));
            elem.insert_or_assign(String::shallow("line"), D_integer(pair.second));
            backtrace.emplace_back(std::move(elem));
          }
          ASTERIA_DEBUG_LOG("Exception backtrace:\n", Value(backtrace));
          ref_out = Reference::make_temporary(std::move(backtrace));
          do_safe_set_named_reference(ctx_next, "backtrace array", String::shallow("__backtrace"), ref_out);
          // Execute the `catch` body.
          const auto status = execute_block(ref_out, alt.body_catch, ctx_next);
          if(status != Statement::status_next) {
            // Forward anything unexpected to the caller.
            return status;
          }
        }
        return Statement::status_next;
      }
    case Statement::index_break:
      {
        const auto &alt = this->m_stor.as<Statement::S_break>();
        if(alt.target == Statement::target_scope_switch) {
          return Statement::status_break_switch;
        }
        if(alt.target == Statement::target_scope_while) {
          return Statement::status_break_while;
        }
        if(alt.target == Statement::target_scope_for) {
          return Statement::status_break_for;
        }
        return Statement::status_break_unspec;
      }
    case Statement::index_continue:
      {
        const auto &alt = this->m_stor.as<Statement::S_continue>();
        if(alt.target == Statement::target_scope_while) {
          return Statement::status_continue_while;
        }
        if(alt.target == Statement::target_scope_for) {
          return Statement::status_continue_for;
        }
        return Statement::status_continue_unspec;
      }
    case Statement::index_throw:
      {
        const auto &alt = this->m_stor.as<Statement::S_throw>();
        // Evaluate the expression.
        ref_out = evaluate_expression(alt.expr, ctx_inout);
        ASTERIA_DEBUG_LOG("Throwing exception: ", ref_out.read());
        throw Exception(ref_out);
      }
    case Statement::index_return:
      {
        const auto &alt = this->m_stor.as<Statement::S_return>();
        // Evaluate the expression.
        ref_out = evaluate_expression(alt.expr, ctx_inout);
        return Statement::status_return;
      }
    case Statement::index_export:
      {
        const auto &alt = this->m_stor.as<Statement::S_export>();
        // TODO
        ASTERIA_TERMINATE("TODO : `export` has not been implemented yet.");
        (void)alt;
      }
    case Statement::index_import:
      {
        const auto &alt = this->m_stor.as<Statement::S_import>();
        // TODO
        ASTERIA_TERMINATE("TODO : `import` has not been implemented yet.");
        (void)alt;
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }




Vector<Statement> bind_block_in_place(Analytic_context &ctx_inout, const Vector<Statement> &block)
  {
    Vector<Statement> block_bnd;
    block_bnd.reserve(block.size());
    for(const auto &stmt : block) {
      auto cand_bnd = stmt.bind(ctx_inout);
      block_bnd.emplace_back(std::move(cand_bnd));
    }
    return block_bnd;
  }

Vector<Statement> bind_block(const Vector<Statement> &block, const Analytic_context &ctx)
  {
    Analytic_context ctx_next(&ctx);
    return bind_block_in_place(ctx_next, block);
  }

Statement::Status execute_block_in_place(Reference &ref_out, Executive_context &ctx_inout, const Vector<Statement> &block)
  {
    for(const auto &stmt : block) {
      const auto status = stmt.execute(ref_out, ctx_inout);
      if(status != Statement::status_next) {
        // Forward anything unexpected to the caller.
        return status;
      }
    }
    return Statement::status_next;
  }

Statement::Status execute_block(Reference &ref_out, const Vector<Statement> &block, const Executive_context &ctx)
  {
    Executive_context ctx_next(&ctx);
    return execute_block_in_place(ref_out, ctx_next, block);
  }

}
