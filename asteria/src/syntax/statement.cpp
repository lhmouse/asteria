// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "statement.hpp"
#include "xpnode.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/reference_stack.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/function_analytic_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    template<typename XrefT> void do_safe_set_named_reference(Cow_Vector<PreHashed_String> *names_out_opt, Abstract_Context &ctx_io,
                                                              const char *desc, const PreHashed_String &name, XrefT &&xref)
      {
        if(name.empty()) {
          return;
        }
        if(name.rdstr().starts_with("__")) {
          ASTERIA_THROW_RUNTIME_ERROR("The name `", name, "` for this ", desc, " is reserved and cannot be used.");
        }
        // Create a named reference.
        if(names_out_opt) {
          names_out_opt->emplace_back(name);
        }
        ctx_io.open_named_reference(name) = std::forward<XrefT>(xref);
      }

    Cow_Vector<Air_Node> do_generate_code_for_statement_list(Cow_Vector<PreHashed_String> *names_out_opt, Analytic_Context &ctx_io,
                                                             const Cow_Vector<Statement> &stmts)
      {
        Cow_Vector<Air_Node> code;
        rocket::for_each(stmts, [&](const Statement &stmt) { stmt.generate_code(code, names_out_opt, ctx_io);  });
        return code;
      }

    Cow_Vector<Air_Node> do_generate_code_for_block(const Analytic_Context &ctx, const Cow_Vector<Statement> &block)
      {
        Analytic_Context ctx_next(&ctx);
        auto code = do_generate_code_for_statement_list(nullptr, ctx_next, block);
        return code;
      }

    Air_Node::Status do_execute_statement_list(Reference_Stack &stack, Executive_Context &ctx_io,
                                               const Cow_Vector<Air_Node> &code, const Cow_String &func, const Global_Context &global)
      {
        auto status = Air_Node::status_next;
        rocket::any_of(code, [&](const Air_Node &node) { return (status = node.execute(stack, ctx_io, func, global))
                                                                 != Air_Node::status_next;  });
        return status;
      }

    Air_Node::Status do_execute_block(Reference_Stack &stack,
                                      const Executive_Context &ctx, const Cow_Vector<Air_Node> &code, const Cow_String &func, const Global_Context &global)
      {
        Executive_Context ctx_next(&ctx);
        auto status = do_execute_statement_list(stack, ctx_next, code, func, global);
        return status;
      }

    Cow_Vector<Air_Node> do_generate_code_for_expression(const Analytic_Context &ctx, const Cow_Vector<Xpnode> &expr)
      {
        Cow_Vector<Air_Node> code;
        rocket::for_each(expr, [&](const Xpnode &xpn) { xpn.generate_code(code, ctx);  });
        return code;
      }

    const Reference & do_evaluate_expression(Reference_Stack &stack, Executive_Context &ctx,
                                             const Cow_Vector<Air_Node> &code, const Cow_String &func, const Global_Context &global)
      {
        stack.clear();
        if(code.empty()) {
          return stack.push(Reference_Root::S_uninitialized());
        }
        rocket::for_each(code, [&](const Air_Node &node) { node.execute(stack, ctx, func, global);  });
        return stack.top();
      }

    Air_Node::Status do_execute_clear_stack(Reference_Stack &stack, Executive_Context & /*ctx_io*/,
                                            const Cow_Vector<Air_Node::Variant> & /*p*/, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        stack.clear();
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_block_callback(Reference_Stack &stack, Executive_Context &ctx_io,
                                               const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &code = p.at(0).as<Cow_Vector<Air_Node>>();
        // Execute the block without affecting `ctx_io`.
        auto status = do_execute_block(stack, ctx_io, code, func, global);
        return status;
      }

    Air_Node::Status do_define_variable(Reference_Stack &stack, Executive_Context &ctx_io,
                                        const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        // - unused - const auto &sloc = p.at(0).as<Source_Location>();
        const auto &name = p.at(1).as<PreHashed_String>();
        const bool immutable = p.at(2).as<std::int64_t>();
        const auto &code_init = p.at(3).as<Cow_Vector<Air_Node>>();
        // Create a dummy reference for further name lookups.
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        auto var = global.create_variable();
        Reference_Root::S_variable ref_c = { var };
        do_safe_set_named_reference(nullptr, ctx_io, "variable", name, rocket::move(ref_c));
        // Evaluate the initializer.
        do_evaluate_expression(stack, ctx_io, code_init, func, global);
        var->reset(stack.top().read(), immutable);
        ASTERIA_DEBUG_LOG("New variable: ", name, " = ", var->get_value());
        return Air_Node::status_next;
      }

    Air_Node::Status do_define_function(Reference_Stack &stack, Executive_Context &ctx_io,
                                        const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &sloc = p.at(0).as<Source_Location>();
        const auto &name = p.at(1).as<PreHashed_String>();
        const auto &params = p.at(2).as<Cow_Vector<PreHashed_String>>();
        const auto &body = p.at(3).as<Cow_Vector<Statement>>();
        // Create a dummy reference for further name lookups.
        // A function becomes visible before its definition, where it is initialized to `null`.
        auto var = global.create_variable();
        Reference_Root::S_variable ref_c = { var };
        do_safe_set_named_reference(nullptr, ctx_io, "function", name, rocket::move(ref_c));
        // Generate code for the function body.
        Cow_Vector<Air_Node> code_next;
        Function_Analytic_Context ctx_next(&ctx_io, params);
        rocket::for_each(body, [&](const Statement &stmt) { stmt.generate_code(code_next, nullptr, ctx_next);  });
        // Instantiate the function.
        rocket::insertable_ostream nos;
        nos << name << "("
            << rocket::ostream_implode(params.begin(), params.size(), ", ")
            <<")";
        Rcobj<Instantiated_Function> closure(sloc, nos.extract_string(), params, rocket::move(code_next));
        ASTERIA_DEBUG_LOG("New function: ", closure);
        var->reset(D_function(rocket::move(closure)), true);
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_if(Reference_Stack &stack, Executive_Context &ctx_io,
                                   const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const bool negative = p.at(0).as<std::int64_t>();
        const auto &code_cond = p.at(1).as<Cow_Vector<Air_Node>>();
        const auto &code_true = p.at(2).as<Cow_Vector<Air_Node>>();
        const auto &code_false = p.at(3).as<Cow_Vector<Air_Node>>();
        // Pick a branch basing on the condition.
        do_evaluate_expression(stack, ctx_io, code_cond, func, global);
        if(stack.top().read().test() != negative) {
          // Execute the true branch. Forward any status codes unexpected to the caller.
          auto status = do_execute_block(stack, ctx_io, code_true, func, global);
          if(rocket::is_none_of(status, { Air_Node::status_next })) {
            return status;
          }
          return Air_Node::status_next;
        }
        // Execute the false branch. Forward any status codes unexpected to the caller.
        auto status = do_execute_block(stack, ctx_io, code_false, func, global);
        if(rocket::is_none_of(status, { Air_Node::status_next })) {
          return status;
        }
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_do_while(Reference_Stack &stack, Executive_Context &ctx_io,
                                         const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &code_body = p.at(0).as<Cow_Vector<Air_Node>>();
        const bool negative = p.at(1).as<std::int64_t>();
        const auto &code_cond = p.at(2).as<Cow_Vector<Air_Node>>();
        // This is the same as a `do...while` loop in C.
        for(;;) {
          // Execute the body. Break out of the loop if requested. Forward any status codes unexpected to the caller.
          auto status = do_execute_block(stack, ctx_io, code_body, func, global);
          if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_while })) {
            return status;
          }
          if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_while })) {
            return status;
          }
          // Check the condition.
          do_evaluate_expression(stack, ctx_io, code_cond, func, global);
          if(stack.top().read().test() == negative) {
            break;
          }
        }
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_while(Reference_Stack &stack, Executive_Context &ctx_io,
                                      const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const bool negative = p.at(0).as<std::int64_t>();
        const auto &code_cond = p.at(1).as<Cow_Vector<Air_Node>>();
        const auto &code_body = p.at(2).as<Cow_Vector<Air_Node>>();
        // This is the same as a `while` loop in C.
        for(;;) {
          // Check the condition.
          do_evaluate_expression(stack, ctx_io, code_cond, func, global);
          if(stack.top().read().test() == negative) {
            break;
          }
          // Execute the body. Break out of the loop if requested. Forward any status codes unexpected to the caller.
          auto status = do_execute_block(stack, ctx_io, code_body, func, global);
          if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_while })) {
            return status;
          }
          if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_while })) {
            return status;
          }
        }
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_for(Reference_Stack &stack, Executive_Context &ctx_io,
                                    const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &code_init = p.at(0).as<Cow_Vector<Air_Node>>();
        const auto &code_cond = p.at(1).as<Cow_Vector<Air_Node>>();
        const auto &code_step = p.at(2).as<Cow_Vector<Air_Node>>();
        const auto &code_body = p.at(3).as<Cow_Vector<Air_Node>>();
        // This is the same as a `for` loop in C.
        Executive_Context ctx_for(&ctx_io);
        do_execute_statement_list(stack, ctx_for, code_init, func, global);
        for(;;) {
          // Check the condition.
          do_evaluate_expression(stack, ctx_for, code_cond, func, global);
          if(stack.top().read().test() == false) {
            break;
          }
          // Execute the body. Break out of the loop if requested. Forward any status codes unexpected to the caller.
          auto status = do_execute_block(stack, ctx_for, code_body, func, global);
          if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_for })) {
            return status;
          }
          if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_for })) {
            return status;
          }
          // Evaluate the step expression and discard its value.
          do_evaluate_expression(stack, ctx_for, code_step, func, global);
        }
        return Air_Node::status_next;
      }

    }

