// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "statement.hpp"
#include "xprunit.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/analytic_context.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    void do_user_declare(cow_vector<phsh_string>* names_opt, Analytic_Context& ctx, const phsh_string& name, const char* desc)
      {
        if(name.rdstr().empty()) {
          ASTERIA_THROW_RUNTIME_ERROR("The name for this ", desc, " must not be empty.");
        }
        if(name.rdstr().starts_with("__")) {
          ASTERIA_THROW_RUNTIME_ERROR("The name `", name, "` for this ", desc, " is reserved and cannot be used.");
        }
        // Record this name.
        if(names_opt) {
          auto oldp = std::find(names_opt->begin(), names_opt->end(), name);
          if(oldp != names_opt->end()) {
            names_opt->erase(oldp);
          }
          names_opt->emplace_back(name);
        }
        // Just ensure the name exists.
        ctx.open_named_reference(name) /*= Reference_Root::S_null()*/;
      }

    cow_vector<AIR_Node>& do_generate_clear_stack(cow_vector<AIR_Node>& code)
      {
        AIR_Node::S_clear_stack xnode = { };
        code.emplace_back(rocket::move(xnode));
        return code;
      }

    cow_vector<AIR_Node>& do_generate_expression_partial(cow_vector<AIR_Node>& code,
                                                         const Compiler_Options& opts, TCO_Aware tco_aware, const Analytic_Context& ctx,
                                                         const cow_vector<Xprunit>& units)
      {
        size_t epos = units.size() - 1;
        if(epos != SIZE_MAX) {
          // Expression units other than the last one cannot be TCO'd.
          for(size_t i = 0; i != epos; ++i) {
            units[i].generate_code(code, opts, tco_aware_none, ctx);
          }
          // The last unit may be TCO'd.
          units[epos].generate_code(code, opts, tco_aware, ctx);
        }
        return code;
      }
    cow_vector<AIR_Node> do_generate_expression_partial(const Compiler_Options& opts, TCO_Aware tco_aware, const Analytic_Context& ctx,
                                                        const cow_vector<Xprunit>& units)
      {
        cow_vector<AIR_Node> code;
        do_generate_expression_partial(code, opts, tco_aware, ctx, units);
        return code;
      }

    cow_vector<AIR_Node>& do_generate_statement_list(cow_vector<AIR_Node>& code, cow_vector<phsh_string>* names_opt, Analytic_Context& ctx,
                                                     const Compiler_Options& opts, TCO_Aware tco_aware, const cow_vector<Statement>& stmts)
      {
        size_t epos = stmts.size() - 1;
        if(epos != SIZE_MAX) {
          // Statements other than the last one cannot be the end of function.
          for(size_t i = 0; i != epos; ++i) {
            stmts[i].generate_code(code, names_opt, ctx, opts, stmts[i+1].is_empty_return() ? tco_aware_nullify : tco_aware_none);
          }
          // The last statement may be TCO'd.
          stmts[epos].generate_code(code, names_opt, ctx, opts, tco_aware);
        }
        return code;
      }
    cow_vector<AIR_Node> do_generate_statement_list(cow_vector<phsh_string>* names_opt, Analytic_Context& ctx,
                                                    const Compiler_Options& opts, TCO_Aware tco_aware, const cow_vector<Statement>& stmts)
      {
        cow_vector<AIR_Node> code;
        do_generate_statement_list(code, names_opt, ctx, opts, tco_aware, stmts);
        return code;
      }

    cow_vector<AIR_Node> do_generate_block(const Compiler_Options& opts, TCO_Aware tco_aware, const Analytic_Context& ctx,
                                           const cow_vector<Statement>& stmts)
      {
        // Create a new context for the block. No new names are injected into `ctx`.
        Analytic_Context ctx_stmts(rocket::ref(ctx));
        auto code = do_generate_statement_list(nullptr, ctx_stmts, opts, tco_aware, stmts);
        return code;
      }

    }

