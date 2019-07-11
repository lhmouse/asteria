// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "statement.hpp"
#include "xprunit.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/evaluation_stack.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../runtime/exception.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    template<typename XrefT> void do_set_user_declared_reference(Cow_Vector<PreHashed_String>* names_opt, Abstract_Context& ctx,
                                                                 const char* desc, const PreHashed_String& name, XrefT&& xref)
      {
        if(name.empty()) {
          return;
        }
        if(name.rdstr().starts_with("__")) {
          ASTERIA_THROW_RUNTIME_ERROR("The name `", name, "` for this ", desc, " is reserved and cannot be used.");
        }
        if(names_opt) {
          names_opt->emplace_back(name);
        }
        ctx.open_named_reference(name) = rocket::forward<XrefT>(xref);
      }

    Rcptr<Variable> do_safe_create_variable(Cow_Vector<PreHashed_String>* names_opt, Abstract_Context& ctx,
                                            const char* desc, const PreHashed_String& name, const Global_Context& global)
      {
        auto var = global.create_variable();
        Reference_Root::S_variable xref = { var };
        do_set_user_declared_reference(names_opt, ctx, desc, name, rocket::move(xref));
        return var;
      }

    Cow_Vector<Air_Node> do_generate_code_statement_list(Cow_Vector<PreHashed_String>* names_opt, Analytic_Context& ctx,
                                                         const Cow_Vector<Statement>& stmts)
      {
        Cow_Vector<Air_Node> code;
        rocket::for_each(stmts, [&](const Statement& stmt) { stmt.generate_code(code, names_opt, ctx);  });
        return code;
      }

    Cow_Vector<Air_Node> do_generate_code_block(const Analytic_Context& ctx, const Cow_Vector<Statement>& block)
      {
        Analytic_Context ctx_next(&ctx);
        auto code = do_generate_code_statement_list(nullptr, ctx_next, block);
        return code;
      }

    Air_Node::Status do_execute_statement_list(Evaluation_Stack& stack, Executive_Context& ctx,
                                               const Cow_Vector<Air_Node>& code, const Cow_String& func, const Global_Context& global)
      {
        auto status = Air_Node::status_next;
        rocket::any_of(code, [&](const Air_Node& node) { return (status = node.execute(stack, ctx, func, global))
                                                                 != Air_Node::status_next;  });
        return status;
      }

    Air_Node::Status do_execute_block(Evaluation_Stack& stack,
                                      const Executive_Context& ctx, const Cow_Vector<Air_Node>& code, const Cow_String& func, const Global_Context& global)
      {
        Executive_Context ctx_next(&ctx);
        auto status = do_execute_statement_list(stack, ctx_next, code, func, global);
        return status;
      }

    Cow_Vector<Air_Node> do_generate_code_expression(const Analytic_Context& ctx, const Cow_Vector<Xprunit>& expr)
      {
        Cow_Vector<Air_Node> code;
        rocket::for_each(expr, [&](const Xprunit& unit) { unit.generate_code(code, ctx);  });
        return code;
      }

    void do_evaluate_expression_nonempty(Evaluation_Stack& stack, Executive_Context& ctx,
                                         const Cow_Vector<Air_Node>& code, const Cow_String& func, const Global_Context& global)
      {
        stack.clear_references();
        // Evaluate the expression. The result will be pushed on `stack`.
        ROCKET_ASSERT(!code.empty());
        rocket::for_each(code, [&](const Air_Node& node) { node.execute(stack, ctx, func, global);  });
      }

    Air_Node::Status do_execute_clear_stack(Evaluation_Stack& stack, Executive_Context& /*ctx*/,
                                            const Cow_Vector<Air_Node::Parameter>& /*p*/, const Cow_String& /*func*/, const Global_Context& /*global*/)
      {
        // We push a null reference in case of empty expressions.
        stack.clear_references();
        stack.push_reference(Reference_Root::S_null());
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_block_callback(Evaluation_Stack& stack, Executive_Context& ctx,
                                               const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& func, const Global_Context& global)
      {
        // Decode arguments.
        const auto& code = p.at(0).as<Cow_Vector<Air_Node>>();
        // Execute the block without affecting `ctx`.
        auto status = do_execute_block(stack, ctx, code, func, global);
        return status;
      }

    Air_Node::Status do_define_uninitialized_variable(Evaluation_Stack& /*stack*/, Executive_Context& ctx,
                                                      const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& /*func*/, const Global_Context& global)
      {
        // Decode arguments.
        const auto& sloc = p.at(0).as<Source_Location>();
        const auto& immutable = static_cast<bool>(p.at(1).as<std::int64_t>());
        const auto& name = p.at(2).as<PreHashed_String>();
        // Allocate a variable.
        auto var = do_safe_create_variable(nullptr, ctx, "variable", name, global);
        // Initialize the variable.
        var->reset(sloc, G_null(), immutable);
        return Air_Node::status_next;
      }

   Air_Node::Status do_declare_variable_and_clear_stack(Evaluation_Stack& stack, Executive_Context& ctx,
                                                        const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& /*func*/, const Global_Context& global)
      {
        // Decode arguments.
        const auto& name = p.at(0).as<PreHashed_String>();
        // Allocate a variable.
        auto var = do_safe_create_variable(nullptr, ctx, "variable placeholder", name, global);
        stack.set_last_variable(rocket::move(var));
        // Note that the initializer must not be empty for this code.
        stack.clear_references();
        return Air_Node::status_next;
      }

    Air_Node::Status do_initialize_variable(Evaluation_Stack& stack, Executive_Context& /*ctx*/,
                                            const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& /*func*/, const Global_Context& /*global*/)
      {
        // Decode arguments.
        const auto& sloc = p.at(0).as<Source_Location>();
        const auto& immutable = static_cast<bool>(p.at(1).as<std::int64_t>());
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this code.
        auto value = stack.get_top_reference().read();
        stack.pop_reference();
        // Get back the variable that has been allocated in `do_declare_variable_and_clear_stack()`.
        auto var = stack.release_last_variable_opt();
        ROCKET_ASSERT(var);
        var->reset(sloc, rocket::move(value), immutable);
        return Air_Node::status_next;
      }

    Air_Node::Status do_define_function(Evaluation_Stack& /*stack*/, Executive_Context& ctx,
                                        const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& /*func*/, const Global_Context& global)
      {
        // Decode arguments.
        const auto& sloc = p.at(0).as<Source_Location>();
        const auto& name = p.at(1).as<PreHashed_String>();
        const auto& params = p.at(2).as<Cow_Vector<PreHashed_String>>();
        const auto& body = p.at(3).as<Cow_Vector<Statement>>();
        // Create a dummy reference for further name lookups.
        // A function becomes visible before its definition, where it is initialized to `null`.
        auto var = do_safe_create_variable(nullptr, ctx, "function", name, global);
        // Generate code for the function body.
        Cow_Vector<Air_Node> code_func;
        Analytic_Context ctx_func(&ctx);
        ctx_func.prepare_function_parameters(params);
        rocket::for_each(body, [&](const Statement& stmt) { stmt.generate_code(code_func, nullptr, ctx_func);  });
        // Format the prototype string.
        Cow_osstream fmtss;
        fmtss.imbue(std::locale::classic());
        fmtss << name << "(";
        if(!params.empty()) {
          std::for_each(params.begin(), params.end() - 1, [&](const PreHashed_String& param) { fmtss << param << ", ";  });
          fmtss << params.back();
        }
        fmtss <<")";
        Rcobj<Instantiated_Function> closure(sloc, fmtss.extract_string(), params, rocket::move(code_func));
        // Initialized the function variable.
        var->reset(sloc, G_function(rocket::move(closure)), true);
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_branch(Evaluation_Stack& stack, Executive_Context& ctx,
                                       const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& func, const Global_Context& global)
      {
        // Decode arguments.
        const auto& negative = static_cast<bool>(p.at(0).as<std::int64_t>());
        const auto& code_true = p.at(1).as<Cow_Vector<Air_Node>>();
        const auto& code_false = p.at(2).as<Cow_Vector<Air_Node>>();
        // Pick a branch basing on the condition.
        if(stack.get_top_reference().read().test() != negative) {
          // Execute the true branch. Forward any status codes unexpected to the caller.
          auto status = do_execute_block(stack, ctx, code_true, func, global);
          if(rocket::is_none_of(status, { Air_Node::status_next })) {
            return status;
          }
          return Air_Node::status_next;
        }
        // Execute the false branch. Forward any status codes unexpected to the caller.
        auto status = do_execute_block(stack, ctx, code_false, func, global);
        if(rocket::is_none_of(status, { Air_Node::status_next })) {
          return status;
        }
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_select(Evaluation_Stack& stack, Executive_Context& ctx,
                                       const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& func, const Global_Context& global)
      {
        // This is different from a C `switch` statement where `case` labels must have constant operands.
        // Evaluate the control expression.
        auto ctrl_value = stack.get_top_reference().read();
        // Set the target clause.
        Opt<Cow_Vector<Air_Node::Parameter>::const_iterator> qtarget;
        // Iterate over all `case` labels and evaluate them. Stop if the result value equals `ctrl_value`.
        // In this loop, `qtarget` points to the `default` clause.
        for(auto it = p.begin(); it != p.end(); it += 3) {
          // Decode arguments.
          const auto& code_cond = it[0].as<Cow_Vector<Air_Node>>();
          if(code_cond.empty()) {
            // This is a `default` clause.
            if(qtarget) {
              ASTERIA_THROW_RUNTIME_ERROR("Multiple `default` clauses have been found in this `switch` statement.");
            }
            qtarget = it;
            continue;
          }
          // This is a `case` clause.
          // Evaluate the operand and check whether it equals `ctrl_value`.
          do_evaluate_expression_nonempty(stack, ctx, code_cond, func, global);
          if(stack.get_top_reference().read().compare(ctrl_value) == Value::compare_equal) {
            // Found a `case` label. Stop.
            qtarget = it;
            break;
          }
        }
        if(!qtarget) {
          // No match clause has been found.
          return Air_Node::status_next;
        }
        // Jump to the clause denoted by `*qtarget`.
        // Note that all clauses share the same context.
        Executive_Context ctx_body(&ctx);
        // Skip clauses that precede `*qtarget`.
        for(auto it = p.begin(); it != *qtarget; it += 3) {
          // Decode arguments.
          const auto& names = it[2].as<Cow_Vector<PreHashed_String>>();
          // Inject all names into this scope.
          rocket::for_each(names, [&](const PreHashed_String& name) { do_set_user_declared_reference(nullptr, ctx_body, "skipped reference",
                                                                                                     name, Reference_Root::S_null());  });
        }
        // Execute all clauses from `*qtarget` to the end of this block.
        for(auto it = *qtarget; it != p.end(); it += 3) {
          // Decode arguments.
          const auto& code_clause = it[1].as<Cow_Vector<Air_Node>>();
          // Execute the clause. Break out of the block if requested. Forward any status codes unexpected to the caller.
          auto status = do_execute_statement_list(stack, ctx_body, code_clause, func, global);
          if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_switch })) {
            break;
          }
          if(rocket::is_none_of(status, { Air_Node::status_next })) {
            return status;
          }
        }
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_do_while(Evaluation_Stack& stack, Executive_Context& ctx,
                                         const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& func, const Global_Context& global)
      {
        // Decode arguments.
        const auto& code_body = p.at(0).as<Cow_Vector<Air_Node>>();
        const auto& negative = static_cast<bool>(p.at(1).as<std::int64_t>());
        const auto& code_cond = p.at(2).as<Cow_Vector<Air_Node>>();
        // This is the same as a `do...while` loop in C.
        for(;;) {
          // Execute the body. Break out of the loop if requested. Forward any status codes unexpected to the caller.
          auto status = do_execute_block(stack, ctx, code_body, func, global);
          if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_while })) {
            break;
          }
          if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_while })) {
            return status;
          }
          // Check the condition.
          do_evaluate_expression_nonempty(stack, ctx, code_cond, func, global);
          if(stack.get_top_reference().read().test() == negative) {
            break;
          }
        }
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_while(Evaluation_Stack& stack, Executive_Context& ctx,
                                      const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& func, const Global_Context& global)
      {
        // Decode arguments.
        const auto& negative = static_cast<bool>(p.at(0).as<std::int64_t>());
        const auto& code_cond = p.at(1).as<Cow_Vector<Air_Node>>();
        const auto& code_body = p.at(2).as<Cow_Vector<Air_Node>>();
        // This is the same as a `while` loop in C.
        for(;;) {
          // Check the condition.
          do_evaluate_expression_nonempty(stack, ctx, code_cond, func, global);
          if(stack.get_top_reference().read().test() == negative) {
            break;
          }
          // Execute the body. Break out of the loop if requested. Forward any status codes unexpected to the caller.
          auto status = do_execute_block(stack, ctx, code_body, func, global);
          if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_while })) {
            break;
          }
          if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_while })) {
            return status;
          }
        }
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_for_each(Evaluation_Stack& stack, Executive_Context& ctx,
                                         const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& func, const Global_Context& global)
      {
        // Decode arguments.
        const auto& key_name = p.at(0).as<PreHashed_String>();
        const auto& mapped_name = p.at(1).as<PreHashed_String>();
        const auto& code_init = p.at(2).as<Cow_Vector<Air_Node>>();
        const auto& code_body = p.at(3).as<Cow_Vector<Air_Node>>();
        // This is the same as a ranged-`for` loop in C++.
        Executive_Context ctx_for(&ctx);
        auto key_var = do_safe_create_variable(nullptr, ctx_for, "key variable", key_name, global);
        do_set_user_declared_reference(nullptr, ctx_for, "mapped reference", mapped_name, Reference_Root::S_null());
        // Evaluate the range initializer.
        do_evaluate_expression_nonempty(stack, ctx_for, code_init, func, global);
        auto range_ref = rocket::move(stack.open_top_reference());
        auto range_value = range_ref.read();
        // Iterate over the range.
        if(range_value.is_array()) {
          const auto& array = range_value.as_array();
          for(auto it = array.begin(); it != array.end(); ++it) {
            // Create a fresh context for the loop body.
            Executive_Context ctx_body(&ctx_for);
            // Set up the key variable, which is immutable.
            key_var->reset(Source_Location(rocket::sref("<built-in>"), 0), G_integer(it - array.begin()), true);
            // Set up the mapped reference.
            Reference_Modifier::S_array_index xrefm = { it - array.begin() };
            range_ref.zoom_in(rocket::move(xrefm));
            do_set_user_declared_reference(nullptr, ctx_for, "mapped reference", mapped_name, range_ref);
            range_ref.zoom_out();
            // Execute the loop body.
            auto status = do_execute_statement_list(stack, ctx_body, code_body, func, global);
            if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_for })) {
              break;
            }
            if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_for })) {
              return status;
            }
          }
          goto z;
        }
        if(range_value.is_object()) {
          const auto& object = range_value.as_object();
          for(auto it = object.begin(); it != object.end(); ++it) {
            // Create a fresh context for the loop body.
            Executive_Context ctx_body(&ctx_for);
            // Set up the key variable, which is immutable.
            key_var->reset(Source_Location(rocket::sref("<built-in>"), 0), G_string(it->first), true);
            // Set up the mapped reference.
            Reference_Modifier::S_object_key xrefm = { it->first };
            range_ref.zoom_in(rocket::move(xrefm));
            do_set_user_declared_reference(nullptr, ctx_for, "mapped reference", mapped_name, range_ref);
            range_ref.zoom_out();
            // Execute the loop body.
            auto status = do_execute_statement_list(stack, ctx_body, code_body, func, global);
            if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_for })) {
              break;
            }
            if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_for })) {
              return status;
            }
          }
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The `for each` statement does not accept a range of type `", range_value.gtype_name(), "`.");
      z:
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_for(Evaluation_Stack& stack, Executive_Context& ctx,
                                    const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& func, const Global_Context& global)
      {
        // Decode arguments.
        const auto& code_init = p.at(0).as<Cow_Vector<Air_Node>>();
        const auto& code_cond = p.at(1).as<Cow_Vector<Air_Node>>();
        const auto& code_step = p.at(2).as<Cow_Vector<Air_Node>>();
        const auto& code_body = p.at(3).as<Cow_Vector<Air_Node>>();
        // This is the same as a `for` loop in C.
        Executive_Context ctx_for(&ctx);
        do_execute_statement_list(stack, ctx_for, code_init, func, global);
        for(;;) {
          // Treat an empty condition as being always true.
          if(!code_cond.empty()) {
            // Check the condition.
            do_evaluate_expression_nonempty(stack, ctx_for, code_cond, func, global);
            if(stack.get_top_reference().read().test() == false) {
              break;
            }
          }
          // Execute the body. Break out of the loop if requested. Forward any status codes unexpected to the caller.
          auto status = do_execute_block(stack, ctx_for, code_body, func, global);
          if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_for })) {
            break;
          }
          if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_for })) {
            return status;
          }
          // Evaluate the step expression and discard its value.
          if(!code_step.empty()) {
            do_evaluate_expression_nonempty(stack, ctx_for, code_step, func, global);
          }
        }
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_try(Evaluation_Stack& stack, Executive_Context& ctx,
                                    const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& func, const Global_Context& global)
      {
        // Decode arguments.
        const auto& code_try = p.at(0).as<Cow_Vector<Air_Node>>();
        const auto& sloc = p.at(1).as<Source_Location>();
        const auto& except_name = p.at(2).as<PreHashed_String>();
        const auto& code_catch = p.at(3).as<Cow_Vector<Air_Node>>();
        // This is the same as a `try...catch` block in C++.
        try {
          // Execute the `try` clause. If no exception is thrown, this will have little cost.
          auto status = do_execute_block(stack, ctx, code_try, func, global);
          if(rocket::is_none_of(status, { Air_Node::status_next })) {
            return status;
          }
          return Air_Node::status_next;
        }
        catch(const std::exception& stdex) {
          // Translate the exception.
          Exception except(stdex);
          except.push_frame(sloc, Backtrace_Frame::ftype_catch, G_null());
          // The exception reference shall not outlast the `catch` body.
          Executive_Context ctx_catch(&ctx);
          Reference_Root::S_temporary xref = { except.get_value() };
          do_set_user_declared_reference(nullptr, ctx_catch, "exception reference", except_name, rocket::move(xref));
          // Initialize backtrace information.
          G_array backtrace;
          for(std::size_t i = 0; i < except.count_frames(); ++i) {
            const auto& frame = except.get_frame(i);
            G_object elem;
            // Translate the frame.
            elem.try_emplace(rocket::sref("file"), G_string(frame.file()));
            elem.try_emplace(rocket::sref("line"), G_integer(frame.line()));
            elem.try_emplace(rocket::sref("ftype"), G_string(rocket::sref(Backtrace_Frame::stringify_ftype(frame.ftype()))));
            elem.try_emplace(rocket::sref("value"), frame.value());
            // Append the frame.
            backtrace.emplace_back(rocket::move(elem));
          }
          xref.value = rocket::move(backtrace);
          ASTERIA_DEBUG_LOG("Exception backtrace:\n", xref.value);
          ctx_catch.open_named_reference(rocket::sref("__backtrace")) = rocket::move(xref);
          // Execute the `catch` body.
          auto status = do_execute_statement_list(stack, ctx_catch, code_catch, func, global);
          if(rocket::is_none_of(status, { Air_Node::status_next })) {
            return status;
          }
          return Air_Node::status_next;
        }
      }

    Air_Node::Status do_return_status_simple(Evaluation_Stack& /*stack*/, Executive_Context& /*ctx*/,
                                             const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& /*func*/, const Global_Context& /*global*/)
      {
        // Decode arguments.
        const auto& status = static_cast<Air_Node::Status>(p.at(0).as<std::int64_t>());
        // Return the status as is.
        return status;
      }

    [[noreturn]] Air_Node::Status do_execute_throw(Evaluation_Stack& stack, Executive_Context& /*ctx*/,
                                                   const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& /*func*/, const Global_Context& /*global*/)
      {
        // Decode arguments.
        const auto& sloc = p.at(0).as<Source_Location>();
        // Store the nested exception here, if any.
        Exception except;
        auto qnested = std::current_exception();
        if(qnested) {
          // Rethrow it to get its effective type.
          try {
            std::rethrow_exception(qnested);
          }
          catch(const std::exception& stdex) {
            except.dynamic_copy(stdex);
          }
          catch(...) {
            // Fallthrough.
          }
        }
        // Save the previous exception.
        except.push_frame(sloc, Backtrace_Frame::ftype_throw, rocket::move(except.open_value()));
        // Coyp and throw the value at the top of the stack, as we don't throw by reference.
        except.open_value() = stack.get_top_reference().read();
        throw except;
      }

    Air_Node::Status do_execute_assert(Evaluation_Stack& stack, Executive_Context& /*ctx*/,
                                       const Cow_Vector<Air_Node::Parameter>& p, const Cow_String& /*func*/, const Global_Context& /*global*/)
      {
        // Decode arguments.
        const auto& sloc = p.at(0).as<Source_Location>();
        const auto& negative = static_cast<bool>(p.at(1).as<std::int64_t>());
        const auto& msg = p.at(2).as<PreHashed_String>();
        // If the assertion succeeds, there is no effect.
        if(ROCKET_EXPECT(stack.get_top_reference().read().test() != negative)) {
          return Air_Node::status_next;
        }
        // Throw a `Runtime_Error` if the assertion fails.
        Cow_osstream fmtss;
        fmtss.imbue(std::locale::classic());
        fmtss << "Assertion failed at \'" << sloc << "\'";
        if(msg.empty()) {
          fmtss << "!";
        }
        else {
          fmtss << ": " << msg;
        }
        throw_runtime_error(__func__, fmtss.extract_string());
      }

    }  // namespace

