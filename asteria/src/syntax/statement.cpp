// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "statement.hpp"
#include "xpnode.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/function_analytic_context.hpp"
#include "../runtime/variable.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../runtime/traceable_exception.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    template<typename XnameT, typename XrefT> void do_safe_set_named_reference(Abstract_Context &ctx_io, const char *desc, const XnameT &name, XrefT &&xref)
      {
        if(name.empty()) {
          return;
        }
        if(name.rdstr().starts_with("__")) {
          ASTERIA_THROW_RUNTIME_ERROR("The name `", name, "` of this ", desc, " is reserved and cannot be used.");
        }
        ctx_io.open_named_reference(name) = std::forward<XrefT>(xref);
      }

    }

void Statement::fly_over_in_place(Abstract_Context &ctx_io) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_expression:
    case index_block:
      {
        return;
      }
    case index_variable:
      {
        const auto &alt = this->m_stor.as<S_variable>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "skipped variable", alt.name, Reference_Root::S_undefined());
        return;
      }
    case index_function:
      {
        const auto &alt = this->m_stor.as<S_function>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "skipped function", alt.name, Reference_Root::S_undefined());
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
    case index_return:
    case index_assert:
      {
        break;
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

void Statement::bind_in_place(Cow_Vector<Statement> &stmts_out, Analytic_Context &ctx_io, const Global_Context &global) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_expression:
      {
        const auto &alt = this->m_stor.as<S_expression>();
        // Bind the expression recursively.
        auto expr_bnd = alt.expr.bind(global, ctx_io);
        Statement::S_expression alt_bnd = { rocket::move(expr_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_block:
      {
        const auto &alt = this->m_stor.as<S_block>();
        // Bind the body recursively.
        auto body_bnd = alt.body.bind(global, ctx_io);
        Statement::S_block alt_bnd = { rocket::move(body_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_variable:
      {
        const auto &alt = this->m_stor.as<S_variable>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "variable", alt.name, Reference_Root::S_undefined());
        // Bind the initializer recursively.
        auto init_bnd = alt.init.bind(global, ctx_io);
        Statement::S_variable alt_bnd = { alt.sloc, alt.name, alt.immutable, rocket::move(init_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_function:
      {
        const auto &alt = this->m_stor.as<S_function>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "function", alt.name, Reference_Root::S_undefined());
        // Bind the function body recursively.
        Function_Analytic_Context ctx_next(&ctx_io, alt.params);
        auto body_bnd = alt.body.bind_in_place(ctx_next, global);
        Statement::S_function alt_bnd = { alt.sloc, alt.name, alt.params, rocket::move(body_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_if:
      {
        const auto &alt = this->m_stor.as<S_if>();
        // Bind the condition and both branches recursively.
        auto cond_bnd = alt.cond.bind(global, ctx_io);
        auto branch_true_bnd = alt.branch_true.bind(global, ctx_io);
        auto branch_false_bnd = alt.branch_false.bind(global, ctx_io);
        Statement::S_if alt_bnd = { alt.neg, rocket::move(cond_bnd), rocket::move(branch_true_bnd), rocket::move(branch_false_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_switch:
      {
        const auto &alt = this->m_stor.as<S_switch>();
        // Bind the control expression and all clauses recursively.
        auto ctrl_bnd = alt.ctrl.bind(global, ctx_io);
        // Note that all `switch` clauses share the same context.
        Analytic_Context ctx_next(&ctx_io);
        Cow_Vector<std::pair<Expression, Block>> clauses_bnd;
        clauses_bnd.reserve(alt.clauses.size());
        for(const auto &pair : alt.clauses) {
          auto first_bnd = pair.first.bind(global, ctx_next);
          auto second_bnd = pair.second.bind_in_place(ctx_next, global);
          clauses_bnd.emplace_back(rocket::move(first_bnd), rocket::move(second_bnd));
        }
        Statement::S_switch alt_bnd = { rocket::move(ctrl_bnd), rocket::move(clauses_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_do_while:
      {
        const auto &alt = this->m_stor.as<S_do_while>();
        // Bind the loop body and condition recursively.
        auto body_bnd = alt.body.bind(global, ctx_io);
        auto cond_bnd = alt.cond.bind(global, ctx_io);
        Statement::S_do_while alt_bnd = { rocket::move(body_bnd), alt.neg, rocket::move(cond_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_while:
      {
        const auto &alt = this->m_stor.as<S_while>();
        // Bind the condition and loop body recursively.
        auto cond_bnd = alt.cond.bind(global, ctx_io);
        auto body_bnd = alt.body.bind(global, ctx_io);
        Statement::S_while alt_bnd = { alt.neg, rocket::move(cond_bnd), rocket::move(body_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_for:
      {
        const auto &alt = this->m_stor.as<S_for>();
        // If the initialization part is a variable definition, the variable defined shall not outlast the loop body.
        Analytic_Context ctx_next(&ctx_io);
        // Bind the loop initializer, condition, step expression and loop body recursively.
        auto init_bnd = alt.init.bind_in_place(ctx_next, global);
        auto cond_bnd = alt.cond.bind(global, ctx_next);
        auto step_bnd = alt.step.bind(global, ctx_next);
        auto body_bnd = alt.body.bind(global, ctx_next);
        Statement::S_for alt_bnd = { rocket::move(init_bnd), rocket::move(cond_bnd), rocket::move(step_bnd), rocket::move(body_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_for_each:
      {
        const auto &alt = this->m_stor.as<S_for_each>();
        // The key and mapped variables shall not outlast the loop body.
        Analytic_Context ctx_next(&ctx_io);
        do_safe_set_named_reference(ctx_next, "`for each` key", alt.key_name, Reference_Root::S_undefined());
        do_safe_set_named_reference(ctx_next, "`for each` reference", alt.mapped_name, Reference_Root::S_undefined());
        // Bind the range initializer and loop body recursively.
        auto init_bnd = alt.init.bind(global, ctx_next);
        auto body_bnd = alt.body.bind(global, ctx_next);
        Statement::S_for_each alt_bnd = { alt.key_name, alt.mapped_name, rocket::move(init_bnd), rocket::move(body_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_try:
      {
        const auto &alt = this->m_stor.as<S_try>();
        // The `try` branch needs no special treatement.
        auto body_try_bnd = alt.body_try.bind(global, ctx_io);
        // The exception variable shall not outlast the `catch` body.
        Analytic_Context ctx_next(&ctx_io);
        do_safe_set_named_reference(ctx_next, "exception", alt.except_name, Reference_Root::S_undefined());
        // Bind the `catch` branch recursively.
        auto body_catch_bnd = alt.body_catch.bind_in_place(ctx_next, global);
        Statement::S_try alt_bnd = { rocket::move(body_try_bnd), alt.sloc, alt.except_name, rocket::move(body_catch_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_break:
      {
        const auto &alt = this->m_stor.as<S_break>();
        // Copy it as-is.
        Statement::S_break alt_bnd = { alt.target };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_continue:
      {
        const auto &alt = this->m_stor.as<S_continue>();
        // Copy it as-is.
        Statement::S_continue alt_bnd = { alt.target };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_throw:
      {
        const auto &alt = this->m_stor.as<S_throw>();
        // Bind the exception initializer recursively.
        auto expr_bnd = alt.expr.bind(global, ctx_io);
        Statement::S_throw alt_bnd = { alt.sloc, rocket::move(expr_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_return:
      {
        const auto &alt = this->m_stor.as<S_return>();
        // Bind the result initializer recursively.
        auto expr_bnd = alt.expr.bind(global, ctx_io);
        Statement::S_return alt_bnd = { alt.by_ref, rocket::move(expr_bnd) };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    case index_assert:
      {
        const auto &alt = this->m_stor.as<S_assert>();
        // Bind the condition recursively.
        auto expr_bnd = alt.expr.bind(global, ctx_io);
        Statement::S_assert alt_bnd = { rocket::move(expr_bnd), alt.msg };
        stmts_out.emplace_back(rocket::move(alt_bnd));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

    namespace {

    Block::Status do_execute_expression(const Statement::S_expression &alt,
                                        Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // Evaluate the expression.
        alt.expr.evaluate(ref_out, func, global, ctx_io);
        return Block::status_next;
      }

    Block::Status do_execute_block(const Statement::S_block &alt,
                                   Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // Execute the body.
        return alt.body.execute(ref_out, func, global, ctx_io);
      }

    Block::Status do_execute_variable(const Statement::S_variable &alt,
                                      Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // Create a dummy reference for further name lookups.
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        auto var = global.create_variable();
        Reference_Root::S_variable ref_c = { var };
        do_safe_set_named_reference(ctx_io, "variable", alt.name, rocket::move(ref_c));
        // Create a variable using the initializer.
        alt.init.evaluate(ref_out, func, global, ctx_io);
        auto value = ref_out.read();
        ASTERIA_DEBUG_LOG("Creating named variable: ", (alt.immutable ? "const " : "var "), alt.name, " = ", value);
        var->reset(rocket::move(value), alt.immutable);
        return Block::status_next;
      }

    Block::Status do_execute_function(const Statement::S_function &alt,
                                      Reference & /*ref_out*/, Executive_Context &ctx_io, const Cow_String & /*func*/, const Global_Context &global)
      {
        // Create a dummy reference for further name lookups.
        // A function becomes visible before its definition, where it is initialized to `null`.
        auto var = global.create_variable();
        Reference_Root::S_variable ref_c = { var };
        do_safe_set_named_reference(ctx_io, "function", alt.name, rocket::move(ref_c));
        // Instantiate the function here.
        rocket::insertable_ostream nos;
        nos << alt.name << "("
            << rocket::ostream_implode(alt.params.begin(), alt.params.size(), ", ")
            <<")";
        auto closure = alt.body.instantiate_function(alt.sloc, nos.extract_string(), alt.params, global, ctx_io);
        ASTERIA_DEBUG_LOG("Creating named function: ", closure.get());
        var->reset(D_function(rocket::move(closure)), true);
        return Block::status_next;
      }

    Block::Status do_execute_if(const Statement::S_if &alt,
                                Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // Evaluate the condition and pick a branch.
        alt.cond.evaluate(ref_out, func, global, ctx_io);
        auto status = (ref_out.read().test() != alt.neg) ? alt.branch_true.execute(ref_out, func, global, ctx_io)
                                                               : alt.branch_false.execute(ref_out, func, global, ctx_io);
        return status;
      }

    Block::Status do_execute_switch(const Statement::S_switch &alt,
                                    Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // Evaluate the control expression.
        alt.ctrl.evaluate(ref_out, func, global, ctx_io);
        auto value_ctrl = ref_out.read();
        // Note that all `switch` clauses share the same context.
        // We will iterate from the first clause to the last one. If a `default` clause is encountered in the middle
        // and there is no match `case` clause, we will have to jump back into half of the scope. To simplify design,
        // a nested scope is created when a `default` clause is encountered. When jumping to the `default` scope, we
        // simply discard the new scope.
        Executive_Context ctx_first(&ctx_io);
        Executive_Context ctx_second(&ctx_first);
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
            it->first.evaluate(ref_out, func, global, ctx_next);
            auto value_comp = ref_out.read();
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
          auto status = it->second.execute_in_place(ref_out, ctx_next, func, global);
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

    Block::Status do_execute_do_while(const Statement::S_do_while &alt,
                                      Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        for(;;) {
          // Execute the loop body.
          auto status = alt.body.execute(ref_out, func, global, ctx_io);
          if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_while })) {
            // Break out of the body as requested.
            break;
          }
          if(!rocket::is_any_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_while })) {
            // Forward anything unexpected to the caller.
            return status;
          }
          // Check the loop condition.
          alt.cond.evaluate(ref_out, func, global, ctx_io);
          if(ref_out.read().test() == alt.neg) {
            break;
          }
        }
        return Block::status_next;
      }

    Block::Status do_execute_while(const Statement::S_while &alt,
                                   Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        for(;;) {
          // Check the loop condition.
          alt.cond.evaluate(ref_out, func, global, ctx_io);
          if(ref_out.read().test() == alt.neg) {
            break;
          }
          // Execute the loop body.
          auto status = alt.body.execute(ref_out, func, global, ctx_io);
          if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_while })) {
            // Break out of the body as requested.
            break;
          }
          if(!rocket::is_any_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_while })) {
            // Forward anything unexpected to the caller.
            return status;
          }
        }
        return Block::status_next;
      }

    Block::Status do_execute_for(const Statement::S_for &alt,
                                 Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // If the initialization part is a variable definition, the variable defined shall not outlast the loop body.
        Executive_Context ctx_next(&ctx_io);
        // Execute the initializer. The status is ignored.
        ASTERIA_DEBUG_LOG("Begin running `for` initialization...");
        alt.init.execute_in_place(ref_out, ctx_next, func, global);
        ASTERIA_DEBUG_LOG("Done running `for` initialization: ", ref_out.read());
        for(;;) {
          // Check the loop condition.
          if(!alt.cond.empty()) {
            alt.cond.evaluate(ref_out, func, global, ctx_next);
            if(!ref_out.read().test()) {
              break;
            }
          }
          // Execute the loop body.
          auto status = alt.body.execute(ref_out, func, global, ctx_next);
          if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_for })) {
            // Break out of the body as requested.
            break;
          }
          if(!rocket::is_any_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_for })) {
            // Forward anything unexpected to the caller.
            return status;
          }
          // Evaluate the loop step expression.
          alt.step.evaluate(ref_out, func, global, ctx_next);
        }
        return Block::status_next;
      }

    Block::Status do_execute_for_each(const Statement::S_for_each &alt,
                                      Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // The key and mapped variables shall not outlast the loop body.
        Executive_Context ctx_for(&ctx_io);
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        do_safe_set_named_reference(ctx_for, "`for each` key", alt.key_name, Reference_Root::S_undefined());
        do_safe_set_named_reference(ctx_for, "`for each` reference", alt.mapped_name, Reference_Root::S_undefined());
        // Calculate the range using the initializer.
        Reference mapped;
        alt.init.evaluate(mapped, func, global, ctx_for);
        auto range_value = mapped.read();
        switch(rocket::weaken_enum(range_value.type())) {
        case type_array:
          {
            const auto &array = range_value.check<D_array>();
            for(auto it = array.begin(); it != array.end(); ++it) {
              Executive_Context ctx_next(&ctx_for);
              // Initialize the per-loop key constant.
              auto key = D_integer(it - array.begin());
              ASTERIA_DEBUG_LOG("Creating key constant with `for each` scope: name = ", alt.key_name, ": ", key);
              Reference_Root::S_constant ref_c = { rocket::move(key) };
              do_safe_set_named_reference(ctx_for, "`for each` key", alt.key_name, rocket::move(ref_c));
              // Initialize the per-loop value reference.
              Reference_Modifier::S_array_index refmod_c = { it - array.begin() };
              mapped.zoom_in(rocket::move(refmod_c));
              do_safe_set_named_reference(ctx_for, "`for each` reference", alt.mapped_name, mapped);
              ASTERIA_DEBUG_LOG("Created value reference with `for each` scope: name = ", alt.mapped_name, ": ", mapped.read());
              mapped.zoom_out();
              // Execute the loop body.
              auto status = alt.body.execute_in_place(ref_out, ctx_next, func, global);
              if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_for })) {
                // Break out of the body as requested.
                break;
              }
              if(!rocket::is_any_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_for })) {
                // Forward anything unexpected to the caller.
                return status;
              }
            }
            break;
          }
        case type_object:
          {
            const auto &object = range_value.check<D_object>();
            for(auto it = object.begin(); it != object.end(); ++it) {
              Executive_Context ctx_next(&ctx_for);
              // Initialize the per-loop key constant.
              auto key = D_string(it->first);
              ASTERIA_DEBUG_LOG("Creating key constant with `for each` scope: name = ", alt.key_name, ": ", key);
              Reference_Root::S_constant ref_c = { rocket::move(key) };
              do_safe_set_named_reference(ctx_for, "`for each` key", alt.key_name, rocket::move(ref_c));
              // Initialize the per-loop value reference.
              Reference_Modifier::S_object_key refmod_c = { it->first };
              mapped.zoom_in(rocket::move(refmod_c));
              do_safe_set_named_reference(ctx_for, "`for each` reference", alt.mapped_name, mapped);
              ASTERIA_DEBUG_LOG("Created value reference with `for each` scope: name = ", alt.mapped_name, ": ", mapped.read());
              mapped.zoom_out();
              // Execute the loop body.
              auto status = alt.body.execute_in_place(ref_out, ctx_next, func, global);
              if(rocket::is_any_of(status, { Block::status_break_unspec, Block::status_break_for })) {
                // Break out of the body as requested.
                break;
              }
              if(!rocket::is_any_of(status, { Block::status_next, Block::status_continue_unspec, Block::status_continue_for })) {
                // Forward anything unexpected to the caller.
                return status;
              }
            }
            break;
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("The `for each` statement does not accept a range of type `", Value::get_type_name(range_value.type()), "`.");
        }
        return Block::status_next;
      }

    Block::Status do_execute_try(const Statement::S_try &alt,
                                 Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        auto status = Block::status_next;
        try {
          // Execute the `try` body.
          // This is straightforward and hopefully zero-cost if no exception is thrown.
          status = alt.body_try.execute(ref_out, func, global, ctx_io);
        } catch(std::exception &stdex) {
          // The exception variable shall not outlast the `catch` body.
          Executive_Context ctx_next(&ctx_io);
          // Translate the exception.
          auto traceable = trace_exception(rocket::move(stdex));
          ASTERIA_DEBUG_LOG("Creating exception reference with `catch` scope: name = ", alt.except_name, ": ", traceable.get_value());
          Reference_Root::S_temporary eref_c = { traceable.get_value() };
          do_safe_set_named_reference(ctx_next, "exception object", alt.except_name, rocket::move(eref_c));
          // Backtrace frames.
          D_array backtrace;
          for(std::size_t i = 0; i < traceable.get_frame_count(); ++i) {
            const auto &frame = traceable.get_frame(i);
            D_object elem;
            // Append frame information.
            elem.try_emplace(rocket::sref("file"), D_string(frame.source_file()));
            elem.try_emplace(rocket::sref("line"), D_integer(frame.source_line()));
            elem.try_emplace(rocket::sref("func"), D_string(frame.function_signature()));
            backtrace.emplace_back(rocket::move(elem));
          }
          ASTERIA_DEBUG_LOG("Exception backtrace:\n", Value(backtrace));
          Reference_Root::S_temporary btref_c = { rocket::move(backtrace) };
          ctx_next.open_named_reference(rocket::sref("__backtrace")) = rocket::move(btref_c);
          // Execute the `catch` body.
          status = alt.body_catch.execute(ref_out, func, global, ctx_next);
        }
        return status;
      }

    Block::Status do_execute_throw(const Statement::S_throw &alt,
                                   Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // Evaluate the expression.
        alt.expr.evaluate(ref_out, func, global, ctx_io);
        // Throw an exception containing the value.
        auto value = ref_out.read();
        ASTERIA_DEBUG_LOG("Throwing `Traceable_Exception` at \'", alt.sloc, "\' inside `", func, "`: ", value);
        throw Traceable_Exception(rocket::move(value), alt.sloc, func);
      }

    Block::Status do_execute_return(const Statement::S_return &alt,
                                    Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // Evaluate the expression.
        alt.expr.evaluate(ref_out, func, global, ctx_io);
        // If the result refers a variable and the statement will pass it by value, replace it with a temporary value.
        if(!alt.by_ref && !ref_out.is_temporary()) {
          Reference_Root::S_temporary ref_c = { ref_out.read() };
          ref_out = rocket::move(ref_c);
        }
        return Block::status_return;
      }

    Block::Status do_execute_assert(const Statement::S_assert &alt,
                                    Reference &ref_out, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global)
      {
        // Evaluate the expression.
        alt.expr.evaluate(ref_out, func, global, ctx_io);
        // If the condition yields `false`, throw an exception.
        auto value = ref_out.read();
        if(ROCKET_UNEXPECT(!value.test())) {
          rocket::insertable_ostream mos;
          mos << "Assertion failed";
          if(alt.msg.empty()) {
            mos << "!";
          } else {
            mos << ": " << alt.msg;
          }
          ASTERIA_DEBUG_LOG("Throwing `Runtime_Error`: ", value);
          throw_runtime_error(ROCKET_FUNCSIG, mos.extract_string());
        }
        return Block::status_next;
      }

    // Why do we have to duplicate these parameters so many times?
    // BECAUSE C++ IS STUPID, PERIOD.
    template<typename AltT,
             Block::Status (&funcT)(const AltT &,
                                    Reference &, Executive_Context &, const Cow_String &, const Global_Context &)
             > Block::Compiled_Instruction do_bind(const AltT &alt)
      {
        return rocket::bind_front(
          [](const void *qalt,
             const std::tuple<Reference &, Executive_Context &, const Cow_String &, const Global_Context &> &params)
            {
              return funcT(*static_cast<const AltT *>(qalt),
                           std::get<0>(params), std::get<1>(params), std::get<2>(params), std::get<3>(params));
            },
          std::addressof(alt));
      }

    Block::Compiled_Instruction do_bind_constant(Block::Status status)
      {
        return rocket::bind_front(
          [](const void *value,
             const std::tuple<Reference &, Executive_Context &, const Cow_String &, const Global_Context &> & /*params*/)
            {
              return static_cast<Block::Status>(reinterpret_cast<std::intptr_t>(value));
            },
          reinterpret_cast<void *>(static_cast<std::intptr_t>(status)));
      }

    }

void Statement::compile(Cow_Vector<Block::Compiled_Instruction> &cinsts_out) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_expression:
      {
        const auto &alt = this->m_stor.as<S_expression>();
        cinsts_out.emplace_back(do_bind<S_expression, do_execute_expression>(alt));
        return;
      }
    case index_block:
      {
        const auto &alt = this->m_stor.as<S_block>();
        cinsts_out.emplace_back(do_bind<S_block, do_execute_block>(alt));
        return;
      }
    case index_variable:
      {
        const auto &alt = this->m_stor.as<S_variable>();
        cinsts_out.emplace_back(do_bind<S_variable, do_execute_variable>(alt));
        return;
      }
    case index_function:
      {
        const auto &alt = this->m_stor.as<S_function>();
        cinsts_out.emplace_back(do_bind<S_function, do_execute_function>(alt));
        return;
      }
    case index_if:
      {
        const auto &alt = this->m_stor.as<S_if>();
        cinsts_out.emplace_back(do_bind<S_if, do_execute_if>(alt));
        return;
      }
    case index_switch:
      {
        const auto &alt = this->m_stor.as<S_switch>();
        cinsts_out.emplace_back(do_bind<S_switch, do_execute_switch>(alt));
        return;
      }
    case index_do_while:
      {
        const auto &alt = this->m_stor.as<S_do_while>();
        cinsts_out.emplace_back(do_bind<S_do_while, do_execute_do_while>(alt));
        return;
      }
    case index_while:
      {
        const auto &alt = this->m_stor.as<S_while>();
        cinsts_out.emplace_back(do_bind<S_while, do_execute_while>(alt));
        return;
      }
    case index_for:
      {
        const auto &alt = this->m_stor.as<S_for>();
        cinsts_out.emplace_back(do_bind<S_for, do_execute_for>(alt));
        return;
      }
    case index_for_each:
      {
        const auto &alt = this->m_stor.as<S_for_each>();
        cinsts_out.emplace_back(do_bind<S_for_each, do_execute_for_each>(alt));
        return;
      }
    case index_try:
      {
        const auto &alt = this->m_stor.as<S_try>();
        cinsts_out.emplace_back(do_bind<S_try, do_execute_try>(alt));
        return;
      }
    case index_break:
      {
        const auto &alt = this->m_stor.as<S_break>();
        switch(alt.target) {
        case Statement::target_unspec:
          {
            cinsts_out.emplace_back(do_bind_constant(Block::status_break_unspec));
            return;
          }
        case Statement::target_switch:
          {
            cinsts_out.emplace_back(do_bind_constant(Block::status_break_switch));
            return;
          }
        case Statement::target_while:
          {
            cinsts_out.emplace_back(do_bind_constant(Block::status_break_while));
            return;
          }
        case Statement::target_for:
          {
            cinsts_out.emplace_back(do_bind_constant(Block::status_break_for));
            return;
          }
        default:
          ASTERIA_TERMINATE("An unknown target scope type `", alt.target, "` has been encountered.");
        }
      }
    case index_continue:
      {
        const auto &alt = this->m_stor.as<S_continue>();
        switch(alt.target) {
        case Statement::target_unspec:
          {
            cinsts_out.emplace_back(do_bind_constant(Block::status_continue_unspec));
            return;
          }
        case Statement::target_switch:
          {
            ASTERIA_TERMINATE("`target_switch` is not allowed to follow `continue`.");
          }
        case Statement::target_while:
          {
            cinsts_out.emplace_back(do_bind_constant(Block::status_continue_while));
            return;
          }
        case Statement::target_for:
          {
            cinsts_out.emplace_back(do_bind_constant(Block::status_continue_for));
            return;
          }
        default:
          ASTERIA_TERMINATE("An unknown target scope type `", alt.target, "` has been encountered.");
        }
      }
    case index_throw:
      {
        const auto &alt = this->m_stor.as<S_throw>();
        cinsts_out.emplace_back(do_bind<S_throw, do_execute_throw>(alt));
        return;
      }
    case index_return:
      {
        const auto &alt = this->m_stor.as<S_return>();
        cinsts_out.emplace_back(do_bind<S_return, do_execute_return>(alt));
        return;
      }
    case index_assert:
      {
        const auto &alt = this->m_stor.as<S_assert>();
        cinsts_out.emplace_back(do_bind<S_assert, do_execute_assert>(alt));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

void Statement::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_expression:
      {
        const auto &alt = this->m_stor.as<S_expression>();
        alt.expr.enumerate_variables(callback);
        return;
      }
    case index_block:
      {
        const auto &alt = this->m_stor.as<S_block>();
        alt.body.enumerate_variables(callback);
        return;
      }
    case index_variable:
      {
        const auto &alt = this->m_stor.as<S_variable>();
        alt.init.enumerate_variables(callback);
        return;
      }
    case index_function:
      {
        const auto &alt = this->m_stor.as<S_function>();
        alt.body.enumerate_variables(callback);
        return;
      }
    case index_if:
      {
        const auto &alt = this->m_stor.as<S_if>();
        alt.cond.enumerate_variables(callback);
        alt.branch_true.enumerate_variables(callback);
        alt.branch_false.enumerate_variables(callback);
        return;
      }
    case index_switch:
      {
        const auto &alt = this->m_stor.as<S_switch>();
        alt.ctrl.enumerate_variables(callback);
        for(const auto &pair : alt.clauses) {
          pair.first.enumerate_variables(callback);
          pair.second.enumerate_variables(callback);
        }
        return;
      }
    case index_do_while:
      {
        const auto &alt = this->m_stor.as<S_do_while>();
        alt.body.enumerate_variables(callback);
        alt.cond.enumerate_variables(callback);
        return;
      }
    case index_while:
      {
        const auto &alt = this->m_stor.as<S_while>();
        alt.cond.enumerate_variables(callback);
        alt.body.enumerate_variables(callback);
        return;
      }
    case index_for:
      {
        const auto &alt = this->m_stor.as<S_for>();
        alt.init.enumerate_variables(callback);
        alt.cond.enumerate_variables(callback);
        alt.step.enumerate_variables(callback);
        alt.body.enumerate_variables(callback);
        return;
      }
    case index_for_each:
      {
        const auto &alt = this->m_stor.as<S_for_each>();
        alt.init.enumerate_variables(callback);
        alt.body.enumerate_variables(callback);
        return;
      }
    case index_try:
      {
        const auto &alt = this->m_stor.as<S_try>();
        alt.body_try.enumerate_variables(callback);
        alt.body_catch.enumerate_variables(callback);
        return;
      }
    case index_break:
    case index_continue:
      {
        return;
      }
    case index_throw:
      {
        const auto &alt = this->m_stor.as<S_throw>();
        alt.expr.enumerate_variables(callback);
        return;
      }
    case index_return:
      {
        const auto &alt = this->m_stor.as<S_return>();
        alt.expr.enumerate_variables(callback);
        return;
      }
    case index_assert:
      {
        const auto &alt = this->m_stor.as<S_assert>();
        alt.expr.enumerate_variables(callback);
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}