cow_vector<AIR_Node>& Statement::generate_code(cow_vector<AIR_Node>& code, cow_vector<phsh_string>* names_opt,
                                               Analytic_Context& ctx, const Compiler_Options& opts, TCO_Aware tco_aware) const
  {
    switch(this->index()) {
    case index_expression:
      {
        const auto& altr = this->m_stor.as<index_expression>();
        // If the expression is empty, don't bother do anything.
        if(!altr.expr.empty()) {
          // Clear the stack.
          do_generate_clear_stack(code);
          // Evaluate the expression. Its value is discarded.
          do_generate_expression_partial(code, opts, tco_aware, ctx, altr.expr);
        }
        return code;
      }
    case index_block:
      {
        const auto& altr = this->m_stor.as<index_block>();
        // Generate code for the body.
        // This can be TCO'd.
        auto code_body = do_generate_block(opts, tco_aware, ctx, altr.body);
        // Encode arguments.
        AIR_Node::S_execute_block xnode = { rocket::move(code_body) };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_variables:
      {
        const auto& altr = this->m_stor.as<index_variables>();
        // Declare each variable.
        for(const auto& pair : altr.vars) {
          // Create dummy references for further name lookups.
          if(pair.first.size() == 1) {
            // Create a single reference.
            do_user_declare(names_opt, ctx, pair.first[0], "variable placeholder");
          }
          else if((pair.first.at(0) == "[") && (pair.first.back() == "]")) {
            // Create references from left to right.
            for(size_t i = 1; i != pair.first.size() - 1; ++i) {
              do_user_declare(names_opt, ctx, pair.first[i], "array structured binding placeholder");
            }
          }
          else if((pair.first.at(0) == "{") && (pair.first.back() == "}")) {
            // Create references from left to right.
            for(size_t i = 1; i != pair.first.size() - 1; ++i) {
              do_user_declare(names_opt, ctx, pair.first[i], "object structured binding placeholder");
            }
          }
          else {
            ASTERIA_TERMINATE("A malformed variable definition has been encountered. This is likely a bug. Please report.");
          }
          // Declare the variable, when it will be initialized to `null`.
          if(pair.second.empty()) {
            // If no initializer is provided, this can be simplified a little.
            AIR_Node::S_declare_variables xnode_decl = { altr.immutable, pair.first };
            code.emplace_back(rocket::move(xnode_decl));
            continue;
          }
          // Clear the stack.
          do_generate_clear_stack(code);
          // If an initializer is provided, we declare the variable as immutable to prevent
          // unintentional modification before the initialization is completed.
          AIR_Node::S_declare_variables xnode_decl = { true, pair.first };
          code.emplace_back(rocket::move(xnode_decl));
          // Generate code for the initializer.
          do_generate_expression_partial(code, opts, tco_aware_none, ctx, pair.second);
          // Initialize the variable.
          AIR_Node::S_initialize_variables xnode_init = { altr.immutable, pair.first };
          code.emplace_back(rocket::move(xnode_init));
        }
        return code;
      }
    case index_function:
      {
        const auto& altr = this->m_stor.as<index_function>();
        // Create a dummy reference for further name lookups.
        do_user_declare(names_opt, ctx, altr.name, "function placeholder");
        // Create a vector of only one name.
        cow_vector<phsh_string> names;
        names.emplace_back(altr.name);
        // Declare the function, which is effectively an immutable variable.
        AIR_Node::S_declare_variables xnode_decl = { true, names };
        code.emplace_back(rocket::move(xnode_decl));
        // Instantiate the function body.
        AIR_Node::S_define_function xnode_defn = { opts, altr.sloc, altr.name, altr.params, altr.body };
        code.emplace_back(rocket::move(xnode_defn));
        // Initialize the function.
        AIR_Node::S_initialize_variables xnode_init = { true, rocket::move(names) };
        code.emplace_back(rocket::move(xnode_init));
        return code;
      }
    case index_if:
      {
        const auto& altr = this->m_stor.as<index_if>();
        // Clear the stack.
        do_generate_clear_stack(code);
        // Generate code for the condition.
        ROCKET_ASSERT(!altr.cond.empty());
        do_generate_expression_partial(code, opts, tco_aware_none, ctx, altr.cond);
        // The result will have been pushed onto the top of the stack.
        // Generate code for both branches.
        // Both can be TCO'd.
        auto code_true = do_generate_block(opts, tco_aware, ctx, altr.branch_true);
        auto code_false = do_generate_block(opts, tco_aware, ctx, altr.branch_false);
        // Encode arguments.
        AIR_Node::S_if_statement xnode = { altr.negative, rocket::move(code_true), rocket::move(code_false) };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_switch:
      {
        const auto& altr = this->m_stor.as<index_switch>();
        // Clear the stack.
        do_generate_clear_stack(code);
        // Generate code for the control expression.
        ROCKET_ASSERT(!altr.ctrl.empty());
        do_generate_expression_partial(code, opts, tco_aware_none, ctx, altr.ctrl);
        // Create a fresh context for the `switch` body.
        // Be advised that all clauses inside a `switch` statement share the same context.
        Analytic_Context ctx_body(rocket::ref(ctx));
        cow_vector<phsh_string> names;
        // Generate code for all clauses.
        cow_vector<cow_vector<AIR_Node>> code_labels;
        cow_vector<cow_vector<AIR_Node>> code_bodies;
        cow_vector<cow_vector<phsh_string>> names_added;
        for(const auto& p : altr.clauses) {
          // Generate code for the label.
          do_generate_expression_partial(code_labels.emplace_back(), opts, tco_aware_none, ctx_body, p.first);
          // Generate code for the clause and accumulate names.
          // This cannot be TCO'd.
          do_generate_statement_list(code_bodies.emplace_back(), std::addressof(names), ctx_body, opts, tco_aware_none, p.second);
          names_added.emplace_back(names);
        }
        // Encode arguments.
        AIR_Node::S_switch_statement xnode = { rocket::move(code_labels), rocket::move(code_bodies), rocket::move(names_added) };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_do_while:
      {
        const auto& altr = this->m_stor.as<index_do_while>();
        // Generate code for the body.
        // Loop statements cannot be TCO'd.
        auto code_body = do_generate_block(opts, tco_aware_none, ctx, altr.body);
        // Generate code for the condition.
        ROCKET_ASSERT(!altr.cond.empty());
        auto code_cond = do_generate_expression_partial(opts, tco_aware_none, ctx, altr.cond);
        // Encode arguments.
        AIR_Node::S_do_while_statement xnode = { rocket::move(code_body), altr.negative, rocket::move(code_cond) };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_while:
      {
        const auto& altr = this->m_stor.as<index_while>();
        // Generate code for the condition.
        ROCKET_ASSERT(!altr.cond.empty());
        auto code_cond = do_generate_expression_partial(opts, tco_aware_none, ctx, altr.cond);
        // Generate code for the body.
        // Loop statements cannot be TCO'd.
        auto code_body = do_generate_block(opts, tco_aware_none, ctx, altr.body);
        // Encode arguments.
        AIR_Node::S_while_statement xnode = { altr.negative, rocket::move(code_cond), rocket::move(code_body) };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_for_each:
      {
        const auto& altr = this->m_stor.as<index_for_each>();
        // Note that the key and value references outlasts every iteration, so we have to create an outer contexts here.
        Analytic_Context ctx_for(rocket::ref(ctx));
        do_user_declare(names_opt, ctx_for, altr.name_key, "key placeholder");
        do_user_declare(names_opt, ctx_for, altr.name_mapped, "value placeholder");
        // Generate code for the range initializer.
        ROCKET_ASSERT(!altr.init.empty());
        auto code_init = do_generate_expression_partial(opts, tco_aware_none, ctx_for, altr.init);
        // Generate code for the body.
        // Loop statements cannot be TCO'd.
        auto code_body = do_generate_block(opts, tco_aware_none, ctx_for, altr.body);
        // Encode arguments.
        AIR_Node::S_for_each_statement xnode = { altr.name_key, altr.name_mapped, rocket::move(code_init), rocket::move(code_body) };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_for:
      {
        const auto& altr = this->m_stor.as<index_for>();
        // Note that names declared in the first segment of a for-statement outlasts every iteration, so we have to create an outer contexts here.
        Analytic_Context ctx_for(rocket::ref(ctx));
        // Generate code for the initializer, the condition and the loop increment.
        auto code_init = do_generate_statement_list(nullptr, ctx_for, opts, tco_aware_none, altr.init);
        auto code_cond = do_generate_expression_partial(opts, tco_aware_none, ctx_for, altr.cond);
        auto code_step = do_generate_expression_partial(opts, tco_aware_none, ctx_for, altr.step);
        // Generate code for the body.
        // Loop statements cannot be TCO'd.
        auto code_body = do_generate_block(opts, tco_aware_none, ctx_for, altr.body);
        // Encode arguments.
        AIR_Node::S_for_statement xnode = { rocket::move(code_init), rocket::move(code_cond), rocket::move(code_step), rocket::move(code_body) };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_try:
      {
        const auto& altr = this->m_stor.as<index_try>();
        // Generate code for the `try` body.
        // This cannot be TCO'd, otherwise exceptions thrown from tail calls won't be caught.
        auto code_try = do_generate_block(opts, tco_aware_none, ctx, altr.body_try);
        // Create a fresh context for the `catch` clause.
        Analytic_Context ctx_catch(rocket::ref(ctx));
        do_user_declare(names_opt, ctx_catch, altr.name_except, "exception placeholder");
        ctx_catch.open_named_reference(rocket::sref("__backtrace"));
        // Generate code for the `catch` body.
        // Unlike the `try` body, this may be TCO'd.
        auto code_catch = do_generate_statement_list(nullptr, ctx_catch, opts, tco_aware, altr.body_catch);
        // Encode arguments.
        AIR_Node::S_try_statement xnode = { rocket::move(code_try), altr.sloc, altr.name_except, rocket::move(code_catch) };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_break:
      {
        const auto& altr = this->m_stor.as<index_break>();
        // Translate jump targets to AIR status codes.
        switch(altr.target) {
        case jump_target_unspec:
          {
            AIR_Node::S_simple_status xnode = { air_status_break_unspec };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case jump_target_switch:
          {
            AIR_Node::S_simple_status xnode = { air_status_break_switch };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case jump_target_while:
          {
            AIR_Node::S_simple_status xnode = { air_status_break_while };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case jump_target_for:
          {
            AIR_Node::S_simple_status xnode = { air_status_break_for };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        default:
          ASTERIA_TERMINATE("An unknown target scope type `", altr.target, "` has been encountered. This is likely a bug. Please report.");
        }
      }
    case index_continue:
      {
        const auto& altr = this->m_stor.as<index_continue>();
        // Translate jump targets to AIR status codes.
        switch(altr.target) {
        case jump_target_unspec:
          {
            AIR_Node::S_simple_status xnode = { air_status_continue_unspec };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case jump_target_switch:
          {
            ASTERIA_TERMINATE("`target_switch` is not allowed to follow `continue`.");
          }
        case jump_target_while:
          {
            AIR_Node::S_simple_status xnode = { air_status_continue_while };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case jump_target_for:
          {
            AIR_Node::S_simple_status xnode = { air_status_continue_for };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        default:
          ASTERIA_TERMINATE("An unknown target scope type `", altr.target, "` has been encountered. This is likely a bug. Please report.");
        }
      }
    case index_throw:
      {
        const auto& altr = this->m_stor.as<index_throw>();
        // Generate code for the operand.
        ROCKET_ASSERT(!altr.expr.empty());
        do_generate_expression_partial(code, opts, tco_aware_none, ctx, altr.expr);
        // Encode arguments.
        AIR_Node::S_throw_statement xnode = { altr.sloc };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_return:
      {
        const auto& altr = this->m_stor.as<index_return>();
        // Clear the stack.
        do_generate_clear_stack(code);
        // We don't tell empty return statements from non-empty ones here.
        if(altr.expr.empty()) {
          // If no expression is provided, return `null`.
          AIR_Node::S_push_literal xnode_def = { G_null() };
          code.emplace_back(rocket::move(xnode_def));
          // Forward the result as is.
          AIR_Node::S_simple_status xnode_ret = { air_status_return };
          code.emplace_back(rocket::move(xnode_ret));
        }
        else if(altr.by_ref) {
          // Generate code for the operand.
          // This may be TCO'd by reference.
          do_generate_expression_partial(code, opts, tco_aware_by_ref, ctx, altr.expr);
          // Forward the result as is.
          AIR_Node::S_simple_status xnode_ret = { air_status_return };
          code.emplace_back(rocket::move(xnode_ret));
        }
        else {
          // Generate code for the operand.
          // This may be TCO'd by value.
          do_generate_expression_partial(code, opts, tco_aware_by_val, ctx, altr.expr);
          // Convert the result to an rvalue and return it.
          AIR_Node::S_return_by_value xnode_ret = { };
          code.emplace_back(rocket::move(xnode_ret));
        }
        return code;
      }
    case index_assert:
      {
        const auto& altr = this->m_stor.as<index_assert>();
        // Generate code for the operand.
        ROCKET_ASSERT(!altr.expr.empty());
        do_generate_expression_partial(code, opts, tco_aware_none, ctx, altr.expr);
        // Encode arguments.
        AIR_Node::S_assert_statement xnode = { altr.sloc, altr.negative, altr.msg };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