void Statement::generate_code(Cow_Vector<Air_Node> &code_out, Cow_Vector<PreHashed_String> *names_out_opt, Analytic_Context &ctx_io) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_expression:
      {
        const auto &alt = this->m_stor.as<S_expression>();
        // Generate preparation code.
        Cow_Vector<Air_Node::Variant> p;
        code_out.emplace_back(&do_execute_clear_stack, rocket::move(p));
        // Generate inline code for the expression.
        rocket::for_each(alt.expr, [&](const Xpnode &xpn) { xpn.generate_code(code_out, ctx_io);  });
        return;
      }
    case index_block:
      {
        const auto &alt = this->m_stor.as<S_block>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(do_generate_code_for_block(ctx_io, alt.body));  // 0
        code_out.emplace_back(&do_execute_block_callback, rocket::move(p));
        return;
      }
    case index_variable:
      {
        const auto &alt = this->m_stor.as<S_variable>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(names_out_opt, ctx_io, "variable placeholder", alt.name, Reference_Root::S_uninitialized());
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.sloc);  // 0
        p.emplace_back(alt.name);  // 1
        p.emplace_back(static_cast<std::int64_t>(alt.immutable));  // 2
        p.emplace_back(do_generate_code_for_expression(ctx_io, alt.init));  // 3
        code_out.emplace_back(&do_define_variable, rocket::move(p));
        return;
      }
    case index_function:
      {
        const auto &alt = this->m_stor.as<S_function>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(names_out_opt, ctx_io, "function placeholder", alt.name, Reference_Root::S_uninitialized());
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.sloc);  // 0
        p.emplace_back(alt.name);  // 1
        p.emplace_back(alt.params);  // 2
        p.emplace_back(alt.body);  // 3
        code_out.emplace_back(&do_define_function, rocket::move(p));
        return;
      }
    case index_if:
      {
        const auto &alt = this->m_stor.as<S_if>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(static_cast<std::int64_t>(alt.negative));  // 0
        p.emplace_back(do_generate_code_for_expression(ctx_io, alt.cond));  // 1
        p.emplace_back(do_generate_code_for_block(ctx_io, alt.branch_true));  // 2
        p.emplace_back(do_generate_code_for_block(ctx_io, alt.branch_false));  // 3
        code_out.emplace_back(&do_execute_if, rocket::move(p));
        return;
      }
    case index_switch:
      // TODO
    case index_do_while:
      {
        const auto &alt = this->m_stor.as<S_do_while>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(do_generate_code_for_block(ctx_io, alt.body));  // 0
        p.emplace_back(static_cast<std::int64_t>(alt.negative));  // 1
        p.emplace_back(do_generate_code_for_expression(ctx_io, alt.cond));  // 2
        code_out.emplace_back(&do_execute_do_while, rocket::move(p));
        return;
      }
    case index_while:
      {
        const auto &alt = this->m_stor.as<S_while>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(static_cast<std::int64_t>(alt.negative));  // 0
        p.emplace_back(do_generate_code_for_expression(ctx_io, alt.cond));  // 1
        p.emplace_back(do_generate_code_for_block(ctx_io, alt.body));  // 2
        code_out.emplace_back(&do_execute_while, rocket::move(p));
        return;
      }
    case index_for:
      {
        const auto &alt = this->m_stor.as<S_for>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(do_generate_code_for_block(ctx_io, alt.init));  // 0
        p.emplace_back(do_generate_code_for_expression(ctx_io, alt.cond));  // 1
        p.emplace_back(do_generate_code_for_expression(ctx_io, alt.step));  // 2
        p.emplace_back(do_generate_code_for_statement_list(nullptr, ctx_io, alt.body));  // 3
        code_out.emplace_back(&do_execute_for, rocket::move(p));
        return;
      }