void Statement::generate_code(Cow_Vector<Air_Node>& code, Cow_Vector<PreHashed_String>* names_opt, Analytic_Context& ctx) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_expression:
      {
        const auto& altr = this->m_stor.as<index_expression>();
        if(altr.expr.empty()) {
          // Generate nothing for empty expressions.
          return;
        }
        // Generate preparation code.
        Cow_Vector<Air_Node::Parameter> p;
        code.emplace_back(do_execute_clear_stack, rocket::move(p));
        // Generate inline code for the expression.
        rocket::for_each(altr.expr, [&](const Xprunit& unit) { unit.generate_code(code, ctx);  });
        return;
      }
    case index_block:
      {
        const auto& altr = this->m_stor.as<index_block>();
        // Encode arguments.
        Cow_Vector<Air_Node::Parameter> p;
        p.emplace_back(do_generate_code_block(ctx, altr.body));  // 0
        code.emplace_back(do_execute_block_callback, rocket::move(p));
        return;
      }
    case index_variable:
      {
        const auto& altr = this->m_stor.as<index_variable>();
        // There may be multiple variables.
        for(const auto& pair : altr.vars) {
          // Create a dummy reference for further name lookups.
          do_set_user_declared_reference(names_opt, ctx, "variable placeholder", pair.first, Reference_Root::S_null());
          // Distinguish uninitialized variables from initialized ones.
          if(pair.second.empty()) {
            Cow_Vector<Air_Node::Parameter> p;
            p.emplace_back(altr.sloc);  // 0
            p.emplace_back(static_cast<std::int64_t>(altr.immutable));  // 1
            p.emplace_back(pair.first);  // 2
            code.emplace_back(do_define_uninitialized_variable, rocket::move(p));
            continue;
          }
          // A variable becomes visible before its initializer, where it is initialized to `null`.
          Cow_Vector<Air_Node::Parameter> p;
          p.emplace_back(pair.first);  // 0
          code.emplace_back(do_declare_variable_and_clear_stack, rocket::move(p));
          // Generate inline code for the initializer.
          rocket::for_each(pair.second, [&](const Xprunit& unit) { unit.generate_code(code, ctx);  });
          // Generate code to initialize the variable.
          p.clear();
          p.emplace_back(altr.sloc);  // 0
          p.emplace_back(static_cast<std::int64_t>(altr.immutable));  // 1
          code.emplace_back(do_initialize_variable, rocket::move(p));
        }
        return;
      }
    case index_function:
      {
        const auto& altr = this->m_stor.as<index_function>();
        // Create a dummy reference for further name lookups.
        do_set_user_declared_reference(names_opt, ctx, "function placeholder", altr.name, Reference_Root::S_null());
        // Encode arguments.
        Cow_Vector<Air_Node::Parameter> p;
        p.emplace_back(altr.sloc);  // 0
        p.emplace_back(altr.name);  // 1
        p.emplace_back(altr.params);  // 2
        p.emplace_back(altr.body);  // 3
        code.emplace_back(do_define_function, rocket::move(p));
        return;
      }
    case index_if:
      {
        const auto& altr = this->m_stor.as<index_if>();
        // Generate preparation code.
        Cow_Vector<Air_Node::Parameter> p;
        code.emplace_back(do_execute_clear_stack, rocket::move(p));
        // Generate inline code for the condition expression.
        rocket::for_each(altr.cond, [&](const Xprunit& unit) { unit.generate_code(code, ctx);  });
        // Encode arguments.
        p.clear();
        p.emplace_back(static_cast<std::int64_t>(altr.negative));  // 0
        p.emplace_back(do_generate_code_block(ctx, altr.branch_true));  // 1
        p.emplace_back(do_generate_code_block(ctx, altr.branch_false));  // 2
        code.emplace_back(do_execute_branch, rocket::move(p));
        return;
      }
    case index_switch:
      {
        const auto& altr = this->m_stor.as<index_switch>();
        // Generate preparation code.
        Cow_Vector<Air_Node::Parameter> p;
        code.emplace_back(do_execute_clear_stack, rocket::move(p));
        // Generate inline code for the condition expression.
        rocket::for_each(altr.ctrl, [&](const Xprunit& unit) { unit.generate_code(code, ctx);  });
        // Create a fresh context for the `switch` body.
        // Note that all clauses inside a `switch` statement share the same context.
        Analytic_Context ctx_switch(&ctx);
        // Encode arguments.
        p.clear();
        // Note that this node takes variable number of arguments.
        // Names are accumulated.
        Cow_Vector<PreHashed_String> names;
        for(const auto& pair : altr.clauses) {
          p.emplace_back(do_generate_code_expression(ctx_switch, pair.first));  // n * 3 + 0
          p.emplace_back(do_generate_code_statement_list(&names, ctx_switch, pair.second));  // n * 3 + 1
          p.emplace_back(names);  // n * 3 + 2
        }
        code.emplace_back(do_execute_select, rocket::move(p));
        return;
      }
    case index_do_while:
      {
        const auto& altr = this->m_stor.as<index_do_while>();
        // Encode arguments.
        Cow_Vector<Air_Node::Parameter> p;
        p.emplace_back(do_generate_code_block(ctx, altr.body));  // 0
        p.emplace_back(static_cast<std::int64_t>(altr.negative));  // 1
        p.emplace_back(do_generate_code_expression(ctx, altr.cond));  // 2
        code.emplace_back(do_execute_do_while, rocket::move(p));
        return;
      }
    case index_while:
      {
        const auto& altr = this->m_stor.as<index_while>();
        // Encode arguments.
        Cow_Vector<Air_Node::Parameter> p;
        p.emplace_back(static_cast<std::int64_t>(altr.negative));  // 0
        p.emplace_back(do_generate_code_expression(ctx, altr.cond));  // 1
        p.emplace_back(do_generate_code_block(ctx, altr.body));  // 2
        code.emplace_back(do_execute_while, rocket::move(p));
        return;
      }
    case index_for_each:
      {
        const auto& altr = this->m_stor.as<index_for_each>();
        // Create a fresh context for the `for` loop.
        Analytic_Context ctx_for(&ctx);
        do_set_user_declared_reference(nullptr, ctx_for, "key placeholder", altr.key_name, Reference_Root::S_null());
        do_set_user_declared_reference(nullptr, ctx_for, "mapped placeholder", altr.mapped_name, Reference_Root::S_null());
        // Encode arguments.
        Cow_Vector<Air_Node::Parameter> p;
        p.emplace_back(altr.key_name);  // 0
        p.emplace_back(altr.mapped_name);  // 1
        p.emplace_back(do_generate_code_expression(ctx_for, altr.init));  // 2
        p.emplace_back(do_generate_code_block(ctx_for, altr.body));  // 3
        code.emplace_back(do_execute_for_each, rocket::move(p));
        return;
      }
    case index_for:
      {
        const auto& altr = this->m_stor.as<index_for>();
        // Create a fresh context for the `for` loop.
        Analytic_Context ctx_for(&ctx);
        // Encode arguments.
        Cow_Vector<Air_Node::Parameter> p;
        p.emplace_back(do_generate_code_statement_list(nullptr, ctx_for, altr.init));  // 0
        p.emplace_back(do_generate_code_expression(ctx_for, altr.cond));  // 1
        p.emplace_back(do_generate_code_expression(ctx_for, altr.step));  // 2
        p.emplace_back(do_generate_code_block(ctx_for, altr.body));  // 3
        code.emplace_back(do_execute_for, rocket::move(p));
        return;
      }
    case index_try:
      {
        const auto& altr = this->m_stor.as<index_try>();
        // Create a fresh context for the `catch` clause.
        Analytic_Context ctx_catch(&ctx);
        do_set_user_declared_reference(nullptr, ctx_catch, "exception placeholder", altr.except_name, Reference_Root::S_null());
        ctx_catch.open_named_reference(rocket::sref("__backtrace")) /*= Reference_Root::S_null()*/;
        // Encode arguments.
        Cow_Vector<Air_Node::Parameter> p;
        p.emplace_back(do_generate_code_block(ctx, altr.body_try));  // 0
        p.emplace_back(altr.sloc);  // 1
        p.emplace_back(altr.except_name);  // 2
        p.emplace_back(do_generate_code_statement_list(nullptr, ctx_catch, altr.body_catch));  // 3
        code.emplace_back(do_execute_try, rocket::move(p));
        return;
      }
    case index_break:
      {
        const auto& altr = this->m_stor.as<index_break>();
        // Encode arguments.
        Cow_Vector<Air_Node::Parameter> p;
        switch(altr.target) {
        case Statement::target_unspec:
          {
            p.emplace_back(static_cast<std::int64_t>(Air_Node::status_break_unspec));  // 0
            break;
          }
        case Statement::target_switch:
          {
            p.emplace_back(static_cast<std::int64_t>(Air_Node::status_break_switch));  // 0
            break;
          }
        case Statement::target_while:
          {
            p.emplace_back(static_cast<std::int64_t>(Air_Node::status_break_while));  // 0
            break;
          }
        case Statement::target_for:
          {
            p.emplace_back(static_cast<std::int64_t>(Air_Node::status_break_for));  // 0
            break;
          }
        default:
          ASTERIA_TERMINATE("An unknown target scope type `", altr.target, "` has been encountered.");
        }
        code.emplace_back(do_return_status_simple, rocket::move(p));
        return;
      }
    case index_continue:
      {
        const auto& altr = this->m_stor.as<index_continue>();
        // Encode arguments.
        Cow_Vector<Air_Node::Parameter> p;
        switch(altr.target) {
        case Statement::target_unspec:
          {
            p.emplace_back(static_cast<std::int64_t>(Air_Node::status_continue_unspec));  // 0
            break;
          }
        case Statement::target_switch:
          {
            ASTERIA_TERMINATE("`target_switch` is not allowed to follow `continue`.");
          }
        case Statement::target_while:
          {
            p.emplace_back(static_cast<std::int64_t>(Air_Node::status_continue_while));  // 0
            break;
          }
        case Statement::target_for:
          {
            p.emplace_back(static_cast<std::int64_t>(Air_Node::status_continue_for));  // 0
            break;
          }
        default:
          ASTERIA_TERMINATE("An unknown target scope type `", altr.target, "` has been encountered.");
        }
        code.emplace_back(do_return_status_simple, rocket::move(p));
        return;
      }
    case index_throw:
      {
        const auto& altr = this->m_stor.as<index_throw>();
        // Generate preparation code.
        Cow_Vector<Air_Node::Parameter> p;
        code.emplace_back(do_execute_clear_stack, rocket::move(p));
        // Generate inline code for the operand.
        rocket::for_each(altr.expr, [&](const Xprunit& unit) { unit.generate_code(code, ctx);  });
        // Encode arguments.
        p.clear();
        p.emplace_back(altr.sloc);  // 0
        code.emplace_back(do_execute_throw, rocket::move(p));
        return;
      }
    case index_return:
      {
        const auto& altr = this->m_stor.as<index_return>();
        // Generate preparation code.
        Cow_Vector<Air_Node::Parameter> p;
        code.emplace_back(do_execute_clear_stack, rocket::move(p));
        // Generate inline code for the operand.
        rocket::for_each(altr.expr, [&](const Xprunit& unit) { unit.generate_code(code, ctx);  });
        // Encode arguments.
        p.clear();
        p.emplace_back(static_cast<std::int64_t>(Air_Node::status_return));  // 0
        code.emplace_back(do_return_status_simple, rocket::move(p));
        return;
      }
    case index_assert:
      {
        const auto& altr = this->m_stor.as<index_assert>();
        // Generate preparation code.
        Cow_Vector<Air_Node::Parameter> p;
        code.emplace_back(do_execute_clear_stack, rocket::move(p));
        // Generate inline code for the operand.
        rocket::for_each(altr.expr, [&](const Xprunit& unit) { unit.generate_code(code, ctx);  });
        // Encode arguments.
        p.clear();
        p.emplace_back(altr.sloc);  // 0
        p.emplace_back(static_cast<std::int64_t>(altr.negative));  // 1
        p.emplace_back(PreHashed_String(altr.msg));  // 2
        code.emplace_back(do_execute_assert, rocket::move(p));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}  // namespace Asteria
