// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "statement.hpp"
#include "expression_unit.hpp"
#include "enums.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/air_optimizer.hpp"
#include "../runtime/enums.hpp"
#include "../utils.hpp"
namespace asteria {
namespace {

void
do_user_declare(Analytic_Context& ctx, cow_vector<phsh_string>* names_opt, phsh_stringR name)
  {
    if(name.empty())
      return;

    // Inject this name.
    if(names_opt && !find(*names_opt, name))
      names_opt->emplace_back(name);

    ctx.insert_named_reference(name);
  }

void
do_generate_clear_stack(cow_vector<AIR_Node>& code)
  {
    AIR_Node::S_clear_stack xnode = { };
    code.emplace_back(move(xnode));
  }

void
do_generate_subexpression(cow_vector<AIR_Node>& code, const Compiler_Options& opts,
                          const Global_Context& global, const Analytic_Context& ctx,
                          PTC_Aware ptc, const Statement::S_expression& expr)
  {
    if(opts.verbose_single_step_traps) {
      // Generate a single-step trap if it is requested. This happens at translation
      // time, so if it's not requested at all, there will be no runtime overhead.
      AIR_Node::S_single_step_trap xnode = { expr.sloc };
      code.emplace_back(move(xnode));
    }

    if(expr.units.empty())
      return;

    // Generate code for the subexpression itself.
    for(size_t i = 0;  i < expr.units.size();  ++i)
      expr.units.at(i).generate_code(code, opts, global, ctx,
                          (i != expr.units.size() - 1) ? ptc_aware_none : ptc);
  }

void
do_generate_expression(cow_vector<AIR_Node>& code, const Compiler_Options& opts,
                       const Global_Context& global, const Analytic_Context& ctx,
                       PTC_Aware ptc, const Statement::S_expression& expr)
  {
    do_generate_clear_stack(code);
    do_generate_subexpression(code, opts, global, ctx, ptc, expr);
  }

cow_vector<AIR_Node>
do_generate_expression(const Compiler_Options& opts, const Global_Context& global,
                       Analytic_Context& ctx, PTC_Aware ptc,
                       const Statement::S_expression& expr)
  {
    cow_vector<AIR_Node> code;
    do_generate_expression(code, opts, global, ctx, ptc, expr);
    return code;
  }

void
do_generate_statement_list(cow_vector<AIR_Node>& code, Analytic_Context& ctx,
                           cow_vector<phsh_string>* names_opt, const Global_Context& global,
                           const Compiler_Options& opts, PTC_Aware ptc,
                           const cow_vector<Statement>& stmts)
  {
    for(size_t i = 0;  i < stmts.size();  ++i)
      stmts.at(i).generate_code(code, ctx, names_opt, global, opts,
                           (i != stmts.size() - 1)
                             ? (stmts.at(i + 1).is_empty_return() ? ptc_aware_void : ptc_aware_none)
                             : ptc);
  }

cow_vector<AIR_Node>
do_generate_statement_list(Analytic_Context& ctx, cow_vector<phsh_string>* names_opt,
                           const Global_Context& global, const Compiler_Options& opts,
                           PTC_Aware ptc, const cow_vector<Statement>& stmts)
  {
    cow_vector<AIR_Node> code;
    do_generate_statement_list(code, ctx, names_opt, global, opts, ptc, stmts);
    return code;
  }

cow_vector<AIR_Node>
do_generate_block(const Compiler_Options& opts, const Global_Context& global,
                  const Analytic_Context& ctx, PTC_Aware ptc, const Statement::S_block& block)
  {
    cow_vector<AIR_Node> code;
    Analytic_Context ctx_stmts(xtc_plain, ctx);
    do_generate_statement_list(code, ctx_stmts, nullptr, global, opts, ptc, block.stmts);
    return code;
  }

}  // namespace

void
Statement::
generate_code(cow_vector<AIR_Node>& code, Analytic_Context& ctx,
              cow_vector<phsh_string>* names_opt, const Global_Context& global,
              const Compiler_Options& opts, PTC_Aware ptc) const
  {
    if(!code.empty() && code.back().is_terminator())
      return;

    switch(static_cast<Index>(this->m_stor.index())) {
      case index_expression: {
        const auto& altr = this->m_stor.as<S_expression>();

        // Evaluate the expression. Its value is discarded.
        do_generate_expression(code, opts, global, ctx, ptc, altr);
        return;
      }

      case index_block: {
        const auto& altr = this->m_stor.as<S_block>();

        // https://github.com/lhmouse/asteria/issues/102
        // First, unblockify Matryoshka blocks. If the innermost block contains
        // neither declarations nor `defer` statements, unblockify it, too.
        auto qblock = &altr;
        while((qblock->stmts.size() == 1) && (qblock->stmts.at(0).m_stor.index() == index_block))
          qblock = &(qblock->stmts.at(0).m_stor.as<S_block>());

        if(qblock->stmts.empty())
          return;

        if(::rocket::all_of(altr.stmts, [](const Statement& st) { return st.is_scopeless();  })) {
          do_generate_statement_list(code, ctx, nullptr, global, opts, ptc, qblock->stmts);
          return;
        }

        // Generate code for the body. This can be PTC'd.
        auto code_body = do_generate_block(opts, global, ctx, ptc, altr);

        AIR_Node::S_execute_block xnode = { move(code_body) };
        code.emplace_back(move(xnode));
        return;
      }

      case index_variables: {
        const auto& altr = this->m_stor.as<S_variables>();

        for(const auto& decl : altr.decls) {
          if(decl.names.size() == 1) {
            // Declare a scalar variable.
            do_user_declare(ctx, names_opt, decl.names.at(0));

            if(decl.init.units.empty()) {
              // Declare a variable with default initialization.
              AIR_Node::S_define_null_variable xnode_decl = { decl.sloc, altr.immutable,
                                                              decl.names.at(0) };
              code.emplace_back(move(xnode_decl));
            }
            else {
              // Evaluate the initializer.
              do_generate_clear_stack(code);

              AIR_Node::S_declare_variable xnode_decl = { decl.sloc, decl.names.at(0) };
              code.emplace_back(move(xnode_decl));

              // Generate code for the initializer.
              do_generate_subexpression(code, opts, global, ctx, ptc_aware_none, decl.init);

              // Initialize the variable.
              AIR_Node::S_initialize_variable xnode_init = { decl.sloc, altr.immutable };
              code.emplace_back(move(xnode_init));
            }
          }
          else if((decl.names.size() >= 2) && (decl.names.at(0) == "[") && (decl.names.back() == "]")) {
            // Declare a structured binding for array.
            for(uint32_t k = 1;  k != decl.names.size() - 1;  ++k)
              do_user_declare(ctx, names_opt, decl.names.at(k));

            if(decl.init.units.empty()) {
              // Declare variables with default initialization.
              for(uint32_t k = 1;  k != decl.names.size() - 1;  ++k) {
                AIR_Node::S_define_null_variable xnode_decl = { decl.sloc, altr.immutable,
                                                                decl.names.at(k) };
                code.emplace_back(move(xnode_decl));
              }
            }
            else {
              // Evaluate the initializer.
              do_generate_clear_stack(code);

              for(uint32_t k = 1;  k != decl.names.size() - 1;  ++k) {
                AIR_Node::S_declare_variable xnode_decl = { decl.sloc, decl.names.at(k) };
                code.emplace_back(move(xnode_decl));
              }

              // Generate code for the initializer.
              do_generate_subexpression(code, opts, global, ctx, ptc_aware_none, decl.init);

              // Initialize variables.
              AIR_Node::S_unpack_array xnode_init = { decl.sloc, altr.immutable,
                                                      (uint32_t) (decl.names.size() - 2) };
              code.emplace_back(move(xnode_init));
            }
          }
          else if((decl.names.size() >= 2) && (decl.names.at(0) == "{") && (decl.names.back() == "}")) {
            // Declare a structured binding for object.
            for(uint32_t k = 1;  k != decl.names.size() - 1;  ++k)
              do_user_declare(ctx, names_opt, decl.names.at(k));

            if(decl.init.units.empty()) {
              // Declare variables with default initialization.
              for(uint32_t k = 1;  k != decl.names.size() - 1;  ++k) {
                AIR_Node::S_define_null_variable xnode_decl = { decl.sloc, altr.immutable, decl.names.at(k) };
                code.emplace_back(move(xnode_decl));
              }
            }
            else {
              // Evaluate the initializer.
              do_generate_clear_stack(code);

              for(uint32_t k = 1;  k != decl.names.size() - 1;  ++k) {
                AIR_Node::S_declare_variable xnode_decl = { decl.sloc, decl.names.at(k) };
                code.emplace_back(move(xnode_decl));
              }

              // Generate code for the initializer.
              do_generate_subexpression(code, opts, global, ctx, ptc_aware_none, decl.init);

              // Initialize variables.
              AIR_Node::S_unpack_object xnode_init = { decl.sloc, altr.immutable,
                                                       decl.names.subvec(1, decl.names.size() - 2) };
              code.emplace_back(move(xnode_init));
            }
          }
          else
            ASTERIA_TERMINATE(("Corrupted variable declaration at '$1'"), decl.sloc);
        }
        return;
      }

      case index_function: {
        const auto& altr = this->m_stor.as<S_function>();

        // Create a dummy reference for further name lookups.
        do_user_declare(ctx, names_opt, altr.name);

        // Declare the function, which is effectively an immutable variable.
        AIR_Node::S_declare_variable xnode_decl = { altr.sloc, altr.name };
        code.emplace_back(move(xnode_decl));

        // Generate code
        AIR_Optimizer optmz(opts);
        optmz.reload(&ctx, altr.params, global, altr.body);

        AIR_Node::S_define_function xnode_defn = { opts, altr.sloc, altr.name, altr.params,
                                                   optmz.get_code() };
        code.emplace_back(move(xnode_defn));

        // Initialize the function.
        AIR_Node::S_initialize_variable xnode = { altr.sloc, true };
        code.emplace_back(move(xnode));
        return;
      }

      case index_if: {
        const auto& altr = this->m_stor.as<S_if>();

        // Generate code for the condition.
        ROCKET_ASSERT(!altr.cond.units.empty());
        do_generate_expression(code, opts, global, ctx, ptc_aware_none, altr.cond);

        // The result will have been pushed onto the top of the stack.
        // Generate code for both branches. Both can be PTC'd.
        auto code_true = do_generate_block(opts, global, ctx, ptc, altr.branch_true);
        auto code_false = do_generate_block(opts, global, ctx, ptc, altr.branch_false);

        AIR_Node::S_if_statement xnode = { altr.negative, move(code_true), move(code_false) };
        code.emplace_back(move(xnode));
        return;
      }

      case index_switch: {
        const auto& altr = this->m_stor.as<S_switch>();

        // Generate code for the control expression.
        ROCKET_ASSERT(!altr.ctrl.units.empty());
        do_generate_expression(code, opts, global, ctx, ptc_aware_none, altr.ctrl);

        // Create a fresh context for the `switch` body.
        // Be advised that all clauses inside a `switch` statement share the same context.
        Analytic_Context ctx_body(xtc_plain, ctx);
        cow_vector<AIR_Node::switch_clause> clauses;
        for(const auto& clause : altr.clauses) {
          auto& r = clauses.emplace_back();

          // Generate code for `case` labels, or create empty code for the `default` label.
          // Note labels are not part of the body.
          if(!clause.label.units.empty())
            do_generate_expression(r.code_label, opts, global, ctx, ptc_aware_none, clause.label);

          // Generate code for the clause and accumulate names.
          do_generate_statement_list(r.code_body, ctx_body, &(r.names_added), global, opts,
                                     ptc_aware_none, clause.body);
        }

        AIR_Node::S_switch_statement xnode = { move(clauses) };
        code.emplace_back(move(xnode));
        return;
      }

      case index_do_while: {
        const auto& altr = this->m_stor.as<S_do_while>();

        // Generate code for the body.
        // Loop statements cannot be PTC'd.
        auto code_body = do_generate_block(opts, global, ctx, ptc_aware_none, altr.body);

        // Generate code for the condition.
        ROCKET_ASSERT(!altr.cond.units.empty());
        auto code_cond = do_generate_expression(opts, global, ctx, ptc_aware_none, altr.cond);

        AIR_Node::S_do_while_statement xnode = { move(code_body), altr.negative, move(code_cond) };
        code.emplace_back(move(xnode));
        return;
      }

      case index_while: {
        const auto& altr = this->m_stor.as<S_while>();

        // Generate code for the condition.
        ROCKET_ASSERT(!altr.cond.units.empty());
        auto code_cond = do_generate_expression(opts, global, ctx, ptc_aware_none, altr.cond);

        // Generate code for the body.
        // Loop statements cannot be PTC'd.
        auto code_body = do_generate_block(opts, global, ctx, ptc_aware_none, altr.body);

        AIR_Node::S_while_statement xnode = { altr.negative, move(code_cond), move(code_body) };
        code.emplace_back(move(xnode));
        return;
      }

      case index_for_each: {
        const auto& altr = this->m_stor.as<S_for_each>();

        // Note that the key and value references outlasts every iteration, so we
        // have to create an outer contexts here.
        Analytic_Context ctx_for(xtc_plain, ctx);
        do_user_declare(ctx_for, names_opt, altr.name_key);
        do_user_declare(ctx_for, names_opt, altr.name_mapped);

        // Generate code for the range initializer.
        ROCKET_ASSERT(!altr.init.units.empty());
        auto code_init = do_generate_expression(opts, global, ctx_for, ptc_aware_none, altr.init);

        // Generate code for the body.
        // Loop statements cannot be PTC'd.
        auto code_body = do_generate_block(opts, global, ctx_for, ptc_aware_none, altr.body);

        AIR_Node::S_for_each_statement xnode = { altr.name_key, altr.name_mapped, altr.sloc_init,
                                                 move(code_init), move(code_body) };
        code.emplace_back(move(xnode));
        return;
      }

      case index_for: {
        const auto& altr = this->m_stor.as<S_for>();

        // Note that names declared in the first segment of a for-statement outlasts
        // every iteration, so we have to create an outer contexts here.
        Analytic_Context ctx_for(xtc_plain, ctx);

        // Generate code for the initializer, the condition and the loop increment.
        auto code_init = do_generate_statement_list(ctx_for, nullptr, global, opts, ptc_aware_none, altr.init);
        auto code_cond = do_generate_expression(opts, global, ctx_for, ptc_aware_none, altr.cond);
        auto code_step = do_generate_expression(opts, global, ctx_for, ptc_aware_none, altr.step);

        // Generate code for the body.
        // Loop statements cannot be PTC'd.
        auto code_body = do_generate_block(opts, global, ctx_for, ptc_aware_none, altr.body);

        AIR_Node::S_for_statement xnode = { move(code_init), move(code_cond), move(code_step),
                                            move(code_body) };
        code.emplace_back(move(xnode));
        return;
      }

      case index_try: {
        const auto& altr = this->m_stor.as<S_try>();

        // Generate code for the `try` body.
        auto code_try = do_generate_block(opts, global, ctx, ptc_aware_none, altr.body_try);

        // Create a fresh context for the `catch` clause.
        Analytic_Context ctx_catch(xtc_plain, ctx);
        do_user_declare(ctx_catch, names_opt, altr.name_except);
        ctx_catch.insert_named_reference(sref("__backtrace"));

        // Generate code for the `catch` body.
        // Unlike the `try` body, this may be PTC'd.
        auto code_catch = do_generate_statement_list(ctx_catch, nullptr, global, opts, ptc,
                                                     altr.body_catch.stmts);

        AIR_Node::S_try_statement xnode = { altr.sloc_try, move(code_try), altr.sloc_catch,
                                            altr.name_except, move(code_catch) };
        code.emplace_back(move(xnode));
        return;
      }

      case index_break: {
        const auto& altr = this->m_stor.as<S_break>();

        // Translate jump targets to AIR status codes.
        switch(altr.target) {
          case jump_target_unspec: {
            AIR_Node::S_simple_status xnode = { air_status_break_unspec };
            code.emplace_back(move(xnode));
            return;
          }

          case jump_target_switch: {
            AIR_Node::S_simple_status xnode = { air_status_break_switch };
            code.emplace_back(move(xnode));
            return;
          }

          case jump_target_while: {
            AIR_Node::S_simple_status xnode = { air_status_break_while };
            code.emplace_back(move(xnode));
            return;
          }

          case jump_target_for: {
            AIR_Node::S_simple_status xnode = { air_status_break_for };
            code.emplace_back(move(xnode));
            return;
          }

          default:
            ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), altr.target);
        }
      }