/*
    case index_for_each:
    case index_try:
    case index_break:
    case index_continue:
    case index_throw:
    case index_return:
    case index_assert:
*/
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}





















#if 0
    namespace {


    Air_Node::Status do_execute_statement_list

    Air_Node::Status do_execute_block(Reference_Stack &stack, const Executive_Context &ctx,
                                      const Cow_Vector<Air_Node> &code, const Cow_String &func, const Global_Context &global)
      {
        // Create a fresh context; the stack can be reused safely.
        Executive_Context ctx_next(&ctx);
        // Execute AIR nodes one by one.
        auto status = Air_Node::status_next;
        rocket::any_of(code, [&](const Air_Node &node) { return (status = node.execute(stack, ctx_next, func, global))
                                                                 != Air_Node::status_next;  });
        return status;
      }

    Cow_Vector<Air_Node> do_generate_code_statement_list(Analytic_Context &ctx_io, const Cow_Vector<Statement> &stmts)
      {
        Cow_Vector<Air_Node> code;
        rocket::for_each(stmts, [&](const Statement &stmt) { stmt.generate_code(code, ctx_io);  });
        return code;
      }

    Cow_Vector<Air_Node> do_generate_code_expression_nonempty(const Analytic_Context &ctx, const Cow_Vector<Xpnode> &expr)
      {
        Cow_Vector<Air_Node> code;
        rocket::for_each(expr, [&](const Xpnode &xpn) { xpn.generate_code(code, ctx);  });
        return code;
      }

    Cow_Vector<Air_Node> do_generate_code_block(const Analytic_Context &ctx, const Cow_Vector<Statement> &block)
      {
        Analytic_Context ctx_next(&ctx);
        return do_generate_code_statement_list(ctx_next, block);
      }


    Air_Node::Status do_execute_block(Reference_Stack &stack, Executive_Context &ctx_io,
                                      const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &code_body = p.at(0).as<Cow_Vector<Air_Node>>();
        // Create a fresh context; the stack is reused.
        Executive_Context ctx_body(&ctx_io);
      }

    Air_Node::Status do_define_variable_uninitialized(Reference_Stack & /*stack*/, Executive_Context &ctx_io,
                                                      const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context &global)
      {
        // Decode arguments.
        const auto &name = p.at(0).as<PreHashed_String>();
        const bool immutable = p.at(1).as<std::int64_t>();
        // Create a dummy reference for further name lookups.
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        auto var = global.create_variable();
        Reference_Root::S_variable ref_c = { var };
        do_safe_set_named_reference(ctx_io, "variable", name, rocket::move(ref_c));
        // Initialize the variable to `null`.
        ASTERIA_DEBUG_LOG("New variable `", name, "`: <uninitialized>");
        var->reset(D_null(), immutable);
        return Air_Node::status_next;
      }

    Air_Node::Status do_define_variable_initialized(Reference_Stack &stack, Executive_Context &ctx_io,
                                                    const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &name = p.at(0).as<PreHashed_String>();
        const bool immutable = p.at(1).as<std::int64_t>();
        const auto &code_init = p.at(2).as<Cow_Vector<Air_Node>>();
        // Create a dummy reference for further name lookups.
        // A variable becomes visible before its initializer, where it is initialized to `null`.
        auto var = global.create_variable();
        Reference_Root::S_variable ref_c = { var };
        do_safe_set_named_reference(ctx_io, "variable", name, rocket::move(ref_c));
        // Evaluate the initializer.
        stack.clear();
        ROCKET_ASSERT(!code_init.empty());
        rocket::for_each(code_init, [&](const Air_Node &node) { node.execute(stack, ctx_io, func, global);  });
        auto value = stack.top().read();
        ASTERIA_DEBUG_LOG("New variable `", name, "`: ", value);
        var->reset(rocket::move(value), immutable);
        return Air_Node::status_next;
      }

    Air_Node::Status do_define_function(Reference_Stack & /*stack*/, Executive_Context &ctx_io,
                                        const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context &global)
      {
        // Decode arguments.
        const auto &sloc = p.at(0).as<Source_Location>();
        const auto &name = p.at(1).as<PreHashed_String>();
        const auto &params = p.at(2).as<Cow_Vector<PreHashed_String>>();
        const auto &body = p.at(3).as<Cow_Vector<Statement>>();
        // Create a dummy reference for further name lookups.
        // A function becomes visible before its definition, where it is initialized to `null`.
        auto var = global.create_variable();
        Reference_Root::S_variable ref_c = { var };
        do_safe_set_named_reference(ctx_io, "variable", name, rocket::move(ref_c));
        // Generate code of the function body.
        Cow_Vector<Air_Node> code_next;
        Function_Analytic_Context ctx_next(&ctx_io, params);
        rocket::for_each(body, [&](const Statement &stmt) { stmt.generate_code(code_next, ctx_next);  });
        // Instantiate the function.
        rocket::insertable_ostream nos;
        nos << name << "("
            << rocket::ostream_implode(params.begin(), params.size(), ", ")
            <<")";
        Rcobj<Instantiated_Function> closure(sloc, nos.extract_string(), params, rocket::move(code_next));
        ASTERIA_DEBUG_LOG("New function `", closure, "`: ", closure);
        var->reset(D_function(rocket::move(closure)), true);
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_if(Reference_Stack &stack, Executive_Context &ctx_io,
                                   const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const bool negative = p.at(0).as<std::int64_t>();
        const auto &code_cond = p.at(1).as<Cow_Vector<Air_Node>>();
        const auto &code_true = p.at(2).as<Cow_Vector<Air_Node>>();
        const auto &code_false = p.at(3).as<Cow_Vector<Air_Node>>();
        // Evaluate the condition.
        stack.clear();
        ROCKET_ASSERT(!code_cond.empty());
        rocket::for_each(code_cond, [&](const Air_Node &node) { node.execute(stack, ctx_io, func, global);  });
        if(stack.top().read().test() != negative) {
          // Create a fresh context for the true branch; the stack is reused.
          Executive_Context ctx_true(&ctx_io);
          // Execute AIR nodes one by one.
          auto status = Air_Node::status_next;
          rocket::any_of(code_true, [&](const Air_Node &node) { return (status = node.execute(stack, ctx_true, func, global))
                                                                        != Air_Node::status_next;  });
          return status;
        }
        // Create a fresh context for the false branch; the stack is reused.
        Executive_Context ctx_false(&ctx_io);
        // Execute AIR nodes one by one.
        auto status = Air_Node::status_next;
        rocket::any_of(code_false, [&](const Air_Node &node) { return (status = node.execute(stack, ctx_false, func, global))
                                                                       != Air_Node::status_next;  });
        return status;
      }

    }

void Statement::generate_code(Cow_Vector<Air_Node> &code_out, Analytic_Context &ctx_io) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_expression:
      {
        const auto &alt = this->m_stor.as<S_expression>();
        // Generate code to clear the stack.
        Cow_Vector<Air_Node::Variant> p;
        code_out.emplace_back(&do_clear_stack, rocket::move(p));
        // Generate code for the expression itself.
        rocket::for_each(alt.expr, [&](const Xpnode &xpn) { node.generate_code(code_out, ctx_io);  });
        return;
      }
    case index_block:
      {
        const auto &alt = this->m_stor.as<S_block>();
        // Generate code for the block body.
        Cow_Vector<Air_Node::Variant> p;
        Cow_Vector<Air_Node> code_body;
        Analytic_Context ctx_body(&ctx_io);
        rocket::for_each(alt.body, [&](const Statement &stmt) { stmt.generate_code(code_body, ctx_body);  });
        p.emplace_back(rocket::move(code_body));  // 0
        code_out.emplace_back(&do_execute_block, rocket::move(p));
        return;
      }
    case index_variable:
      {
        const auto &alt = this->m_stor.as<S_variable>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "variable placeholder", alt.name, Reference_Root::S_uninitialized());
        // Generate code for the declaration.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.name);  // 0
        p.emplace_back(static_cast<std::int64_t>(alt.immutable));  // 1
        if(alt.init.empty()) {
          code_out.emplace_back(&do_define_variable_uninitialized, rocket::move(p));
          return;
        }
        // Generate code for the initializer.
        Cow_Vector<Air_Node> code_init;
        rocket::for_each(alt.init, [&](const Xpnode &xpn) { node.generate_code(code_init, ctx_io);  });
        p.emplace_back(rocket::move(code_init));  // 2
        code_out.emplace_back(&do_define_variable_initialized, rocket::move(p));
        return;
      }
    case index_function:
      {
        const auto &alt = this->m_stor.as<S_function>();
        // Create a dummy reference for further name lookups.
        do_safe_set_named_reference(ctx_io, "function placeholder", alt.name, Reference_Root::S_uninitialized());
        // Generate code for the definition.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.sloc);  // 0
        p.emplace_back(alt.name);  // 1
        p.emplace_back(alt.params);  // 2
        p.emplace_back(alt.body);  // 3
        code_out.emplace_back(&do_define_function, rocket::move(p));
        return;
      }
    case index_if:
      {
        const auto &alt = this->m_stor.as<S_if>();
        // Generate code.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(static_cast<std::int64_t>(alt.negative));  // 0
        // Generate code for the condition.
        if(alt.cond.empty()) {
          ASTERIA_THROW_RUNTIME_ERROR("The condition expression of an `if` statement must not be empty.");
        }
        Cow_Vector<Air_Node> code_cond;
        rocket::for_each(alt.cond, [&](const Xpnode &xpn) { node.generate_code(code_cond, ctx_io);  });
        p.emplace_back(rocket::move(code_cond));  // 1
        // Generate code for the true branch.
        Cow_Vector<Air_Node> code_true;
        Analytic_Context ctx_true(&ctx_io);
        rocket::for_each(alt.branch_true, [&](const Statement &stmt) { stmt.generate_code(code_true, ctx_true);  });
        p.emplace_back(rocket::move(code_true));  // 2
        // Generate code for the false branch.
        Cow_Vector<Air_Node> code_false;
        Analytic_Context ctx_false(&ctx_io);
        rocket::for_each(alt.branch_false, [&](const Statement &stmt) { stmt.generate_code(code_false, ctx_false);  });
        p.emplace_back(rocket::move(code_false));  // 3
        code_out.emplace_back(&do_execute_if, rocket::move(p));
        return;
      }
