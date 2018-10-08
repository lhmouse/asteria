// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "statement.hpp"
#include "xpnode.hpp"
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

namespace {

  void do_safe_set_named_reference(Abstract_context &ctx_io, const char *desc, const String &name, Reference ref)
    {
      if(name.empty()) {
        return;
      }
      if(ctx_io.is_name_reserved(name)) {
        ASTERIA_THROW_RUNTIME_ERROR("The name `", name, "` of this ", desc, " is reserved and cannot be used.");
      }
      ctx_io.set_named_reference(name, std::move(ref));
    }

}

void Statement::fly_over_in_place(Abstract_context &ctx_io) const
  {
    switch(Index(this->m_stor.index())) {
      case index_expr:
      case index_block: {
        return;
      }
      case index_var_def: {
        const auto &alt = this->m_stor.as<S_var_def>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "skipped variable", alt.name, { });
        return;
      }
      case index_func_def: {
        const auto &alt = this->m_stor.as<S_func_def>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "skipped function", alt.name, { });
        return;
      }
      case index_if:
      case index_switch:
      case index_do_while:
      case index_while:
      case index_for:
      case index_for_each:
      case index_try:
      case index_break:
      case index_continue:
      case index_throw:
      case index_return: {
        break;
      }
      default: {
        ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

Statement Statement::bind_in_place(Analytic_context &ctx_io, const Global_context *global_opt) const
  {
    switch(Index(this->m_stor.index())) {
      case index_expr: {
        const auto &alt = this->m_stor.as<S_expr>();
        // Bind the expression recursively.
        auto expr_bnd = alt.expr.bind(global_opt, ctx_io);
        Statement::S_expr alt_bnd = { std::move(expr_bnd) };
        return std::move(alt_bnd);
      }
      case index_block: {
        const auto &alt = this->m_stor.as<S_block>();
        // Bind the body recursively.
        auto body_bnd = alt.body.bind(global_opt, ctx_io);
        Statement::S_block alt_bnd = { std::move(body_bnd) };
        return std::move(alt_bnd);
      }
      case index_var_def: {
        const auto &alt = this->m_stor.as<S_var_def>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "variable", alt.name, { });
        // Bind the initializer recursively.
        auto init_bnd = alt.init.bind(global_opt, ctx_io);
        Statement::S_var_def alt_bnd = { alt.name, alt.immutable, std::move(init_bnd) };
        return std::move(alt_bnd);
      }
      case index_func_def: {
        const auto &alt = this->m_stor.as<S_func_def>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "function", alt.name, { });
        // Bind the function body recursively.
        Analytic_context ctx_next(&ctx_io);
        ctx_next.initialize_for_function(alt.params);
        auto body_bnd = alt.body.bind_in_place(ctx_next, global_opt);
        Statement::S_func_def alt_bnd = { alt.file, alt.line, alt.name, alt.params, std::move(body_bnd) };
        return std::move(alt_bnd);
      }
      case index_if: {
        const auto &alt = this->m_stor.as<S_if>();
        // Bind the condition and both branches recursively.
        auto cond_bnd = alt.cond.bind(global_opt, ctx_io);
        auto branch_true_bnd = alt.branch_true.bind(global_opt, ctx_io);
        auto branch_false_bnd = alt.branch_false.bind(global_opt, ctx_io);
        Statement::S_if alt_bnd = { std::move(cond_bnd), std::move(branch_true_bnd), std::move(branch_false_bnd) };
        return std::move(alt_bnd);
      }
      case index_switch: {
        const auto &alt = this->m_stor.as<S_switch>();
        // Bind the control expression and all clauses recursively.
        auto ctrl_bnd = alt.ctrl.bind(global_opt, ctx_io);
        // Note that all `switch` clauses share the same context.
        Analytic_context ctx_next(&ctx_io);
        Bivector<Expression, Block> clauses_bnd;
        clauses_bnd.reserve(alt.clauses.size());
        for(const auto &pair : alt.clauses) {
          auto first_bnd = pair.first.bind(global_opt, ctx_next);
          auto second_bnd = pair.second.bind_in_place(ctx_next, global_opt);
          clauses_bnd.emplace_back(std::move(first_bnd), std::move(second_bnd));
        }
        Statement::S_switch alt_bnd = { std::move(ctrl_bnd), std::move(clauses_bnd) };
        return std::move(alt_bnd);
      }
      case index_do_while: {
        const auto &alt = this->m_stor.as<S_do_while>();
        // Bind the loop body and condition recursively.
        auto body_bnd = alt.body.bind(global_opt, ctx_io);
        auto cond_bnd = alt.cond.bind(global_opt, ctx_io);
        Statement::S_do_while alt_bnd = { std::move(body_bnd), std::move(cond_bnd) };
        return std::move(alt_bnd);
      }
      case index_while: {
        const auto &alt = this->m_stor.as<S_while>();
        // Bind the condition and loop body recursively.
        auto cond_bnd = alt.cond.bind(global_opt, ctx_io);
        auto body_bnd = alt.body.bind(global_opt, ctx_io);
        Statement::S_while alt_bnd = { std::move(cond_bnd), std::move(body_bnd) };
        return std::move(alt_bnd);
      }
      case index_for: {
        const auto &alt = this->m_stor.as<S_for>();
        // If the initialization part is a variable definition, the variable defined shall not outlast the loop body.
        Analytic_context ctx_next(&ctx_io);
        // Bind the loop initializer, condition, step expression and loop body recursively.
        auto init_bnd = alt.init.bind(global_opt, ctx_next);
        auto cond_bnd = alt.cond.bind(global_opt, ctx_next);
        auto step_bnd = alt.step.bind(global_opt, ctx_next);
        auto body_bnd = alt.body.bind(global_opt, ctx_next);
        Statement::S_for alt_bnd = { std::move(init_bnd), std::move(cond_bnd), std::move(step_bnd), std::move(body_bnd) };
        return std::move(alt_bnd);
      }
      case index_for_each: {
        const auto &alt = this->m_stor.as<S_for_each>();
        // The key and mapped variables shall not outlast the loop body.
        Analytic_context ctx_next(&ctx_io);
        do_safe_set_named_reference(ctx_next, "`for each` key", alt.key_name, { });
        do_safe_set_named_reference(ctx_next, "`for each` reference", alt.mapped_name, { });
        // Bind the range initializer and loop body recursively.
        auto range_init_bnd = alt.range_init.bind(global_opt, ctx_next);
        auto body_bnd = alt.body.bind(global_opt, ctx_next);
        Statement::S_for_each alt_bnd = { alt.key_name, alt.mapped_name, std::move(range_init_bnd), std::move(body_bnd) };
        return std::move(alt_bnd);
      }
      case index_try: {
        const auto &alt = this->m_stor.as<S_try>();
        // The `try` branch needs no special treatement.
        auto body_try_bnd = alt.body_try.bind(global_opt, ctx_io);
        // The exception variable shall not outlast the `catch` body.
        Analytic_context ctx_next(&ctx_io);
        do_safe_set_named_reference(ctx_next, "exception", alt.except_name, { });
        // Bind the `catch` branch recursively.
        auto body_catch_bnd = alt.body_catch.bind_in_place(ctx_next, global_opt);
        Statement::S_try alt_bnd = { std::move(body_try_bnd), alt.except_name, std::move(body_catch_bnd) };
        return std::move(alt_bnd);
      }
      case index_break: {
        const auto &alt = this->m_stor.as<S_break>();
        // Copy it as-is.
        Statement::S_break alt_bnd = { alt.target };
        return std::move(alt_bnd);
      }
      case index_continue: {
        const auto &alt = this->m_stor.as<S_continue>();
        // Copy it as-is.
        Statement::S_continue alt_bnd = { alt.target };
        return std::move(alt_bnd);
      }
      case index_throw: {
        const auto &alt = this->m_stor.as<S_throw>();
        // Bind the exception initializer recursively.
        auto expr_bnd = alt.expr.bind(global_opt, ctx_io);
        Statement::S_throw alt_bnd = { std::move(expr_bnd) };
        return std::move(alt_bnd);
      }
      case index_return: {
        const auto &alt = this->m_stor.as<S_return>();
        // Bind the result initializer recursively.
        auto expr_bnd = alt.expr.bind(global_opt, ctx_io);
        Statement::S_return alt_bnd = { alt.by_ref, std::move(expr_bnd) };
        return std::move(alt_bnd);
      }
      default: {
        ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

namespace {

  Sptr<Variable> do_create_variable(Reference &ref_out)
    {
      auto var = rocket::make_refcounted<Variable>();
      Reference_root::S_variable ref_c = { var };
      ref_out = std::move(ref_c);
      return var;
    }

}

Block::Status Statement::execute_in_place(Reference &ref_out, Executive_context &ctx_io, Global_context *global_opt) const
  {
    switch(Index(this->m_stor.index())) {
      case index_expr: {
        const auto &alt = this->m_stor.as<S_expr>();
        // Evaluate the expression.
        ref_out = alt.expr.evaluate(global_opt, ctx_io);
        return Block::status_next;
      }
      case index_block: {
        const auto &alt = this->m_stor.as<S_block>();
        // Execute the body.
        return alt.body.execute(ref_out, global_opt, ctx_io);
      }
      case index_var_def: {
        const auto &alt = this->m_stor.as<S_var_def>();
        // Create a dummy reference for further name lookups.
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        const auto var = do_create_variable(ref_out);
        do_safe_set_named_reference(ctx_io, "variable", alt.name, ref_out);
        // Create a variable using the initializer.
        ref_out = alt.init.evaluate(global_opt, ctx_io);
        var->reset(ref_out.read(), alt.immutable);
        ASTERIA_DEBUG_LOG("Created named variable: name = ", alt.name, ", immutable = ", alt.immutable, ": ", ref_out.read());
        return Block::status_next;
      }
      case index_func_def: {
        const auto &alt = this->m_stor.as<S_func_def>();
        // Create a dummy reference for further name lookups.
        // A function becomes visible before its definition, where it is initialized to `null`.
        const auto var = do_create_variable(ref_out);
        do_safe_set_named_reference(ctx_io, "function", alt.name, ref_out);
        // Bind the function body recursively.
        Analytic_context ctx_next(&ctx_io);
        ctx_next.initialize_for_function(alt.params);
        auto body_bnd = alt.body.bind_in_place(ctx_next, global_opt);
        Instantiated_function func(alt.file, alt.line, alt.name, alt.params, std::move(body_bnd));
        var->reset(D_function(std::move(func)), true);
        ASTERIA_DEBUG_LOG("Created named function: name = ", alt.name, ", file:line = ", alt.file, ':', alt.line);
        return Block::status_next;
      }
      case index_if: {
        const auto &alt = this->m_stor.as<S_if>();
        // Evaluate the condition and pick a branch.
        ref_out = alt.cond.evaluate(global_opt, ctx_io);
        const auto status = (ref_out.read().test() ? alt.branch_true : alt.branch_false).execute(ref_out, global_opt, ctx_io);
        if(status != Block::status_next) {
          // Forward anything unexpected to the caller.
          return status;
        }
        return Block::status_next;
      }
      case index_switch: {
        const auto &alt = this->m_stor.as<S_switch>();
        // Evaluate the control expression.
        ref_out = alt.ctrl.evaluate(global_opt, ctx_io);
        const auto value_ctrl = ref_out.read();
        // Note that all `switch` clauses share the same context.
        // We will iterate from the first clause to the last one. If a `default` clause is encountered in the middle
        // and there is no match `case` clause, we will have to jump back into half of the scope. To simplify design,
        // a nested scope is created when a `default` clause is encountered. When jumping to the `default` scope, we
        // simply discard the new scope.
        Executive_context ctx_first(&ctx_io);
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
            ref_out = it->first.evaluate(global_opt, ctx_next);
            const auto value_comp = ref_out.read();
            if(value_ctrl.compare(value_comp) == Value::compare_equal) {
              // If this is a match, we resume from wherever `ctx_test` is pointing.
              match = it;
              ctx_next = ctx_test;
              break;
            }
          }
          // Create null references for declarations in the clause skipped.
          it->second.fly_over_in_place(ctx_test);
        }
        // Iterate from the match clause to the end of the body, falling through clause boundaries if any.
        for(auto it = match; it != alt.clauses.end(); ++it) {
          const auto status = it->second.execute_in_place(ref_out, ctx_next, global_opt);
          if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_switch })) {
            // Break out of the body as requested.
            break;
          }
          if(status != Block::status_next) {
            // Forward anything unexpected to the caller.
            return status;
          }
        }
        return Block::status_next;
      }
      case index_do_while: {
        const auto &alt = this->m_stor.as<S_do_while>();
        for(;;) {
          // Execute the loop body.
          Executive_context ctx_next(&ctx_io);
          const auto status = alt.body.execute_in_place(ref_out, ctx_next, global_opt);
          if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_while })) {
            // Break out of the body as requested.
            break;
          }
          if(rocket::is_none_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_while })) {
            // Forward anything unexpected to the caller.
            return status;
          }
          // Check the loop condition.
          // This differs from a `while` loop where the context for the loop body is destroyed before this check.
          ref_out = alt.cond.evaluate(global_opt, ctx_next);
          if(ref_out.read().test() == false) {
            break;
          }
        }
        return Block::status_next;
      }
      case index_while: {
        const auto &alt = this->m_stor.as<S_while>();
        for(;;) {
          // Check the loop condition.
          ref_out = alt.cond.evaluate(global_opt, ctx_io);
          if(ref_out.read().test() == false) {
            break;
          }
          // Execute the loop body.
          const auto status = alt.body.execute(ref_out, global_opt, ctx_io);
          if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_while })) {
            // Break out of the body as requested.
            break;
          }
          if(rocket::is_none_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_while })) {
            // Forward anything unexpected to the caller.
            return status;
          }
        }
        return Block::status_next;
      }
      case index_for: {
        const auto &alt = this->m_stor.as<S_for>();
        // If the initialization part is a variable definition, the variable defined shall not outlast the loop body.
        Executive_context ctx_next(&ctx_io);
        // Execute the initializer. The status is ignored.
        ASTERIA_DEBUG_LOG("Begin running `for` initialization...");
        alt.init.execute_in_place(ref_out, ctx_next, global_opt);
        ASTERIA_DEBUG_LOG("Done running `for` initialization: ", ref_out.read());
        for(;;) {
          // Check the loop condition.
          if(alt.cond.empty() == false) {
            ref_out = alt.cond.evaluate(global_opt, ctx_next);
            if(ref_out.read().test() == false) {
              break;
            }
          }
          // Execute the loop body.
          const auto status = alt.body.execute(ref_out, global_opt, ctx_next);
          if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_for })) {
            // Break out of the body as requested.
            break;
          }
          if(rocket::is_none_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_for })) {
            // Forward anything unexpected to the caller.
            return status;
          }
          // Evaluate the loop step expression.
          alt.step.evaluate(global_opt, ctx_next);
        }
        return Block::status_next;
      }
      case index_for_each: {
        const auto &alt = this->m_stor.as<S_for_each>();
        // The key and mapped variables shall not outlast the loop body.
        Executive_context ctx_for(&ctx_io);
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_for, "`for each` key", alt.key_name, { });
        do_safe_set_named_reference(ctx_for, "`for each` reference", alt.mapped_name, { });
        // Calculate the range using the initializer.
        auto mapped = alt.range_init.evaluate(global_opt, ctx_for);
        ref_out = mapped;
        const auto range_value = mapped.read();
        switch(rocket::weaken_enum(range_value.type())) {
          case Value::type_array: {
            const auto &array = range_value.check<D_array>();
            for(auto it = array.begin(); it != array.end(); ++it) {
              Executive_context ctx_next(&ctx_for);
              // Initialize the per-loop key constant.
              Reference_root::S_constant ref_c = { D_integer(it - array.begin()) };
              ref_out = std::move(ref_c);
              do_safe_set_named_reference(ctx_for, "`for each` key", alt.key_name, ref_out);
              ASTERIA_DEBUG_LOG("Created key constant with `for each` scope: name = ", alt.key_name, ": ", ref_out.read());
              // Initialize the per-loop value reference.
              Reference_modifier::S_array_index refmod_c = { it - array.begin() };
              mapped.zoom_in(std::move(refmod_c));
              do_safe_set_named_reference(ctx_for, "`for each` reference", alt.mapped_name, mapped);
              ASTERIA_DEBUG_LOG("Created value reference with `for each` scope: name = ", alt.mapped_name, ": ", mapped.read());
              mapped.zoom_out();
              // Execute the loop body.
              const auto status = alt.body.execute_in_place(ref_out, ctx_next, global_opt);
              if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_for })) {
                // Break out of the body as requested.
                break;
              }
              if(rocket::is_none_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_for })) {
                // Forward anything unexpected to the caller.
                return status;
              }
            }
            break;
          }
          case Value::type_object: {
            const auto &object = range_value.check<D_object>();
            for(auto it = object.begin(); it != object.end(); ++it) {
              Executive_context ctx_next(&ctx_for);
              // Initialize the per-loop key constant.
              Reference_root::S_constant ref_c = { D_string(it->first) };
              ref_out = std::move(ref_c);
              do_safe_set_named_reference(ctx_for, "`for each` key", alt.key_name, ref_out);
              ASTERIA_DEBUG_LOG("Created key constant with `for each` scope: name = ", alt.key_name, ": ", ref_out.read());
              // Initialize the per-loop value reference.
              Reference_modifier::S_object_key refmod_c = { it->first };
              mapped.zoom_in(std::move(refmod_c));
              do_safe_set_named_reference(ctx_for, "`for each` reference", alt.mapped_name, mapped);
              ASTERIA_DEBUG_LOG("Created value reference with `for each` scope: name = ", alt.mapped_name, ": ", mapped.read());
              mapped.zoom_out();
              // Execute the loop body.
              const auto status = alt.body.execute_in_place(ref_out, ctx_next, global_opt);
              if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_for })) {
                // Break out of the body as requested.
                break;
              }
              if(rocket::is_none_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_for })) {
                // Forward anything unexpected to the caller.
                return status;
              }
            }
            break;
          }
          default: {
            ASTERIA_THROW_RUNTIME_ERROR("The `for each` statement does not accept a range of type `", Value::get_type_name(range_value.type()), "`.");
          }
        }
        return Block::status_next;
      }
      case index_try: {
        const auto &alt = this->m_stor.as<S_try>();
        try {
          // Execute the `try` body.
          // This is straightforward and hopefully zero-cost if no exception is thrown.
          const auto status = alt.body_try.execute(ref_out, global_opt, ctx_io);
          if(status != Block::status_next) {
            // Forward anything unexpected to the caller.
            return status;
          }
        } catch(...) {
          // The exception variable shall not outlast the loop body.
          Executive_context ctx_next(&ctx_io);
          // Identify the dynamic type of the exception.
          Vector<Backtracer> btv;
          try {
            Backtracer::unpack_and_rethrow(btv, std::current_exception());
          } catch(Exception &e) {
            ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", e.get_value());
            // Copy the value.
            Reference_root::S_temporary ref_c = { e.get_value() };
            ref_out = std::move(ref_c);
          } catch(std::exception &e) {
            ASTERIA_DEBUG_LOG("Caught `std::exception`: ", e.what());
            // Create a temporary string.
            Reference_root::S_temporary ref_c = { D_string(e.what()) };
            ref_out = std::move(ref_c);
          }
          do_safe_set_named_reference(ctx_next, "exception", alt.except_name, ref_out);
          ASTERIA_DEBUG_LOG("Created exception reference with `catch` scope: name = ", alt.except_name, ": ", ref_out.read());
          // Initialize the backtrace array.
          D_array backtrace;
          backtrace.reserve(btv.size());
          for(auto it = btv.rbegin(); it != btv.rend(); ++it) {
            D_object elem;
            elem.insert_or_assign(String::shallow("file"), D_string(it->file()));
            elem.insert_or_assign(String::shallow("line"), D_integer(it->line()));
            elem.insert_or_assign(String::shallow("func"), D_string(it->func()));
            backtrace.emplace_back(std::move(elem));
          }
          ASTERIA_DEBUG_LOG("Exception backtrace:\n", Value(backtrace));
          Reference_root::S_temporary ref_c = { std::move(backtrace) };
          ref_out = std::move(ref_c);
          ctx_next.set_named_reference(String::shallow("__backtrace"), ref_out);
          // Execute the `catch` body.
          const auto status = alt.body_catch.execute(ref_out, global_opt, ctx_next);
          if(status != Block::status_next) {
            // Forward anything unexpected to the caller.
            return status;
          }
        }
        return Block::status_next;
      }
      case index_break: {
        const auto &alt = this->m_stor.as<S_break>();
        switch(rocket::weaken_enum(alt.target)) {
          case Statement::target_switch: {
            return Block::status_break_switch;
          }
          case Statement::target_while: {
            return Block::status_break_while;
          }
          case Statement::target_for: {
            return Block::status_break_for;
          }
        }
        return Block::status_break_unspec;
      }
      case index_continue: {
        const auto &alt = this->m_stor.as<S_continue>();
        switch(rocket::weaken_enum(alt.target)) {
          case Statement::target_switch: {
            ASTERIA_TERMINATE("`target_switch` is not allowed to follow `continue`.");
          }
          case Statement::target_while: {
            return Block::status_continue_while;
          }
          case Statement::target_for: {
            return Block::status_continue_for;
          }
        }
        return Block::status_continue_unspec;
      }
      case index_throw: {
        const auto &alt = this->m_stor.as<S_throw>();
        // Evaluate the expression.
        ref_out = alt.expr.evaluate(global_opt, ctx_io);
        auto value = ref_out.read();
        ASTERIA_DEBUG_LOG("Throwing exception: ", value);
        throw Exception(std::move(value));
      }
      case index_return: {
        const auto &alt = this->m_stor.as<S_return>();
        // Evaluate the expression.
        ref_out = alt.expr.evaluate(global_opt, ctx_io);
        // If `by_ref` is `false`, replace it with a temporary value.
        if(alt.by_ref == false) {
          ref_out.convert_to_temporary();
        }
        return Block::status_return;
      }
      default: {
        ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

}