      case index_continue: {
        const auto& altr = this->m_stor.as<S_continue>();

        // Translate jump targets to AIR status codes.
        switch(altr.target) {
          case jump_target_unspec: {
            AIR_Node::S_simple_status xnode = { air_status_continue_unspec };
            code.emplace_back(move(xnode));
            return;
          }

          case jump_target_switch:
            ASTERIA_TERMINATE(("`target_switch` not allowed to follow `continue`"));

          case jump_target_while: {
            AIR_Node::S_simple_status xnode = { air_status_continue_while };
            code.emplace_back(move(xnode));
            return;
          }

          case jump_target_for: {
            AIR_Node::S_simple_status xnode = { air_status_continue_for };
            code.emplace_back(move(xnode));
            return;
          }

          default:
            ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), altr.target);
        }
      }

      case index_throw: {
        const auto& altr = this->m_stor.as<S_throw>();

        // Generate code for the operand.
        ROCKET_ASSERT(!altr.expr.units.empty());
        do_generate_expression(code, opts, global, ctx, ptc_aware_none, altr.expr);

        AIR_Node::S_throw_statement xnode = { altr.sloc };
        code.emplace_back(move(xnode));
        return;
      }

      case index_return: {
        const auto& altr = this->m_stor.as<S_return>();

        if(altr.expr.units.empty()) {
          // Return void.
          AIR_Node::S_return_statement xnode = { altr.sloc, false, true };
          code.emplace_back(move(xnode));
          return;
        }

        // Generate code for the operand.
        do_generate_expression(code, opts, global, ctx,
                               altr.by_ref ? ptc_aware_by_ref : ptc_aware_by_val,
                               altr.expr);

        AIR_Node::S_return_statement xnode = { altr.sloc, altr.by_ref, false };
        code.emplace_back(move(xnode));
        return;
      }

      case index_assert: {
        const auto& altr = this->m_stor.as<S_assert>();

        // Generate code for the operand.
        ROCKET_ASSERT(!altr.expr.units.empty());
        do_generate_expression(code, opts, global, ctx, ptc_aware_none, altr.expr);

        AIR_Node::S_assert_statement xnode = { altr.sloc, altr.msg };
        code.emplace_back(move(xnode));
        return;
      }

      case index_defer: {
        const auto& altr = this->m_stor.as<S_defer>();

        // Generate code for the operand.
        ROCKET_ASSERT(!altr.expr.units.empty());
        auto code_body = do_generate_expression(opts, global, ctx, ptc_aware_none, altr.expr);

        AIR_Node::S_defer_expression xnode = { altr.sloc, move(code_body) };
        code.emplace_back(move(xnode));
        return;
      }

      case index_references: {
        const auto& altr = this->m_stor.as<S_references>();

        for(const auto& decl : altr.decls) {
          // Structured bindings are not allowed.
          do_user_declare(ctx, names_opt, decl.name);

          // Evaluate the initializer, which is required.
          do_generate_clear_stack(code);

          AIR_Node::S_declare_reference xnode_decl = { decl.name };
          code.emplace_back(move(xnode_decl));

          // Generate code for the initializer.
          do_generate_subexpression(code, opts, global, ctx, ptc_aware_none, decl.init);

          // Initialize the reference.
          AIR_Node::S_initialize_reference xnode_init = { decl.sloc, decl.name };
          code.emplace_back(move(xnode_init));
        }
        return;
      }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

}  // namespace asteria