/*
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
*/
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->m_stor.index(), "` has been encountered.");
    }




    Air_Node::Status do_define_function(Reference_Stack &stack, Executive_Context &ctx_io,
                                        const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &alt = do_decapsulate<Statement::S_function>(opaque);
        // Create a dummy reference for further name lookups.
        // A function becomes visible before its initializer, where it is initialized to `null`.
        auto var = global.create_variable();
        Reference_Root::S_variable ref_c = { var };
        do_safe_set_named_reference(ctx_io, "function", alt.name, rocket::move(ref_c));
        // Generate code of the function body.
        Cow_Vector<Air_Node> code_next;
        Function_Analytic_Context ctx_next(&ctx_io, alt.params);
        rocket::for_each(alt.body, [&](const Statement &stmt) { stmt.generate_code(code_next, ctx_next);  });
        // Instantiate the function.
        rocket::insertable_ostream nos;
        nos << alt.name << "("
            << rocket::ostream_implode(alt.params.begin(), alt.params.size(), ", ")
            <<")";
        RefCnt_Object<Instantiated_Function> closure(alt.sloc, nos.extract_string(), alt.params, rocket::move(code_next));
        ASTERIA_DEBUG_LOG("New function: ", closure);
        var->reset(D_function(rocket::move(closure)), true);
        return Air_Node::status_next;
      }


    

    }

void Statement::generate_code(Cow_Vector<Air_Node> &code_out, Analytic_Context &ctx_io) const
  {
  }
#endif


#if 0

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
        do_safe_set_named_reference(ctx_for, "`for each` key", alt.key_name, Reference_Root::S_uninitialized());
        do_safe_set_named_reference(ctx_for, "`for each` reference", alt.mapped_name, Reference_Root::S_uninitialized());
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

#endif
