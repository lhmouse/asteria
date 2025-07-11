// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "air_node.hpp"
#include "enums.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "analytic_context.hpp"
#include "garbage_collector.hpp"
#include "random_engine.hpp"
#include "runtime_error.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "module_loader.hpp"
#include "air_optimizer.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../compiler/statement.hpp"
#include "../compiler/expression_unit.hpp"
#include "../llds/avm_rod.hpp"
#include "../utils.hpp"
namespace asteria {
namespace {

void
do_set_rebound(bool& dirty, AIR_Node& res, AIR_Node&& bound)
  {
    dirty = true;
    res = move(bound);
  }

void
do_rebind_nodes(bool& dirty, cow_vector<AIR_Node>& code, const Abstract_Context& ctx)
  {
    for(size_t i = 0;  i < code.size();  ++i)
      if(auto qnode = code.at(i).rebind_opt(ctx))
        do_set_rebound(dirty, code.mut(i), move(*qnode));
  }

template<typename xNode>
opt<AIR_Node>
do_return_rebound_opt(bool dirty, xNode&& bound)
  {
    opt<AIR_Node> res;
    if(dirty)
      res.emplace(forward<xNode>(bound));
    return res;
  }

void
do_collect_variables_for_each(Variable_HashMap& staged, Variable_HashMap& temp,
                              const cow_vector<AIR_Node>& code)
  {
    for(size_t i = 0;  i < code.size();  ++i)
      code.at(i).collect_variables(staged, temp);
  }

void
do_solidify_nodes(AVM_Rod& rod, const cow_vector<AIR_Node>& code)
  {
    rod.clear();
    for(size_t i = 0;  i < code.size();  ++i)
      code.at(i).solidify(rod);
    rod.finalize();
  }

template<typename xSubscript>
void
do_push_subscript_and_check(Reference& ref, xSubscript&& xsub)
  {
    ref.push_subscript(forward<xSubscript>(xsub));
    ref.dereference_readonly();
  }

template<typename xSparam>
void
do_sparam_ctor(AVM_Rod::Header* head, void* arg)
  {
    auto sp = reinterpret_cast<xSparam*>(head->sparam);
    auto src = static_cast<xSparam*>(arg);
    ::rocket::construct(sp, move(*src));
  }

template<typename xSparam>
void
do_sparam_dtor(AVM_Rod::Header* head)
  {
    auto sp = reinterpret_cast<xSparam*>(head->sparam);
    ::rocket::destroy(sp);
  }

void
do_execute_block(const AVM_Rod& rod, const Executive_Context& ctx)
  {
    Executive_Context ctx_next(xtc_plain, ctx);
    try {
      rod.execute(ctx_next);
    }
    catch(Runtime_Error& except) {
      ctx_next.on_scope_exit_exceptional(except);
      throw;
    }
    ctx_next.on_scope_exit_normal();
  }

void
do_evaluate_subexpression(Executive_Context& ctx, bool assign, const AVM_Rod& rod)
  {
    if(rod.empty()) {
      // If the rod is empty, leave the condition on the top of the stack.
      ROCKET_ASSERT(ctx.status() == air_status_next);
    }
    else if(assign) {
      // Evaluate the subexpression and assign the result to the first operand.
      rod.execute(ctx);
      ROCKET_ASSERT(ctx.status() == air_status_next);

      // The result really has to be copied, in case that a reference to an
      // element of the LHS operand itself is returned.
      Value& rhs = ctx.stack().mut_top().dereference_copy();
      ctx.stack().top(1).dereference_mutable() = move(rhs);
      ctx.stack().pop();
    }
    else {
      // Discard the top which will be overwritten anyway.
      ctx.stack().pop();

      // Evaluate the subexpression. The status code must be forwarded, because
      // a proper tail call may return `air_status_return_ref`.
      rod.execute(ctx);
    }
  }

cow_function
do_get_target_function(const Reference& top)
  {
    const auto& value = top.dereference_readonly();
    if(value.is_null())
      throw Runtime_Error(xtc_format, "Target function not found");

    if(!value.is_function())
      throw Runtime_Error(xtc_format,
               "Non-function value not invocable (target `$1`)", value);

    return value.as_function();
  }

void
do_invoke_partial(Reference& self, Executive_Context& ctx, const Source_Location& sloc,
                  PTC_Aware ptc, const cow_function& target)
  {
    ctx.global().call_hook(&Abstract_Hooks::on_trap, sloc, ctx);

    if(ROCKET_EXPECT(ptc == ptc_aware_none)) {
      // Perform a plain call.
      ctx.global().call_hook(&Abstract_Hooks::on_call, sloc, target);
      target.invoke(self, ctx.global(), move(ctx.alt_stack()));
      ROCKET_ASSERT(ctx.status() == air_status_next);
    }
    else {
      // Perform a tail call.
      self.set_ptc(::rocket::make_refcnt<PTC_Arguments>(sloc, ptc,
                          target, move(self), move(ctx.alt_stack())));
      ctx.status() = air_status_return;
    }
  }

template<typename xContainer>
xContainer
do_duplicate_sequence(const xContainer& src, int64_t count)
  {
    xContainer dst;
    int64_t rlen;

    if(count < 0)
      throw Runtime_Error(xtc_format,
               "Negative duplication count (value was `$2`)", count);

    if(ROCKET_MUL_OVERFLOW(src.ssize(), count, &rlen) || (rlen > PTRDIFF_MAX))
      throw Runtime_Error(xtc_format,
               "Length overflow (`$1` * `$2` > `$3`)", src.size(), count, PTRDIFF_MAX);

    if(ROCKET_UNEXPECT(rlen == 0))
      return dst;

    size_t ntotal = static_cast<size_t>(rlen);
    dst.reserve(ntotal);
    dst.append(src.begin(), src.end());
    while(dst.size() * 2 <= ntotal)
      dst.append(dst.begin(), dst.end());
    dst.append(dst.begin(), dst.begin() + static_cast<ptrdiff_t>(ntotal - dst.size()));
    return dst;
  }

}  // namespace

opt<Value>
AIR_Node::
get_constant_opt() const noexcept
  {
    switch(this->m_stor.index())
      {
      case index_push_bound_reference:
        try {
          const auto& altr = this->m_stor.as<S_push_bound_reference>();
          if(altr.ref.is_temporary())
            return altr.ref.dereference_readonly();
        }
        catch(...) { }
        return nullopt;

      case index_push_constant:
        return this->m_stor.as<S_push_constant>().val;

      default:
        return nullopt;
      }
  }

bool
AIR_Node::
is_terminator() const noexcept
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_clear_stack:
      case index_declare_variable:
      case index_initialize_variable:
      case index_switch_statement:
      case index_do_while_statement:
      case index_while_statement:
      case index_for_each_statement:
      case index_for_statement:
      case index_try_statement:
      case index_assert_statement:
      case index_check_argument:
      case index_push_global_reference:
      case index_push_local_reference:
      case index_push_bound_reference:
      case index_define_function:
      case index_push_unnamed_array:
      case index_push_unnamed_object:
      case index_apply_operator:
      case index_unpack_array:
      case index_unpack_object:
      case index_define_null_variable:
      case index_single_step_trap:
      case index_defer_expression:
      case index_import_call:
      case index_declare_reference:
      case index_initialize_reference:
      case index_catch_expression:
      case index_push_constant:
      case index_alt_clear_stack:
      case index_coalesce_expression:
      case index_member_access:
      case index_apply_operator_bi32:
      case index_check_null:
        return false;

      case index_throw_statement:
      case index_return_statement:
      case index_return_statement_bi32:
        return true;

      case index_simple_status:
        return this->m_stor.as<S_simple_status>().status != air_status_next;

      case index_function_call:
        return this->m_stor.as<S_function_call>().ptc != ptc_aware_none;

      case index_variadic_call:
        return this->m_stor.as<S_variadic_call>().ptc != ptc_aware_none;

      case index_alt_function_call:
        return this->m_stor.as<S_alt_function_call>().ptc != ptc_aware_none;

      case index_execute_block:
        {
          const auto& altr = this->m_stor.as<S_execute_block>();
          return !altr.code_body.empty() && altr.code_body.back().is_terminator();
        }

      case index_if_statement:
        {
          const auto& altr = this->m_stor.as<S_if_statement>();
          return !altr.code_true.empty() && altr.code_true.back().is_terminator()
                 && !altr.code_false.empty() && altr.code_false.back().is_terminator();
        }

      case index_branch_expression:
        {
          const auto& altr = this->m_stor.as<S_branch_expression>();
          return !altr.code_true.empty() && altr.code_true.back().is_terminator()
                 && !altr.code_false.empty() && altr.code_false.back().is_terminator();
        }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
      }
  }

opt<AIR_Node>
AIR_Node::
rebind_opt(const Abstract_Context& ctx) const
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_clear_stack:
      case index_declare_variable:
      case index_initialize_variable:
      case index_throw_statement:
      case index_assert_statement:
      case index_simple_status:
      case index_check_argument:
      case index_push_global_reference:
      case index_push_bound_reference:
      case index_function_call:
      case index_push_unnamed_array:
      case index_push_unnamed_object:
      case index_apply_operator:
      case index_unpack_array:
      case index_unpack_object:
      case index_define_null_variable:
      case index_single_step_trap:
      case index_variadic_call:
      case index_import_call:
      case index_declare_reference:
      case index_initialize_reference:
      case index_return_statement:
      case index_push_constant:
      case index_alt_clear_stack:
      case index_alt_function_call:
      case index_member_access:
      case index_apply_operator_bi32:
      case index_return_statement_bi32:
      case index_check_null:
        return nullopt;

      case index_execute_block:
        {
          const auto& altr = this->m_stor.as<S_execute_block>();

          bool dirty = false;
          auto bound = altr;
          Analytic_Context ctx_empty(xtc_plain, ctx);

          do_rebind_nodes(dirty, bound.code_body, ctx_empty);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_if_statement:
        {
          const auto& altr = this->m_stor.as<S_if_statement>();

          bool dirty = false;
          auto bound = altr;
          Analytic_Context ctx_empty(xtc_plain, ctx);

          do_rebind_nodes(dirty, bound.code_true, ctx_empty);
          do_rebind_nodes(dirty, bound.code_false, ctx_empty);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_switch_statement:
        {
          const auto& altr = this->m_stor.as<S_switch_statement>();

          bool dirty = false;
          auto bound = altr;
          Analytic_Context ctx_empty(xtc_plain, ctx);

          for(size_t k = 0;  k < bound.clauses.size();  ++k) {
            // Labels are to be evaluated in the same scope as the condition
            // expression, and are not parts of the body.
            for(size_t i = 0;  i < bound.clauses.at(k).code_labels.size();  ++i)
              if(auto qnode = bound.clauses.at(k).code_labels.at(i).rebind_opt(ctx))
                do_set_rebound(dirty, bound.clauses.mut(k).code_labels.mut(i), move(*qnode));

            for(size_t i = 0;  i < bound.clauses.at(k).code_body.size();  ++i)
              if(auto qnode = bound.clauses.at(k).code_body.at(i).rebind_opt(ctx_empty))
                do_set_rebound(dirty, bound.clauses.mut(k).code_body.mut(i), move(*qnode));
          }

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_do_while_statement:
        {
          const auto& altr = this->m_stor.as<S_do_while_statement>();

          bool dirty = false;
          auto bound = altr;
          Analytic_Context ctx_empty(xtc_plain, ctx);

          // The condition expression is not a part of the body.
          do_rebind_nodes(dirty, bound.code_body, ctx_empty);
          do_rebind_nodes(dirty, bound.code_cond, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_while_statement:
        {
          const auto& altr = this->m_stor.as<S_while_statement>();

          bool dirty = false;
          auto bound = altr;
          Analytic_Context ctx_empty(xtc_plain, ctx);

          // The condition expression is not a part of the body.
          do_rebind_nodes(dirty, bound.code_cond, ctx);
          do_rebind_nodes(dirty, bound.code_body, ctx_empty);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_for_each_statement:
        {
          const auto& altr = this->m_stor.as<S_for_each_statement>();

          bool dirty = false;
          auto bound = altr;
          Analytic_Context ctx_for(xtc_plain, ctx);
          Analytic_Context ctx_body(xtc_plain, ctx_for);

          // The range key and mapped references are declared in a dedicated scope
          // where the initializer is to be evaluated. The body is to be executed
          // in an inner scope, created and destroyed for each iteration.
          do_rebind_nodes(dirty, bound.code_init, ctx_for);
          do_rebind_nodes(dirty, bound.code_body, ctx_body);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_for_statement:
        {
          const auto& altr = this->m_stor.as<S_for_statement>();

          bool dirty = false;
          auto bound = altr;
          Analytic_Context ctx_for(xtc_plain, ctx);
          Analytic_Context ctx_body(xtc_plain, ctx_for);

          // All these are declared in a dedicated scope where the initializer is
          // to be evaluated. The body is to be executed in an inner scope,
          // created and destroyed for each iteration.
          do_rebind_nodes(dirty, bound.code_init, ctx_for);
          do_rebind_nodes(dirty, bound.code_cond, ctx_for);
          do_rebind_nodes(dirty, bound.code_step, ctx_for);
          do_rebind_nodes(dirty, bound.code_body, ctx_body);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_try_statement:
        {
          const auto& altr = this->m_stor.as<S_try_statement>();

          bool dirty = false;
          auto bound = altr;
          Analytic_Context ctx_empty(xtc_plain, ctx);

          do_rebind_nodes(dirty, bound.code_try, ctx_empty);
          do_rebind_nodes(dirty, bound.code_catch, ctx_empty);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_push_local_reference:
        {
          const auto& altr = this->m_stor.as<S_push_local_reference>();

          // Get the context.
          const Abstract_Context* qctx = &ctx;
          for(uint32_t k = 0;  k != altr.depth;  ++k)
            qctx = qctx->get_parent_opt();

          if(qctx->is_analytic())
            return nullopt;

          // Look for the name.
          auto qref = qctx->get_named_reference_opt(altr.name);
          if(!qref)
            return nullopt;
          else if(qref->is_invalid())
            throw Runtime_Error(xtc_format,
                     "Initialization of variable or reference `$1` bypassed", altr.name);

          // Bind this reference.
          S_push_bound_reference xnode = { *qref };
          return move(xnode);
        }

      case index_define_function:
        {
          const auto& altr = this->m_stor.as<S_define_function>();

          bool dirty = false;
          auto bound = altr;
          Analytic_Context ctx_empty(xtc_plain, ctx);

          // This is the only scenario where names in the outer scope are visible
          // to the body of a function.
          do_rebind_nodes(dirty, bound.code_body, ctx_empty);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_branch_expression:
        {
          const auto& altr = this->m_stor.as<S_branch_expression>();

          bool dirty = false;
          auto bound = altr;

          do_rebind_nodes(dirty, bound.code_true, ctx);
          do_rebind_nodes(dirty, bound.code_false, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_defer_expression:
        {
          const auto& altr = this->m_stor.as<S_defer_expression>();

          bool dirty = false;
          auto bound = altr;

          do_rebind_nodes(dirty, bound.code_body, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_catch_expression:
        {
          const auto& altr = this->m_stor.as<S_catch_expression>();

          bool dirty = false;
          auto bound = altr;

          do_rebind_nodes(dirty, bound.code_body, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_coalesce_expression:
        {
          const auto& altr = this->m_stor.as<S_coalesce_expression>();

          bool dirty = false;
          auto bound = altr;

          do_rebind_nodes(dirty, bound.code_null, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
      }
  }

void
AIR_Node::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_clear_stack:
      case index_declare_variable:
      case index_initialize_variable:
      case index_throw_statement:
      case index_assert_statement:
      case index_simple_status:
      case index_check_argument:
      case index_push_global_reference:
      case index_push_local_reference:
      case index_function_call:
      case index_push_unnamed_array:
      case index_push_unnamed_object:
      case index_apply_operator:
      case index_unpack_array:
      case index_unpack_object:
      case index_define_null_variable:
      case index_single_step_trap:
      case index_variadic_call:
      case index_import_call:
      case index_declare_reference:
      case index_initialize_reference:
      case index_return_statement:
      case index_alt_clear_stack:
      case index_alt_function_call:
      case index_member_access:
      case index_apply_operator_bi32:
      case index_return_statement_bi32:
      case index_check_null:
        return;

      case index_execute_block:
        {
          const auto& altr = this->m_stor.as<S_execute_block>();

          // Collect variables from the body.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_if_statement:
        {
          const auto& altr = this->m_stor.as<S_if_statement>();

          // Collect variables from both branches.
          do_collect_variables_for_each(staged, temp, altr.code_true);
          do_collect_variables_for_each(staged, temp, altr.code_false);
          return;
        }

      case index_switch_statement:
        {
          const auto& altr = this->m_stor.as<S_switch_statement>();

          // Collect variables from all labels and clauses.
          for(const auto& clause : altr.clauses)
            do_collect_variables_for_each(staged, temp, clause.code_labels);
          for(const auto& clause : altr.clauses)
            do_collect_variables_for_each(staged, temp, clause.code_body);
          return;
        }

      case index_do_while_statement:
        {
          const auto& altr = this->m_stor.as<S_do_while_statement>();

          // Collect variables from the body and the condition expression.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          do_collect_variables_for_each(staged, temp, altr.code_cond);
          return;
        }

      case index_while_statement:
        {
          const auto& altr = this->m_stor.as<S_while_statement>();

          // Collect variables from the condition expression and the body.
          do_collect_variables_for_each(staged, temp, altr.code_cond);
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_for_each_statement:
        {
          const auto& altr = this->m_stor.as<S_for_each_statement>();

          // Collect variables from the range initializer and the body.
          do_collect_variables_for_each(staged, temp, altr.code_init);
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_for_statement:
        {
          const auto& altr = this->m_stor.as<S_for_statement>();

          // Collect variables from the initializer, condition expression and
          // step expression.
          do_collect_variables_for_each(staged, temp, altr.code_init);
          do_collect_variables_for_each(staged, temp, altr.code_cond);
          do_collect_variables_for_each(staged, temp, altr.code_step);
          return;
        }

      case index_try_statement:
        {
          const auto& altr = this->m_stor.as<S_try_statement>();

          // Collect variables from the `try` and `catch` clauses.
          do_collect_variables_for_each(staged, temp, altr.code_try);
          do_collect_variables_for_each(staged, temp, altr.code_catch);
          return;
        }

      case index_push_bound_reference:
        {
          const auto& altr = this->m_stor.as<S_push_bound_reference>();

          // Collect variables from the bound reference.
          altr.ref.collect_variables(staged, temp);
          return;
        }

      case index_define_function:
        {
          const auto& altr = this->m_stor.as<S_define_function>();

          // Collect variables from the function body.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_branch_expression:
        {
          const auto& altr = this->m_stor.as<S_branch_expression>();

          // Collect variables from both branches.
          do_collect_variables_for_each(staged, temp, altr.code_true);
          do_collect_variables_for_each(staged, temp, altr.code_false);
          return;
        }

      case index_defer_expression:
        {
          const auto& altr = this->m_stor.as<S_defer_expression>();

          // Collect variables from the expression.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_catch_expression:
        {
          const auto& altr = this->m_stor.as<S_catch_expression>();

          // Collect variables from the expression.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_push_constant:
        {
          const auto& altr = this->m_stor.as<S_push_constant>();

          // Collect variables from the expression.
          altr.val.collect_variables(staged, temp);
          return;
        }

      case index_coalesce_expression:
        {
          const auto& altr = this->m_stor.as<S_coalesce_expression>();

          // Collect variables from the null branch.
          do_collect_variables_for_each(staged, temp, altr.code_null);
          return;
        }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
      }
  }

void
AIR_Node::
solidify(AVM_Rod& rod) const
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_clear_stack:
        {
          const auto& altr = this->m_stor.as<S_clear_stack>();

          (void) altr;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* /*head*/)
              __attribute__((__hot__, __flatten__))
              {
                ctx.stack().clear();
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_execute_block:
        {
          const auto& altr = this->m_stor.as<S_execute_block>();

          struct Sparam
            {
              AVM_Rod rod_body;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_body, altr.code_body);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Execute the block on a new context. The block may contain control
                // statements, so the status shall be forwarded verbatim.
                do_execute_block(sp.rod_body, ctx);
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_declare_variable:
        {
          const auto& altr = this->m_stor.as<S_declare_variable>();

          struct Sparam
            {
              phcow_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = *(head->pv_meta->sloc_opt);

                // Allocate a variable and inject it into the current context.
                const auto gcoll = ctx.global().garbage_collector();
                const auto var = gcoll->create_variable();
                ctx.insert_named_reference(sp.name).set_variable(var);
                ctx.global().call_hook(&Abstract_Hooks::on_declare, sloc, sp.name);

                // Push a copy of the reference onto the stack, which we will get
                // back after the initializer finishes execution.
                ctx.stack().push().set_variable(var);
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_initialize_variable:
        {
          const auto& altr = this->m_stor.as<S_initialize_variable>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.immutable;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool immutable = head->uparam.b0;

                // Read the value of the initializer. The initializer must not have
                // been empty for this function.
                const Value& val = ctx.stack().top().dereference_readonly();
                auto var = ctx.stack().top(1).unphase_variable_opt();
                ROCKET_ASSERT(var && !var->is_initialized());

                // Initialize it with this value.
                var->initialize(val);
                var->set_immutable(immutable);
                ctx.stack().pop(2);
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_if_statement:
        {
          const auto& altr = this->m_stor.as<S_if_statement>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.negative;

          struct Sparam
            {
              AVM_Rod rod_true;
              AVM_Rod rod_false;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_true, altr.code_true);
          do_solidify_nodes(sp2.rod_false, altr.code_false);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool negative = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Check the condition and pick a branch.
                if(ctx.stack().top().dereference_readonly().test() != negative)
                  do_execute_block(sp.rod_true, ctx);
                else
                  do_execute_block(sp.rod_false, ctx);
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_true.collect_variables(staged, temp);
                sp.rod_false.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_switch_statement:
        {
          const auto& altr = this->m_stor.as<S_switch_statement>();

          struct Sparam_switch_clause
            {
              Switch_Clause_Type type;
              Compare cmp2_lower;
              Compare cmp2_upper;
              AVM_Rod rod_labels;
              AVM_Rod rod_body;
              cow_vector<phcow_string> names_added;
            };

          using Sparam = cow_vector<Sparam_switch_clause>;
          Sparam sp2;
          sp2.reserve(altr.clauses.size());

          for(const auto& clause : altr.clauses) {
            auto& r = sp2.emplace_back();
            r.type = clause.type;
            r.cmp2_lower = clause.lower_closed ? compare_equal : compare_greater;
            r.cmp2_upper = clause.upper_closed ? compare_equal : compare_less;
            do_solidify_nodes(r.rod_labels, clause.code_labels);
            do_solidify_nodes(r.rod_body, clause.code_body);
            r.names_added = clause.names_added;
          }

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read the value of the condition and find the target clause for it.
                // This is different from the `switch` statement in C, where `case`
                // labels must have constant operands.
                auto cond = ctx.stack().top().dereference_readonly();
                size_t target_index = SIZE_MAX;

                for(size_t k = 0;  k < sp.size();  ++k)
                  if(sp.at(k).type == switch_clause_default) {
                    target_index = k;
                  }
                  else if(sp.at(k).type == switch_clause_case) {
                    // Expect an exact match of one value.
                    sp.at(k).rod_labels.execute(ctx);
                    ROCKET_ASSERT(ctx.status() == air_status_next);

                    auto cmp = cond.compare_partial(ctx.stack().top().dereference_readonly());
                    if(cmp != compare_equal)
                      continue;

                    target_index = k;
                    break;
                  }
                  else if(sp.at(k).type == switch_clause_each) {
                    // Expect an interval of two values.
                    sp.at(k).rod_labels.execute(ctx);
                    ROCKET_ASSERT(ctx.status() == air_status_next);

                    auto cmp_lo = cond.compare_partial(ctx.stack().top(1).dereference_readonly());
                    auto cmp_up = cond.compare_partial(ctx.stack().top(0).dereference_readonly());
                    if(::rocket::is_none_of(cmp_lo, { compare_greater, sp.at(k).cmp2_lower })
                       || ::rocket::is_none_of(cmp_up, { compare_less, sp.at(k).cmp2_upper }))
                      continue;

                    target_index = k;
                    break;
                  }

                // Skip this statement if no matching clause has been found.
                if(target_index >= sp.size())
                  return;

                // Jump to the target clause.
                Executive_Context ctx_body(xtc_plain, ctx);
                try {
                  for(size_t i = 0;  i < sp.size();  ++i)
                    if(i < target_index) {
                      // Inject bypassed names into the scope.
                      for(const auto& name : sp.at(i).names_added)
                        ctx_body.insert_named_reference(name);
                    }
                    else {
                      // Execute the body of this clause.
                      sp.at(i).rod_body.execute(ctx_body);
                      if(ctx.status() != air_status_next) {
                        if(::rocket::is_any_of(ctx.status(), { air_status_break, air_status_break_switch }))
                          ctx.status() = air_status_next;
                        break;
                      }
                    }
                }
                catch(Runtime_Error& except) {
                  ctx_body.on_scope_exit_exceptional(except);
                  throw;
                }
                ctx_body.on_scope_exit_normal();
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                for(const auto& r : sp)
                  r.rod_labels.collect_variables(staged, temp);
                for(const auto& r : sp)
                  r.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_do_while_statement:
        {
          const auto& altr = this->m_stor.as<S_do_while_statement>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.negative;

          struct Sparam
            {
              AVM_Rod rod_body;
              AVM_Rod rod_cond;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_body, altr.code_body);
          do_solidify_nodes(sp2.rod_cond, altr.code_cond);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool negative = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // This is the same as a `do`..`while` loop in other languages.
                // A `break while` statement shall terminate the loop and return
                // `air_status_next` to resume execution after the loop body.
                for(;;) {
                  do_execute_block(sp.rod_body, ctx);
                  if(::rocket::is_any_of(ctx.status(), { air_status_continue, air_status_continue_while }))
                    ctx.status() = air_status_next;
                  else if(ctx.status() != air_status_next) {
                    if(::rocket::is_any_of(ctx.status(), { air_status_break, air_status_break_while }))
                      ctx.status() = air_status_next;
                    break;
                  }

                  sp.rod_cond.execute(ctx);
                  ROCKET_ASSERT(ctx.status() == air_status_next);
                  if(ctx.stack().top().dereference_readonly().test() == negative)
                    break;
                }
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_body.collect_variables(staged, temp);
                sp.rod_cond.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_while_statement:
        {
          const auto& altr = this->m_stor.as<S_while_statement>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.negative;

          struct Sparam
            {
              AVM_Rod rod_cond;
              AVM_Rod rod_body;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_cond, altr.code_cond);
          do_solidify_nodes(sp2.rod_body, altr.code_body);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool negative = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // This is the same as a `while` loop in other languages. A
                // `break while` statement shall terminate the loop and return
                // `air_status_next` to resume execution after the loop body.
                for(;;) {
                  sp.rod_cond.execute(ctx);
                  ROCKET_ASSERT(ctx.status() == air_status_next);
                  if(ctx.stack().top().dereference_readonly().test() == negative)
                    break;

                  do_execute_block(sp.rod_body, ctx);
                  if(::rocket::is_any_of(ctx.status(), { air_status_continue, air_status_continue_while }))
                    ctx.status() = air_status_next;
                  else if(ctx.status() != air_status_next) {
                    if(::rocket::is_any_of(ctx.status(), { air_status_break, air_status_break_while }))
                      ctx.status() = air_status_next;
                    break;
                  }
                }
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_cond.collect_variables(staged, temp);
                sp.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_for_each_statement:
        {
          const auto& altr = this->m_stor.as<S_for_each_statement>();

          struct Sparam
            {
              phcow_string name_key;
              phcow_string name_mapped;
              Source_Location sloc_init;
              AVM_Rod rod_init;
              AVM_Rod rod_body;
            };

          Sparam sp2;
          sp2.name_key = altr.name_key;
          sp2.name_mapped = altr.name_mapped;
          sp2.sloc_init = altr.sloc_init;
          do_solidify_nodes(sp2.rod_init, altr.code_init);
          do_solidify_nodes(sp2.rod_body, altr.code_body);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // This is the same as a `for` loop in other languages. A `break for`
                // statement shall terminate the loop and return  `air_status_next`
                // to resume execution after the loop body. We have to create an outer
                // context oweing to the fact that the key and mapped references
                // outlast every iteration.
                Executive_Context ctx_for(xtc_plain, ctx);
                try {
                  // Create key and mapped references.
                  if(!sp.name_key.empty())
                    ctx_for.insert_named_reference(sp.name_key);
                  ctx_for.insert_named_reference(sp.name_mapped);

                  // Evaluate the range initializer and set the range up, which isn't
                  // going to change for all loops.
                  sp.rod_init.execute(ctx_for);
                  ROCKET_ASSERT(ctx_for.status() == air_status_next);

                  Reference* qkey_ref = nullptr;
                  if(!sp.name_key.empty())
                    qkey_ref = ctx_for.mut_named_reference_opt(sp.name_key);

                  Reference* mapped_ref = ctx_for.mut_named_reference_opt(sp.name_mapped);
                  ROCKET_ASSERT(mapped_ref);
                  *mapped_ref = move(ctx_for.stack().mut_top());

                  auto range = mapped_ref->dereference_readonly();
                  if(range.is_array()) {
                    const auto& arr = range.as_array();
                    mapped_ref->push_subscript(Subscript::S_array_head());  // placeholder
                    for(int64_t i = 0;  i < arr.ssize();  ++i) {
                      // Set the key variable which is the subscript of the mapped
                      // element in the array.
                      if(qkey_ref) {
                        auto key_var = qkey_ref->unphase_variable_opt();
                        if(!key_var) {
                          key_var = ctx.global().garbage_collector()->create_variable();
                          qkey_ref->set_variable(key_var);
                        }
                        key_var->initialize(i);
                        key_var->set_immutable();
                      }

                      // Set the mapped reference.
                      mapped_ref->pop_subscript();
                      Subscript::S_array_index xsub = { i };
                      do_push_subscript_and_check(*mapped_ref, move(xsub));

                      // Execute the loop body.
                      do_execute_block(sp.rod_body, ctx_for);
                      if(::rocket::is_any_of(ctx.status(), { air_status_continue, air_status_continue_for }))
                        ctx.status() = air_status_next;
                      else if(ctx.status() != air_status_next) {
                        if(::rocket::is_any_of(ctx.status(), { air_status_break, air_status_break_for }))
                          ctx.status() = air_status_next;
                        break;
                      }
                    }
                  }
                  else if(range.is_object()) {
                    const auto& obj = range.as_object();
                    mapped_ref->push_subscript(Subscript::S_array_head());  // placeholder
                    for(auto it = obj.begin();  it != obj.end();  ++it) {
                      // Set the key variable which is the name of the mapped element
                      // in the object.
                      if(qkey_ref) {
                        auto key_var = qkey_ref->unphase_variable_opt();
                        if(!key_var) {
                          key_var = ctx.global().garbage_collector()->create_variable();
                          qkey_ref->set_variable(key_var);
                        }
                        key_var->initialize(it->first.rdstr());
                        key_var->set_immutable();
                      }

                      // Set the mapped reference.
                      mapped_ref->pop_subscript();
                      Subscript::S_object_key xsub = { it->first };
                      do_push_subscript_and_check(*mapped_ref, move(xsub));

                      // Execute the loop body.
                      do_execute_block(sp.rod_body, ctx_for);
                      if(::rocket::is_any_of(ctx.status(), { air_status_continue, air_status_continue_for }))
                        ctx.status() = air_status_next;
                      else if(ctx.status() != air_status_next) {
                        if(::rocket::is_any_of(ctx.status(), { air_status_break, air_status_break_for }))
                          ctx.status() = air_status_next;
                        break;
                      }
                    }
                  }
                  else if(!range.is_null())
                    throw Runtime_Error(xtc_assert, sp.sloc_init,
                              sformat("Range value not iterable (value `$1`)", range));
                }
                catch(Runtime_Error& except) {
                  ctx_for.on_scope_exit_exceptional(except);
                  throw;
                }
                ctx_for.on_scope_exit_normal();
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_init.collect_variables(staged, temp);
                sp.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_for_statement:
        {
          const auto& altr = this->m_stor.as<S_for_statement>();

          struct Sparam
            {
              AVM_Rod rod_init;
              AVM_Rod rod_cond;
              AVM_Rod rod_step;
              AVM_Rod rod_body;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_init, altr.code_init);
          do_solidify_nodes(sp2.rod_cond, altr.code_cond);
          do_solidify_nodes(sp2.rod_step, altr.code_step);
          do_solidify_nodes(sp2.rod_body, altr.code_body);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // This is the same as a `for` loop in other languages. A `break for`
                // statement shall terminate the loop and return  `air_status_next`
                // to resume execution after the loop body. We have to create an outer
                // context oweing to the fact that names declared in the first segment
                // outlast every iteration.
                Executive_Context ctx_for(xtc_plain, ctx);
                try {
                  // Execute the loop initializer, which shall only be a definition or
                  // an expression statement.
                  sp.rod_init.execute(ctx_for);
                  ROCKET_ASSERT(ctx_for.status() == air_status_next);

                  for(;;) {
                    // Check the condition. If the condition is empty, then it is
                    // always true, and the loop is infinite.
                    sp.rod_cond.execute(ctx_for);
                    ROCKET_ASSERT(ctx_for.status() == air_status_next);
                    if(ctx_for.stack().size() && !ctx_for.stack().top().dereference_readonly().test())
                      break;

                    do_execute_block(sp.rod_body, ctx_for);
                    if(::rocket::is_any_of(ctx.status(), { air_status_continue, air_status_continue_for }))
                      ctx.status() = air_status_next;
                    else if(ctx.status() != air_status_next) {
                      if(::rocket::is_any_of(ctx.status(), { air_status_break, air_status_break_for }))
                        ctx.status() = air_status_next;
                      break;
                    }

                    // Execute the increment.
                    sp.rod_step.execute(ctx_for);
                    ROCKET_ASSERT(ctx_for.status() == air_status_next);
                  }
                }
                catch(Runtime_Error& except) {
                  ctx_for.on_scope_exit_exceptional(except);
                  throw;
                }
                ctx_for.on_scope_exit_normal();
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_init.collect_variables(staged, temp);
                sp.rod_cond.collect_variables(staged, temp);
                sp.rod_step.collect_variables(staged, temp);
                sp.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_try_statement:
        {
          const auto& altr = this->m_stor.as<S_try_statement>();

          struct Sparam
            {
              AVM_Rod rod_try;
              Source_Location sloc_catch;
              phcow_string name_except;
              AVM_Rod rod_catch;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_try, altr.code_try);
          sp2.sloc_catch = altr.sloc_catch;
          sp2.name_except = altr.name_except;
          do_solidify_nodes(sp2.rod_catch, altr.code_catch);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const auto& try_sloc = *(head->pv_meta->sloc_opt);
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // This is almost identical to JavaScript but not to C++. Only one
                // `catch` clause is allowed.
                try {
                  do_execute_block(sp.rod_try, ctx);
                  if(ctx.status() == air_status_return)
                    ctx.stack().mut_top().check_function_result(ctx.global(), move(ctx.alt_stack()));
                }
                catch(Runtime_Error& except) {
                  // Append a frame due to exit of the `try` clause.
                  // Reuse the exception object. Don't bother allocating a new one.
                  except.push_frame_try(try_sloc);

                  // This branch must be executed inside this `catch` block.
                  // User-provided bindings may obtain the current exception using
                  // `::std::current_exception`.
                  Executive_Context ctx_catch(xtc_plain, ctx);
                  try {
                    // Set backtrace frames.
                    V_array backtrace;
                    for(size_t k = 0;  k < except.count_frames();  ++k) {
                      V_object r;
                      r.try_emplace(&"frame", ::rocket::sref(describe_frame_type(except.frame(k).type)));
                      r.try_emplace(&"file", except.frame(k).sloc.file());
                      r.try_emplace(&"line", except.frame(k).sloc.line());
                      r.try_emplace(&"column", except.frame(k).sloc.column());
                      r.try_emplace(&"value", except.frame(k).value);
                      backtrace.emplace_back(move(r));
                    }

                    ctx_catch.insert_named_reference(&"__backtrace").set_temporary(move(backtrace));
                    ctx_catch.insert_named_reference(sp.name_except).set_temporary(except.value());

                    // Execute the `catch` clause.
                    sp.rod_catch.execute(ctx_catch);
                  }
                  catch(Runtime_Error& nested) {
                    ctx_catch.on_scope_exit_exceptional(nested);
                    nested.push_frame_catch(sp.sloc_catch, except.value());
                    throw;
                  }
                  ctx_catch.on_scope_exit_normal();
                }
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_try.collect_variables(staged, temp);
                sp.rod_catch.collect_variables(staged, temp);
              }

            // Symbols
            , &(altr.sloc_try)
          );
          return;
        }

      case index_throw_statement:
        {
          const auto& altr = this->m_stor.as<S_throw_statement>();

          struct Sparam
            {
              Source_Location sloc;
            };

          Sparam sp2;
          sp2.sloc = altr.sloc;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__, __noreturn__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read a value and throw it. The operand expression must not have
                // been empty for this function.
                Value val = ctx.stack().mut_top().dereference_copy();
                if(val.is_null())
                  throw Runtime_Error(xtc_assert, sp.sloc, &"`null` not throwable");

                ctx.stack().pop();
                ctx.global().call_hook(&Abstract_Hooks::on_throw, sp.sloc, val);
                throw Runtime_Error(xtc_throw, move(val), sp.sloc);
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_assert_statement:
        {
          const auto& altr = this->m_stor.as<S_assert_statement>();

          struct Sparam
            {
              Source_Location sloc;
              cow_string msg;
            };

          Sparam sp2;
          sp2.sloc = altr.sloc;
          sp2.msg = altr.msg;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Throw an exception if the assertion fails. This cannot be disabled.
                const auto& tval = ctx.stack().top().dereference_readonly();
                if(ROCKET_UNEXPECT(!tval.test()))
                  throw Runtime_Error(xtc_assert, sp.sloc, sp.msg);
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_simple_status:
        {
          const auto& altr = this->m_stor.as<S_simple_status>();

          AVM_Rod::Uparam up2;
          up2.u0 = altr.status;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                ctx.status() = static_cast<AIR_Status>(head->uparam.u0);
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_check_argument:
        {
          const auto& altr = this->m_stor.as<S_check_argument>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.by_ref;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool by_ref = head->uparam.b0;

                // Ensure the argument is dereferenceable. If it is to be passed
                // by value, also convert it to a temporary value.
                if(by_ref)
                  ctx.stack().top().dereference_readonly();
                else
                  ctx.stack().mut_top().dereference_copy();
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_global_reference:
        {
          const auto& altr = this->m_stor.as<S_push_global_reference>();

          struct Sparam
            {
              phcow_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Look for the name in the global context.
                auto qref = ctx.global().get_named_reference_opt(sp.name);
                if(!qref)
                  throw Runtime_Error(xtc_format,
                           "Undeclared identifier `$1`", sp.name);

                if(qref->is_invalid())
                  throw Runtime_Error(xtc_format,
                           "Global reference `$1` is uninitialized", sp.name);

                // Push a copy of the reference onto the stack.
                ctx.stack().push() = *qref;
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_local_reference:
        {
          const auto& altr = this->m_stor.as<S_push_local_reference>();

          AVM_Rod::Uparam up2;
          up2.u01 = altr.depth;

          struct Sparam
            {
              phcow_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const uint32_t depth = head->uparam.u01;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Locate the target context.
                const Executive_Context* qctx = &ctx;
                for(uint32_t k = 0;  k != depth;  ++k)
                  qctx = qctx->get_parent_opt();

                // Look for the name in the target context.
                auto qref = qctx->get_named_reference_opt(sp.name);
                if(!qref)
                  throw Runtime_Error(xtc_format,
                           "Undeclared identifier `$1`", sp.name);

                if(qref->is_invalid())
                  throw Runtime_Error(xtc_format,
                           "Initialization of `$1` was bypassed", sp.name);

                // Push a copy of the reference onto the stack.
                ctx.stack().push() = *qref;
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_bound_reference:
        {
          const auto& altr = this->m_stor.as<S_push_bound_reference>();

          struct Sparam
            {
              Reference ref;
            };

          Sparam sp2;
          sp2.ref = altr.ref;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Push a copy of the captured reference.
                ctx.stack().push() = sp.ref;
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.ref.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_define_function:
        {
          const auto& altr = this->m_stor.as<S_define_function>();

          struct Sparam
            {
              Compiler_Options opts;
              cow_string func;
              cow_vector<phcow_string> params;
              cow_vector<AIR_Node> code_body;
            };

          Sparam sp2;
          sp2.opts = altr.opts;
          sp2.func = altr.func;
          sp2.params = altr.params;
          sp2.code_body = altr.code_body;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = *(head->pv_meta->sloc_opt);

                // Instantiate the function.
                AIR_Optimizer optmz(sp.opts);
                optmz.rebind(&ctx, sp.params, sp.code_body);

                // Push the function as a temporary value.
                ctx.stack().push().set_temporary(optmz.create_function(sloc, sp.func));
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                do_collect_variables_for_each(staged, temp, sp.code_body);
              }

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_branch_expression:
        {
          const auto& altr = this->m_stor.as<S_branch_expression>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.assign;

          struct Sparam
            {
              AVM_Rod rod_true;
              AVM_Rod rod_false;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_true, altr.code_true);
          do_solidify_nodes(sp2.rod_false, altr.code_false);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool assign = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Check the condition and pick a branch.
                if(ctx.stack().top().dereference_readonly().test())
                  do_evaluate_subexpression(ctx, assign, sp.rod_true);
                else
                  do_evaluate_subexpression(ctx, assign, sp.rod_false);
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_true.collect_variables(staged, temp);
                sp.rod_false.collect_variables(staged, temp);
              }

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_function_call:
        {
          const auto& altr = this->m_stor.as<S_function_call>();

          AVM_Rod::Uparam up2;
          up2.u0 = altr.ptc;
          up2.u2345 = altr.nargs;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const PTC_Aware ptc = static_cast<PTC_Aware>(head->uparam.u0);
                const uint32_t nargs = head->uparam.u2345;
                const auto& sloc = *(head->pv_meta->sloc_opt);
                const auto sentry = ctx.global().copy_recursion_sentry();

                // Collect arguments from left to right.
                ctx.alt_stack().clear();
                for(uint32_t k = nargs - 1;  k != UINT32_MAX;  --k)
                  ctx.alt_stack().push() = move(ctx.stack().mut_top(k));
                ctx.stack().pop(nargs);

                // Invoke the target function.
                auto target = do_get_target_function(ctx.stack().top());
                ctx.stack().mut_top().pop_subscript();
                do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc, target);
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_unnamed_array:
        {
          const auto& altr = this->m_stor.as<S_push_unnamed_array>();

          AVM_Rod::Uparam up2;
          up2.u2345 = altr.nelems;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const uint32_t nelems = head->uparam.u2345;

                // Pop elements from the stack and fill them from right to left.
                V_array arr;
                arr.resize(nelems);
                for(auto it = arr.mut_rbegin();  it != arr.rend();  ++it) {
                  *it = ctx.stack().top().dereference_readonly();
                  ctx.stack().pop();
                }

                // Push the array as a temporary.
                ctx.stack().push().set_temporary(move(arr));
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_unnamed_object:
        {
          const auto& altr = this->m_stor.as<S_push_unnamed_object>();

          struct Sparam
            {
              cow_vector<phcow_string> keys;
            };

          Sparam sp2;
          sp2.keys = altr.keys;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Pop elements from the stack and set them from right to left. In case
                // of duplicate keys, the rightmost value takes precedence.
                V_object obj;
                obj.reserve(sp.keys.size());
                for(auto it = sp.keys.rbegin();  it != sp.keys.rend();  ++it) {
                  obj.try_emplace(*it, ctx.stack().top().dereference_readonly());
                  ctx.stack().pop();
                }

                // Push the object as a temporary.
                ctx.stack().push().set_temporary(move(obj));
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_apply_operator:
        {
          const auto& altr = this->m_stor.as<S_apply_operator>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.assign;

          switch(altr.xop)
            {
            case xop_inc:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool postfix = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = top.dereference_mutable();

                    if(rhs.is_integer()) {
                      // ++ integer ; may overflow
                      V_integer& val = rhs.open_integer();
                      V_integer result;
                      if(ROCKET_ADD_OVERFLOW(val, 1, &result))
                        throw Runtime_Error(xtc_format,
                           "Integer increment overflow (operand was `$1`)", val);
                      if(postfix)
                        top.set_temporary(val);
                      val = result;
                    }
                    else if(rhs.is_real()) {
                      // ++ real ; can't overflow
                      V_real& val = rhs.open_real();
                      V_real result = val + 1;
                      if(postfix)
                        top.set_temporary(val);
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Increment not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_dec:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool postfix = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = top.dereference_mutable();

                    if(rhs.is_integer()) {
                      // -- integer ; may overflow
                      V_integer& val = rhs.open_integer();
                      V_integer result;
                      if(ROCKET_SUB_OVERFLOW(val, 1, &result))
                        throw Runtime_Error(xtc_format,
                           "Integer increment overflow (operand was `$1`)", val);
                      if(postfix)
                        top.set_temporary(val);
                      val = result;
                    }
                    else if(rhs.is_real()) {
                      // -- real ; can't overflow
                      V_real& val = rhs.open_real();
                      V_real result = val - 1;
                      if(postfix)
                        top.set_temporary(val);
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Decrement not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_unset:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* /*head*/)
                  __attribute__((__hot__, __flatten__))
                  {
                    Reference& top = ctx.stack().mut_top();

                    // unset ; `assign` ignored
                    Value val = top.dereference_unset();
                    top.set_temporary(move(val));
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_head:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* /*head*/)
                  __attribute__((__hot__, __flatten__))
                  {
                    Reference& top = ctx.stack().mut_top();

                    // array [^] ; `assign` ignored
                    Subscript::S_array_head xsub = { };
                    do_push_subscript_and_check(top, move(xsub));
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_tail:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* /*head*/)
                  __attribute__((__hot__, __flatten__))
                  {
                    Reference& top = ctx.stack().mut_top();

                    // array [$] ; `assign` ignored
                    Subscript::S_array_tail xsub = { };
                    do_push_subscript_and_check(top, move(xsub));
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_random:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* /*head*/)
                  __attribute__((__hot__, __flatten__))
                  {
                    Reference& top = ctx.stack().mut_top();
                    const auto prng = ctx.global().random_engine();

                    // array [?] ; `assign` ignored
                    Subscript::S_array_random xsub = { prng->bump() };
                    do_push_subscript_and_check(top, move(xsub));
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_isvoid:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* /*head*/)
                  __attribute__((__hot__, __flatten__))
                  {
                    Reference& top = ctx.stack().mut_top();

                    // __isvoid ; `assign` ignored
                    top.set_temporary(top.is_void());
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_assign:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* /*head*/)
                  __attribute__((__hot__, __flatten__))
                  {
                    Value& rhs = ctx.stack().mut_top().dereference_copy();
                    Reference& top = ctx.stack().mut_top(1);

                    // x = y ; `assign` ignored
                    top.dereference_mutable() = move(rhs);
                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_index:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* /*head*/)
                  __attribute__((__hot__, __flatten__))
                  {
                    Value& rhs = ctx.stack().mut_top().dereference_copy();
                    Reference& top = ctx.stack().mut_top(1);

                    if(rhs.is_integer()) {
                      Subscript::S_array_index xsub = { rhs.as_integer() };
                      do_push_subscript_and_check(top, move(xsub));
                    }
                    else if(rhs.is_string()) {
                      Subscript::S_object_key xsub = { move(rhs.open_string()) };
                      do_push_subscript_and_check(top, move(xsub));
                    }
                    else throw Runtime_Error(xtc_format,
                            "Subscript value not valid (operand was `$1`)", rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_pos:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // no-op
                    (void) rhs;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_neg:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // - integer ; may overflow
                      V_integer& val = rhs.open_integer();
                      V_integer result;
                      if(ROCKET_SUB_OVERFLOW(0, val, &result))
                        throw Runtime_Error(xtc_format,
                           "Integer negation overflow (operand was `$1`)", val);
                      val = result;
                    }
                    else if(rhs.is_real()) {
                      // - real ; can't overflow
                      V_real& val = rhs.open_real();
                      val = -val;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Arithmetic negation not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_notb:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // ~ integer
                      V_integer& val = rhs.open_integer();
                      val = ~val;
                    }
                    else if(rhs.is_boolean()) {
                      // ~ boolean ; same as !
                      V_boolean& val = rhs.open_boolean();
                      val = !val;
                    }
                    else if(rhs.is_string()) {
                      // ~ string ; bitwise
                      V_string& val = rhs.open_string();
                      for(auto it = val.mut_begin();  it != val.end();  ++it)
                        *it = static_cast<char>(~*it);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Bitwise NOT not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_notl:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // ! ; logical
                    rhs = !rhs.test();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_countof:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_null()) {
                      rhs = V_integer(0);
                    }
                    else if(rhs.is_string()) {
                      rhs = V_integer(rhs.as_string().size());
                    }
                    else if(rhs.is_array()) {
                      rhs = V_integer(rhs.as_array().size());
                    }
                    else if(rhs.is_object()) {
                      rhs = V_integer(rhs.as_object().size());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`countof` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_typeof:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // typeof
                    rhs = V_string(::rocket::sref(describe_type(rhs.type())));
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sqrt:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_real()) {
                      // __sqrt; always real
                      rhs = ::std::sqrt(rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__sqrt` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_isnan:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __isnan integer ; always false
                      rhs = false;
                    }
                    else if(rhs.is_real()) {
                      // __isnan real
                      rhs = ::std::isnan(rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__isnan` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_isinf:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __isinf integer ; always false
                      rhs = false;
                    }
                    else if(rhs.is_real()) {
                      // __isinf real
                      rhs = ::std::isinf(rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__isinf` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_abs:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __abs integer ; may overflow
                      V_integer& val = rhs.open_integer();
                      V_integer negv;
                      if(ROCKET_SUB_OVERFLOW(0, val, &negv))
                        throw Runtime_Error(xtc_format,
                           "Integer absolute value overflow (operand was `$1`)", val);
                      val = ::std::max(val, negv);
                    }
                    else if(rhs.is_real()) {
                      // __abs real
                      V_real& val = rhs.open_real();
                      val = ::std::fabs(val);
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__abs` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sign:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __sign integer
                      rhs = rhs.as_integer() < 0;
                    }
                    else if(rhs.is_real()) {
                      // __sign real
                      rhs = ::std::signbit(rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__sign` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_round:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __round integer ; no-op
                      (void) rhs;
                    }
                    else if(rhs.is_real()) {
                      // __round real
                      rhs = ::std::round(rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__round` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_floor:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __floor integer ; no-op
                      (void) rhs;
                    }
                    else if(rhs.is_real()) {
                      // __floor real
                      rhs = ::std::floor(rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__floor` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_ceil:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __ceil integer ; no-op
                      (void) rhs;
                    }
                    else if(rhs.is_real()) {
                      // __ceil real
                      rhs = ::std::ceil(rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__ceil` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_trunc:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __trunc integer ; no-op
                      (void) rhs;
                    }
                    else if(rhs.is_real()) {
                      // __trunc real
                      rhs = ::std::trunc(rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__trunc` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_iround:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __iround integer ; no-op
                      (void) rhs;
                    }
                    else if(rhs.is_real()) {
                      // __iround real
                      rhs = safe_double_to_int64(::std::round(rhs.as_real()));
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__iround` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_ifloor:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __ifloor integer ; no-op
                      (void) rhs;
                    }
                    else if(rhs.is_real()) {
                      // __ifloor real
                      rhs = safe_double_to_int64(::std::floor(rhs.as_real()));
                    }
                    else throw Runtime_Error(xtc_format,
                             "`__ifloor` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_iceil:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __iceil integer ; no-op
                      (void) rhs;
                    }
                    else if(rhs.is_real()) {
                      // __iceil real
                      rhs = safe_double_to_int64(::std::ceil(rhs.as_real()));
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__iceil` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_itrunc:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __itrunc integer ; no-op
                      (void) rhs;
                    }
                    else if(rhs.is_real()) {
                      // __itrunc real
                      rhs = safe_double_to_int64(::std::trunc(rhs.as_real()));
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__itrunc` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_lzcnt:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __lzcnt integer
                      V_integer& val = rhs.open_integer();
                      val = ROCKET_LZCNT64(static_cast<uint64_t>(val));
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__lzcnt` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_tzcnt:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __tzcnt integer
                      V_integer& val = rhs.open_integer();
                      val = ROCKET_TZCNT64(static_cast<uint64_t>(val));
                    }
                    else throw Runtime_Error(xtc_format,
                             "`__tzcnt` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_popcnt:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.is_integer()) {
                      // __popcnt integer
                      V_integer& val = rhs.open_integer();
                      val = ROCKET_POPCNT64(static_cast<uint64_t>(val));
                    }
                    else throw Runtime_Error(xtc_format,
                             "`__popcnt` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_eq:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const auto& rhs = ctx.stack().top().dereference_readonly();
                    auto& top = ctx.stack().mut_top(1);
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // ==
                    Compare cmp = lhs.compare_partial(rhs);
                    lhs = cmp == compare_equal;

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_ne:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // != ; not ==
                    Compare cmp = lhs.compare_partial(rhs);
                    lhs = cmp != compare_equal;

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_un:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // </> ; not <= and not >=
                    Compare cmp = lhs.compare_partial(rhs);
                    lhs = cmp == compare_unordered;

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_lt:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // <
                    Compare cmp = lhs.compare_total(rhs);
                    lhs = cmp == compare_less;

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_gt:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // >
                    Compare cmp = lhs.compare_total(rhs);
                    lhs = cmp == compare_greater;

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_lte:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // <= ; not >
                    Compare cmp = lhs.compare_total(rhs);
                    lhs = cmp != compare_greater;

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_gte:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // < ; not >=
                    Compare cmp = lhs.compare_total(rhs);
                    lhs = cmp != compare_less;

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_3way:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // <=>
                    Compare cmp = lhs.compare_partial(rhs);
                    if(cmp == compare_unordered)
                      lhs = &"[unordered]";
                    else
                      lhs = static_cast<int64_t>(cmp) - compare_equal;

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_add:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // integer + integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_ADD_OVERFLOW(val, rhs.as_integer(), &result))
                        throw Runtime_Error(xtc_format,
                           "Integer addition overflow (operands were `$1` and `$2`)", val, rhs.as_integer());
                      val = result;
                    }
                    else if(lhs.is_real() && rhs.is_real()) {
                      // real + real ; can't overflow
                      V_real& val = lhs.open_real();
                      val += rhs.as_real();
                    }
                    else if(lhs.is_boolean() && rhs.is_boolean()) {
                      // boolean + boolean ; same as |
                      V_boolean& val = lhs.open_boolean();
                      val |= rhs.as_boolean();
                    }
                    else if(lhs.is_string() && rhs.is_string()) {
                      // string + string
                      V_string& val = lhs.open_string();
                      val += rhs.as_string();
                    }
                    else throw Runtime_Error(xtc_format,
                            "Addition not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sub:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // integer - integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_SUB_OVERFLOW(val, rhs.as_integer(), &result))
                        throw Runtime_Error(xtc_format,
                           "Integer subtraction overflow (operands were `$1` and `$2`)", val, rhs.as_integer());
                      val = result;
                    }
                    else if(lhs.is_real() && rhs.is_real()) {
                      // real - real ; can't overflow
                      V_real& val = lhs.open_real();
                      val -= rhs.as_real();
                    }
                    else if(lhs.is_boolean() && rhs.is_boolean()) {
                      // boolean - boolean ; same as ^
                      V_boolean& val = lhs.open_boolean();
                      val ^= rhs.as_boolean();
                    }
                    else throw Runtime_Error(xtc_format,
                            "Subtraction not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_mul:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // integer * integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_MUL_OVERFLOW(val, rhs.as_integer(), &result))
                        throw Runtime_Error(xtc_format,
                           "Integer multiplication overflow (operands were `$1` and `$2`)", val, rhs.as_integer());
                      val = result;
                    }
                    else if(lhs.is_real() && rhs.is_real()) {
                      // real * real ; can't overflow
                      V_real& val = lhs.open_real();
                      val *= rhs.as_real();
                    }
                    else if(lhs.is_boolean() && rhs.is_boolean()) {
                      // boolean * boolean ; same as &
                      V_boolean& val = lhs.open_boolean();
                      val &= rhs.as_boolean();
                    }
                    else if(lhs.is_string() && rhs.is_integer()) {
                      // string * integer
                      V_string& val = lhs.open_string();
                      val = do_duplicate_sequence(val, rhs.as_integer());
                    }
                    else if(lhs.is_array() && rhs.is_integer()) {
                      // array * integer
                      V_array& val = lhs.open_array();
                      val = do_duplicate_sequence(val, rhs.as_integer());
                    }
                    else if(lhs.is_integer() && rhs.is_string()) {
                      // integer * string
                      V_string temp = do_duplicate_sequence(rhs.as_string(), lhs.as_integer());
                      lhs = move(temp);
                    }
                    else if(lhs.is_integer() && rhs.is_array()) {
                      // integer * array
                      V_array temp = do_duplicate_sequence(rhs.as_array(), lhs.as_integer());
                      lhs = move(temp);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Multiplication not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_div:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // integer / integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      if(rhs.as_integer() == 0)
                        throw Runtime_Error(xtc_format,
                           "Integer division by zero (operands were `$1` and `$2`)", val, rhs.as_integer());
                      if((val == INT64_MIN) && (rhs.as_integer() == -1))
                        throw Runtime_Error(xtc_format,
                           "Integer division overflow (operands were `$1` and `$2`)", val, rhs.as_integer());
                      val /= rhs.as_integer();
                    }
                    else if(lhs.is_real() && rhs.is_real()) {
                      // real / real ; can't overflow
                      V_real& val = lhs.open_real();
                      val /= rhs.as_real();
                    }
                    else if(lhs.is_string() && rhs.is_string()) {
                      // string / string ; path concatenation
                      V_string& val = lhs.open_string();
                      val += '/';
                      val += rhs.as_string();
                    }
                    else throw Runtime_Error(xtc_format,
                            "Division not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_mod:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // integer % integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      if(rhs.as_integer() == 0)
                        throw Runtime_Error(xtc_format,
                           "Integer division by zero (operands were `$1` and `$2`)", val, rhs.as_integer());
                      if((val == INT64_MIN) && (rhs.as_integer() == -1))
                        throw Runtime_Error(xtc_format,
                           "Integer division overflow (operands were `$1` and `$2`)", val, rhs.as_integer());
                      val %= rhs.as_integer();
                    }
                    else if(lhs.is_real() && rhs.is_real()) {
                      // real % real ; can't overflow
                      V_real& val = lhs.open_real();
                      val = ::std::fmod(val, rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "Modulo not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_andb:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // integer & integer
                      V_integer& val = lhs.open_integer();
                      val &= rhs.as_integer();
                    }
                    else if(lhs.is_boolean() && rhs.is_boolean()) {
                      // boolean & boolean
                      V_boolean& val = lhs.open_boolean();
                      val &= rhs.as_boolean();
                    }
                    else if(lhs.is_string() && rhs.is_string()) {
                      // string & string ; bitwise truncation
                      V_string& val = lhs.open_string();
                      const V_string& mask = rhs.as_string();
                      if(val.size() > mask.size())
                        val.erase(mask.size());
                      auto mc = mask.begin();
                      for(auto it = val.mut_begin();  it != val.end();  ++it, ++mc)
                        *it = static_cast<char>(*it & *mc);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Bitwise AND not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_orb:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // integer | integer
                      V_integer& val = lhs.open_integer();
                      val |= rhs.as_integer();
                    }
                    else if(lhs.is_boolean() && rhs.is_boolean()) {
                      // boolean & boolean
                      V_boolean& val = lhs.open_boolean();
                      val |= rhs.as_boolean();
                    }
                    else if(lhs.is_string() && rhs.is_string()) {
                      // string | string ; bitwise extension
                      V_string& val = lhs.open_string();
                      const V_string& mask = rhs.as_string();
                      if(val.size() < mask.size())
                        val.append(mask.size() - val.size(), 0);
                      auto it = val.mut_begin();
                      for(auto mc = mask.begin();  mc != mask.end();  ++it, ++mc)
                        *it = static_cast<char>(*it | *mc);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Bitwise OR not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_xorb:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // integer ^ integer
                      V_integer& val = lhs.open_integer();
                      val ^= rhs.as_integer();
                    }
                    else if(lhs.is_boolean() && rhs.is_boolean()) {
                      // boolean ^ boolean
                      V_boolean& val = lhs.open_boolean();
                      val ^= rhs.as_boolean();
                    }
                    else if(lhs.is_string() && rhs.is_string()) {
                      // string ^ string ; bitwise flipping
                      V_string& val = lhs.open_string();
                      const V_string& mask = rhs.as_string();
                      if(val.size() < mask.size())
                        val.append(mask.size() - val.size(), 0);
                      auto it = val.mut_begin();
                      for(auto mc = mask.begin();  mc != mask.end();  ++it, ++mc)
                        *it = static_cast<char>(*it ^ *mc);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Bitwise XOR not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_addm:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // __addm integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      ROCKET_ADD_OVERFLOW(val, rhs.as_integer(), &result);
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Modular addition not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_subm:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // __subm integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      ROCKET_SUB_OVERFLOW(val, rhs.as_integer(), &result);
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Modular subtraction not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_mulm:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // __mulm integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      ROCKET_MUL_OVERFLOW(val, rhs.as_integer(), &result);
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Modular multiplication not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_adds:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // __adds integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_ADD_OVERFLOW(val, rhs.as_integer(), &result))
                        result = (val >> 63) ^ INT64_MAX;
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Saturating addition not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_subs:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // __subs integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_SUB_OVERFLOW(val, rhs.as_integer(), &result))
                        result = (val >> 63) ^ INT64_MAX;
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Saturating subtraction not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_muls:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer() && rhs.is_integer()) {
                      // __muls integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_MUL_OVERFLOW(val, rhs.as_integer(), &result))
                        result = (val >> 63) ^ (rhs.as_integer() >> 63) ^ INT64_MAX;
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Saturating multiplication not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sll:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.type() != type_integer)
                      throw Runtime_Error(xtc_format,
                         "Invalid shift count (operands were `$1` and `$2`)", lhs, rhs);

                    if(rhs.as_integer() < 0)
                      throw Runtime_Error(xtc_format,
                         "Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

                    if(lhs.is_integer()) {
                      // integer <<< ; bitwise, fixed-width
                      V_integer& val = lhs.open_integer();
                      int64_t n = rhs.as_integer();
                      reinterpret_cast<uint64_t&>(val) <<= n;
                      reinterpret_cast<uint64_t&>(val) <<= n != rhs.as_integer();
                    }
                    else if(lhs.is_string()) {
                      // string <<< ; bytewise, fixed-width
                      V_string& val = lhs.open_string();
                      size_t n = ::rocket::clamp_cast<size_t>(rhs.as_integer(), 0, val.ssize());
                      val.erase(0, n);
                      val.append(n, '\0');
                    }
                    else if(lhs.is_array()) {
                      // array <<< ; element-wise, fixed-width
                      V_array& val = lhs.open_array();
                      size_t n = ::rocket::clamp_cast<size_t>(rhs.as_integer(), 0, val.ssize());
                      val.erase(0, n);
                      val.append(n);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Logical left shift not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_srl:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.type() != type_integer)
                      throw Runtime_Error(xtc_format,
                         "Invalid shift count (operands were `$1` and `$2`)", lhs, rhs);

                    if(rhs.as_integer() < 0)
                      throw Runtime_Error(xtc_format,
                         "Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

                    if(lhs.is_integer()) {
                      // integer >>> ; bitwise, fixed-width
                      V_integer& val = lhs.open_integer();
                      int64_t n = ::rocket::min(rhs.as_integer(), 63);
                      reinterpret_cast<uint64_t&>(val) >>= n;
                      reinterpret_cast<uint64_t&>(val) >>= n != rhs.as_integer();
                    }
                    else if(lhs.is_string()) {
                      // string >>> ; bytewise, fixed-width
                      V_string& val = lhs.open_string();
                      size_t n = ::rocket::clamp_cast<size_t>(rhs.as_integer(), 0, val.ssize());
                      val.pop_back(n);
                      val.insert(0, n, '\0');
                    }
                    else if(lhs.is_array()) {
                      // array >>> ; element-wise, fixed-width
                      V_array& val = lhs.open_array();
                      size_t n = ::rocket::clamp_cast<size_t>(rhs.as_integer(), 0, val.ssize());
                      val.pop_back(n);
                      val.insert(0, n);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Logical right shift not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sla:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.type() != type_integer)
                      throw Runtime_Error(xtc_format,
                         "Invalid shift count (operands were `$1` and `$2`)", lhs, rhs);

                    if(rhs.as_integer() < 0)
                      throw Runtime_Error(xtc_format,
                         "Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

                    if(lhs.is_integer()) {
                      // integer <<< ; bitwise, variable-width, may overflow
                      V_integer& val = lhs.open_integer();
                      int64_t n = ::rocket::min(rhs.as_integer(), 63);
                      if((val != 0) && ((n != rhs.as_integer()) || (val >> (63 - n) != val >> 63)))
                        throw Runtime_Error(xtc_format,
                           "Arithmetic left shift overflow (operands were `$1` and `$2`)", lhs, rhs);
                      reinterpret_cast<uint64_t&>(val) <<= n;
                    }
                    else if(lhs.is_string()) {
                      // string <<< ; bytewise, variable-width
                      V_string& val = lhs.open_string();
                      size_t n = ::rocket::clamp_cast<size_t>(rhs.as_integer(), 0, PTRDIFF_MAX);
                      val.append(n, '\0');
                    }
                    else if(lhs.is_array()) {
                      // array <<< ; element-wise, variable-width
                      V_array& val = lhs.open_array();
                      size_t n = ::rocket::clamp_cast<size_t>(rhs.as_integer(), 0, PTRDIFF_MAX);
                      val.append(n);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Arithmetic left shift not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sra:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.type() != type_integer)
                      throw Runtime_Error(xtc_format,
                         "Invalid shift count (operands were `$1` and `$2`)", lhs, rhs);

                    if(rhs.as_integer() < 0)
                      throw Runtime_Error(xtc_format,
                         "Negative shift count (operands were `$1` and `$2`)", lhs, rhs);

                    if(lhs.is_integer()) {
                      // integer <<< ; bitwise, variable-width, may overflow
                      V_integer& val = lhs.open_integer();
                      int64_t n = ::rocket::min(rhs.as_integer(), 63);
                      val >>= n;
                    }
                    else if(lhs.is_string()) {
                      // string <<< ; bytewise, variable-width
                      V_string& val = lhs.open_string();
                      size_t n = ::rocket::clamp_cast<size_t>(rhs.as_integer(), 0, val.ssize());
                      val.pop_back(n);
                    }
                    else if(lhs.is_array()) {
                      // array <<< ; element-wise, variable-width
                      V_array& val = lhs.open_array();
                      size_t n = ::rocket::clamp_cast<size_t>(rhs.as_integer(), 0, val.ssize());
                      val.pop_back(n);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Arithmetic right shift not applicable (operands were `$1` and `$2`)", lhs, rhs);

                    ctx.stack().pop();
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_fma:
              // fused multiply-add; ternary
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    const Value& mid = ctx.stack().top(1).dereference_readonly();
                    Reference& top = ctx.stack().mut_top(2);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_real() && mid.is_real() && rhs.is_real()) {
                      // __fma ; always real
                      V_real& val = lhs.open_real();
                      val = ::std::fma(val, mid.as_real(), rhs.as_real());
                    }
                    else throw Runtime_Error(xtc_format,
                            "`__fma` not applicable (operands were `$1`, `$2` and `$3`)", lhs, mid, rhs);

                    ctx.stack().pop(2);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            default:
              ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), altr.xop);
            }
        }

      case index_unpack_array:
        {
          const auto& altr = this->m_stor.as<S_unpack_array>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.immutable;
          up2.u2345 = altr.nelems;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const bool immutable = head->uparam.b0;
                const uint32_t nelems = head->uparam.u2345;

                // Read the value of the initializer.
                auto init = ctx.stack().top().dereference_readonly();
                ctx.stack().pop();
                if(!init.is_null() && !init.is_array())
                  throw Runtime_Error(xtc_format,
                           "Initializer was not an array (value was `$1`)", init);

                for(uint32_t i = nelems - 1;  i != UINT32_MAX;  --i) {
                  // Pop variables from from right to left.
                  auto var = ctx.stack().top().unphase_variable_opt();
                  ctx.stack().pop();
                  ROCKET_ASSERT(var && !var->is_initialized());

                  if(ROCKET_EXPECT(init.is_array()))
                    if(auto ielem = init.as_array().ptr(i))
                      var->initialize(*ielem);

                  if(ROCKET_UNEXPECT(!var->is_initialized()))
                    var->initialize(nullopt);

                  var->set_immutable(immutable);
                }
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_unpack_object:
        {
          const auto& altr = this->m_stor.as<S_unpack_object>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.immutable;

          struct Sparam
            {
              cow_vector<phcow_string> keys;
            };

          Sparam sp2;
          sp2.keys = altr.keys;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const bool immutable = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read the value of the initializer.
                auto init = ctx.stack().top().dereference_readonly();
                ctx.stack().pop();
                if(!init.is_null() && !init.is_object())
                  throw Runtime_Error(xtc_format,
                           "Initializer was not an object (value was `$1`)", init);

                for(auto it = sp.keys.rbegin();  it != sp.keys.rend();  ++it) {
                  // Pop variables from from right to left.
                  auto var = ctx.stack().top().unphase_variable_opt();
                  ctx.stack().pop();
                  ROCKET_ASSERT(var && !var->is_initialized());

                  if(ROCKET_EXPECT(init.is_object()))
                    if(auto ielem = init.as_object().ptr(*it))
                      var->initialize(*ielem);

                  if(ROCKET_UNEXPECT(!var->is_initialized()))
                    var->initialize(nullopt);

                  var->set_immutable(immutable);
                }
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_define_null_variable:
        {
          const auto& altr = this->m_stor.as<S_define_null_variable>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.immutable;

          struct Sparam
            {
              phcow_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool immutable = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = *(head->pv_meta->sloc_opt);

                // Allocate a variable and inject it into the current context.
                const auto gcoll = ctx.global().garbage_collector();
                const auto var = gcoll->create_variable();
                ctx.insert_named_reference(sp.name).set_variable(var);
                ctx.global().call_hook(&Abstract_Hooks::on_declare, sloc, sp.name);

                // Initialize it to null.
                var->initialize(nullopt);
                var->set_immutable(immutable);
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_single_step_trap:
        {
          const auto& altr = this->m_stor.as<S_single_step_trap>();

          (void) altr;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                ctx.global().call_hook(&Abstract_Hooks::on_trap, *(head->pv_meta->sloc_opt), ctx);
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_variadic_call:
        {
          const auto& altr = this->m_stor.as<S_variadic_call>();

          AVM_Rod::Uparam up2;
          up2.u0 = altr.ptc;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const PTC_Aware ptc = static_cast<PTC_Aware>(head->uparam.u0);
                const auto& sloc = *(head->pv_meta->sloc_opt);
                const auto sentry = ctx.global().copy_recursion_sentry();

                auto temp_value = ctx.stack().top().dereference_readonly();
                if(temp_value.is_null()) {
                  // There is no argument for the target function.
                  ctx.alt_stack().clear();
                  ctx.stack().pop();
                }
                else if(temp_value.is_array()) {
                  // Push all values from left to right, as temporary values.
                  ctx.alt_stack().clear();
                  for(const auto& val : temp_value.as_array())
                    ctx.alt_stack().push().set_temporary(val);
                  ctx.stack().pop();
                }
                else if(temp_value.is_function()) {
                  // Invoke the generator function with no argument to get the number
                  // of variadic arguments. This destroys its self reference, so we have
                  // to stash it first.
                  auto va_generator = temp_value.as_function();
                  ctx.stack().mut_top().pop_subscript();
                  ctx.stack().push();
                  ctx.stack().mut_top() = ctx.stack().mut_top(1);
                  ctx.alt_stack().clear();
                  do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc_aware_none, va_generator);
                  temp_value = ctx.stack().top().dereference_readonly();
                  ctx.stack().pop();

                  if(temp_value.type() != type_integer)
                    throw Runtime_Error(xtc_format,
                             "Invalid number of variadic arguments (value `$1`)", temp_value);

                  if((temp_value.as_integer() < 0) || (temp_value.as_integer() > INT_MAX))
                    throw Runtime_Error(xtc_format,
                             "Invalid number of variadic arguments (value `$1`)", temp_value);

                  uint32_t nargs = static_cast<uint32_t>(temp_value.as_integer());
                  if(nargs == 0) {
                    // There is no argument for the target function.
                    ctx.alt_stack().clear();
                    ctx.stack().pop();
                  }
                  else {
                    // Initialize `this` references for variadic arguments.
                    for(uint32_t k = 0;  k != nargs - 1;  ++k) {
                      ctx.stack().push();
                      ctx.stack().mut_top() = ctx.stack().top(1);
                    }

                    // Generate varaidic arguments, and store them on `stack` from
                    // right to left for later use.
                    for(uint32_t k = 0;  k != nargs;  ++k) {
                      ctx.alt_stack().clear();
                      ctx.alt_stack().push().set_temporary(V_integer(k));
                      do_invoke_partial(ctx.stack().mut_top(k), ctx, sloc, ptc_aware_none, va_generator);
                      ctx.stack().top(k).dereference_readonly();
                    }

                    // Move arguments into `alt_stack` from left to right.
                    ctx.alt_stack().clear();
                    for(uint32_t k = 0;  k != nargs;  ++k)
                      ctx.alt_stack().push() = move(ctx.stack().mut_top(k));
                    ctx.stack().pop(nargs);
                  }
                }
                else
                  throw Runtime_Error(xtc_format,
                           "Invalid variadic argument generator (value `$1`)", temp_value);

                // Invoke the target function.
                auto target = do_get_target_function(ctx.stack().top());
                ctx.stack().mut_top().pop_subscript();
                do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc, target);
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_defer_expression:
        {
          const auto& altr = this->m_stor.as<S_defer_expression>();

          struct Sparam
            {
              cow_vector<AIR_Node> code_body;
            };

          Sparam sp2;
          sp2.code_body = altr.code_body;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = *(head->pv_meta->sloc_opt);

                // Capture local references at this time.
                bool dirty = false;
                auto bound_body = sp.code_body;
                do_rebind_nodes(dirty, bound_body, ctx);

                // Instantiate the expression and push it to the current context.
                AVM_Rod rod_body;
                do_solidify_nodes(rod_body, bound_body);
                ctx.mut_defer().emplace_back(sloc, move(rod_body));
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                do_collect_variables_for_each(staged, temp, sp.code_body);
              }

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_import_call:
        {
          const auto& altr = this->m_stor.as<S_import_call>();

          AVM_Rod::Uparam up2;
          up2.u2345 = altr.nargs;

          struct Sparam
            {
              Compiler_Options opts;
            };

          Sparam sp2;
          sp2.opts = altr.opts;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const uint32_t nargs = head->uparam.u2345;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = *(head->pv_meta->sloc_opt);
                const auto sentry = ctx.global().copy_recursion_sentry();

                // Collect arguments from left to right.
                ctx.alt_stack().clear();
                for(uint32_t k = nargs - 2;  k != UINT32_MAX;  --k)
                  ctx.alt_stack().push() = move(ctx.stack().mut_top(k));
                ctx.stack().pop(nargs - 1);

                // Get the path to import.
                const auto& path_val = ctx.stack().top().dereference_readonly();
                if(path_val.type() != type_string)
                  throw Runtime_Error(xtc_format,
                           "Path was not a string (value `$1`)", path_val);

                if(path_val.as_string() == "")
                  throw Runtime_Error(xtc_format, "Path was empty");

                cow_string abs_path = path_val.as_string();
                if(abs_path[0] != '/') {
                  const auto& src_file = sloc.file();
                  if(src_file[0] == '/') {
                    size_t pos = src_file.rfind('/');
                    ROCKET_ASSERT(pos != cow_string::npos);
                    abs_path.insert(0, src_file.c_str(), pos + 1);
                  }
                }

                abs_path = get_real_path(abs_path);
                const Module_Loader::Unique_Stream istrm(ctx.global().module_loader(), abs_path);

                Token_Stream tstrm(sp.opts);
                tstrm.reload(abs_path, 1, move(istrm.file()));
                Statement_Sequence stmtq(sp.opts);
                stmtq.reload(move(tstrm));

                // Instantiate the script as a variadic function.
                AIR_Optimizer optmz(sp.opts);
                cow_vector<phcow_string> script_params;
                script_params.emplace_back(&"...");
                optmz.reload(nullptr, script_params, ctx.global(), stmtq.get_statements());

                // Invoke it without `this`.
                Source_Location script_sloc(abs_path, 0, 0);
                auto target = optmz.create_function(script_sloc, &"[file scope]");
                ctx.stack().mut_top().set_void();
                do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc_aware_none, target);
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_declare_reference:
        {
          const auto& altr = this->m_stor.as<S_declare_reference>();

          struct Sparam
            {
              phcow_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Declare a void reference.
                ctx.insert_named_reference(sp.name).clear();
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_initialize_reference:
        {
          const auto& altr = this->m_stor.as<S_initialize_reference>();

          struct Sparam
            {
              phcow_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Move a reference from the stack into the current context.
                ctx.insert_named_reference(sp.name) = move(ctx.stack().mut_top());
                ctx.stack().pop();
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_catch_expression:
        {
          const auto& altr = this->m_stor.as<S_catch_expression>();

          struct Sparam
            {
              AVM_Rod rod_body;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_body, altr.code_body);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__cold__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Save the current partial expression.
                Reference_Stack saved_stack;
                ctx.stack().swap(saved_stack);

                // Evaluate the operand in a `try` block.
                Value exval;
                try {
                  sp.rod_body.execute(ctx);
                  ROCKET_ASSERT(ctx.status() == air_status_next);
                }
                catch(Runtime_Error& except) {
                  exval = except.value();
                }

                // Restore the original partial expression, then push the exception
                // value back onto it.
                ctx.stack().swap(saved_stack);
                ctx.stack().push().set_temporary(move(exval));
                ctx.status() = air_status_next;
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_return_statement:
        {
          const auto& altr = this->m_stor.as<S_return_statement>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.by_ref;
          up2.b1 = altr.is_void;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool by_ref = head->uparam.b0;
                const bool is_void = head->uparam.b1;
                const auto& sloc = *(head->pv_meta->sloc_opt);

                if(!is_void && !ctx.stack().top().is_void()) {
                  // Ensure the result is dereferenceable. If it is to be
                  // returned by value, also convert it to a temporary value.
                  if(by_ref)
                    ctx.stack().top().dereference_readonly();
                  else
                    ctx.stack().mut_top().dereference_copy();
                }

                ctx.global().call_hook(&Abstract_Hooks::on_return, sloc, ptc_aware_none);
                ctx.status() = is_void ? air_status_return_void : air_status_return;
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_constant:
        {
          const auto& altr = this->m_stor.as<S_push_constant>();

          struct Sparam
            {
              Value val;
            };

          Sparam sp2;
          sp2.val = altr.val;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Push a temporary copy of the constant.
                ctx.stack().push().set_temporary(sp.val);
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.val.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_alt_clear_stack:
        {
          const auto& altr = this->m_stor.as<S_alt_clear_stack>();

          (void) altr;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* /*head*/)
              __attribute__((__hot__, __flatten__))
              {
                ctx.swap_stacks();
                ctx.stack().clear();
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_alt_function_call:
        {
          const auto& altr = this->m_stor.as<S_alt_function_call>();

          AVM_Rod::Uparam up2;
          up2.u0 = altr.ptc;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const PTC_Aware ptc = static_cast<PTC_Aware>(head->uparam.u0);
                const auto& sloc = *(head->pv_meta->sloc_opt);
                const auto sentry = ctx.global().copy_recursion_sentry();

                ctx.swap_stacks();

                // Invoke the target function.
                auto target = do_get_target_function(ctx.stack().top());
                ctx.stack().mut_top().pop_subscript();
                do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc, target);
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_coalesce_expression:
        {
          const auto& altr = this->m_stor.as<S_coalesce_expression>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.assign;

          struct Sparam
            {
              AVM_Rod rod_null;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_null, altr.code_null);

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool assign = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read the condition, and if it is null, evaluate the backup.
                if(ctx.stack().top().dereference_readonly().is_null())
                  do_evaluate_subexpression(ctx, assign, sp.rod_null);
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const AVM_Rod::Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_null.collect_variables(staged, temp);
              }

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_member_access:
        {
          const auto& altr = this->m_stor.as<S_member_access>();

          struct Sparam
            {
              phcow_string key;
            };

          Sparam sp2;
          sp2.key = altr.key;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Push a subscript.
                Subscript::S_object_key xsub = { sp.key };
                do_push_subscript_and_check(ctx.stack().mut_top(), move(xsub));
              }

            // Uparam
            , AVM_Rod::Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_apply_operator_bi32:
        {
          const auto& altr = this->m_stor.as<S_apply_operator_bi32>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.assign;
          up2.i2345 = altr.irhs;

          switch(altr.xop)
            {
            case xop_inc:
            case xop_dec:
            case xop_unset:
            case xop_head:
            case xop_tail:
            case xop_random:
            case xop_isvoid:
            case xop_pos:
            case xop_neg:
            case xop_notb:
            case xop_notl:
            case xop_countof:
            case xop_typeof:
            case xop_sqrt:
            case xop_isnan:
            case xop_isinf:
            case xop_abs:
            case xop_sign:
            case xop_round:
            case xop_floor:
            case xop_ceil:
            case xop_trunc:
            case xop_iround:
            case xop_ifloor:
            case xop_iceil:
            case xop_itrunc:
            case xop_lzcnt:
            case xop_tzcnt:
            case xop_popcnt:
            case xop_fma:
              ASTERIA_TERMINATE(("Constant folding not implemented for `$1`"), altr.xop);

            case xop_assign:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();

                    // x = y ; `assign` ignored
                    top.dereference_mutable() = irhs;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_index:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();

                    Subscript::S_array_index xsub = { irhs };
                    do_push_subscript_and_check(top, move(xsub));
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_eq:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // ==
                    Compare cmp = lhs.compare_numeric_partial(irhs);
                    lhs = cmp == compare_equal;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_ne:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // != ; not ==
                    Compare cmp = lhs.compare_numeric_partial(irhs);
                    lhs = cmp != compare_equal;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_un:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // </> ; not <= and not >=
                    Compare cmp = lhs.compare_numeric_partial(irhs);
                    lhs = cmp == compare_unordered;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_lt:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // <
                    Compare cmp = lhs.compare_numeric_total(irhs);
                    lhs = cmp == compare_less;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_gt:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // >
                    Compare cmp = lhs.compare_numeric_total(irhs);
                    lhs = cmp == compare_greater;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_lte:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // <= ; not >
                    Compare cmp = lhs.compare_numeric_total(irhs);
                    lhs = cmp != compare_greater;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_gte:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // < ; not >=
                    Compare cmp = lhs.compare_numeric_total(irhs);
                    lhs = cmp != compare_less;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_3way:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // <=>
                    Compare cmp = lhs.compare_numeric_partial(irhs);
                    if(cmp == compare_unordered)
                      lhs = &"[unordered]";
                    else
                      lhs = static_cast<int64_t>(cmp) - compare_equal;
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_add:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // integer + integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_ADD_OVERFLOW(val, irhs, &result))
                        throw Runtime_Error(xtc_format,
                           "Integer addition overflow (operands were `$1` and `$2`)", val, irhs);
                      val = result;
                    }
                    else if(lhs.is_real()) {
                      // real + real ; can't overflow
                      V_real& val = lhs.open_real();
                      val += static_cast<V_real>(irhs);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Addition not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sub:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // integer - integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_SUB_OVERFLOW(val, irhs, &result))
                        throw Runtime_Error(xtc_format,
                           "Integer subtraction overflow (operands were `$1` and `$2`)", val, irhs);
                      val = result;
                    }
                    else if(lhs.is_real()) {
                      // real - real ; can't overflow
                      V_real& val = lhs.open_real();
                      val -= static_cast<V_real>(irhs);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Subtraction not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_mul:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // integer * integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_MUL_OVERFLOW(val, irhs, &result))
                        throw Runtime_Error(xtc_format,
                           "Integer multiplication overflow (operands were `$1` and `$2`)", val, irhs);
                      val = result;
                    }
                    else if(lhs.is_real()) {
                      // real * real ; can't overflow
                      V_real& val = lhs.open_real();
                      val *= static_cast<V_real>(irhs);
                    }
                    else if(lhs.is_string()) {
                      // string * integer
                      V_string& val = lhs.open_string();
                      val = do_duplicate_sequence(val, irhs);
                    }
                    else if(lhs.is_array()) {
                      // array * integer
                      V_array& val = lhs.open_array();
                      val = do_duplicate_sequence(val, irhs);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Multiplication not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_div:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // integer / integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      if(irhs == 0)
                        throw Runtime_Error(xtc_format,
                           "Integer division by zero (operands were `$1` and `$2`)", val, irhs);
                      if((val == INT64_MIN) && (irhs == -1))
                        throw Runtime_Error(xtc_format,
                           "Integer division overflow (operands were `$1` and `$2`)", val, irhs);
                      val /= irhs;
                    }
                    else if(lhs.is_real()) {
                      // real / real ; can't overflow
                      V_real& val = lhs.open_real();
                      val /= static_cast<V_real>(irhs);;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Division not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_mod:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // integer % integer ; may overflow
                      V_integer& val = lhs.open_integer();
                      if(irhs == 0)
                        throw Runtime_Error(xtc_format,
                           "Integer division by zero (operands were `$1` and `$2`)", val, irhs);
                      if((val == INT64_MIN) && (irhs == -1))
                        throw Runtime_Error(xtc_format,
                           "Integer division overflow (operands were `$1` and `$2`)", val, irhs);
                      val %= irhs;
                    }
                    else if(lhs.is_real()) {
                      // real % real ; can't overflow
                      V_real& val = lhs.open_real();
                      val = ::std::fmod(val, static_cast<V_real>(irhs));
                    }
                    else throw Runtime_Error(xtc_format,
                            "Modulo not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_andb:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // integer & integer
                      V_integer& val = lhs.open_integer();
                      val &= irhs;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Bitwise AND not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_orb:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // integer | integer
                      V_integer& val = lhs.open_integer();
                      val |= irhs;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Bitwise OR not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_xorb:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // integer ^ integer
                      V_integer& val = lhs.open_integer();
                      val ^= irhs;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Bitwise XOR not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_addm:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // __addm integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      ROCKET_ADD_OVERFLOW(val, irhs, &result);
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Modular addition not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_subm:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // __subm integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      ROCKET_SUB_OVERFLOW(val, irhs, &result);
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Modular subtraction not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_mulm:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // __mulm integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      ROCKET_MUL_OVERFLOW(val, irhs, &result);
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Modular multiplication not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_adds:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // __adds integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_ADD_OVERFLOW(val, irhs, &result))
                        result = (val >> 63) ^ INT64_MAX;
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Saturating addition not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_subs:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // __subs integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_SUB_OVERFLOW(val, irhs, &result))
                        result = (val >> 63) ^ INT64_MAX;
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Saturating subtraction not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_muls:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(lhs.is_integer()) {
                      // __muls integer
                      V_integer& val = lhs.open_integer();
                      V_integer result;
                      if(ROCKET_MUL_OVERFLOW(val, irhs, &result))
                        result = (val >> 63) ^ (irhs >> 63) ^ INT64_MAX;
                      val = result;
                    }
                    else throw Runtime_Error(xtc_format,
                            "Saturating multiplication not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sll:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(irhs < 0)
                      throw Runtime_Error(xtc_format,
                         "Negative shift count (operands were `$1` and `$2`)", lhs, irhs);

                    if(lhs.is_integer()) {
                      // integer <<< ; bitwise, fixed-width
                      V_integer& val = lhs.open_integer();
                      int64_t n = irhs;
                      reinterpret_cast<uint64_t&>(val) <<= n;
                      reinterpret_cast<uint64_t&>(val) <<= n != irhs;
                    }
                    else if(lhs.is_string()) {
                      // string <<< ; bytewise, fixed-width
                      V_string& val = lhs.open_string();
                      size_t n = ::rocket::clamp_cast<size_t>(irhs, 0, val.ssize());
                      val.erase(0, n);
                      val.append(n, '\0');
                    }
                    else if(lhs.is_array()) {
                      // array <<< ; element-wise, fixed-width
                      V_array& val = lhs.open_array();
                      size_t n = ::rocket::clamp_cast<size_t>(irhs, 0, val.ssize());
                      val.erase(0, n);
                      val.append(n);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Logical left shift not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_srl:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(irhs < 0)
                      throw Runtime_Error(xtc_format,
                         "Negative shift count (operands were `$1` and `$2`)", lhs, irhs);

                    if(lhs.is_integer()) {
                      // integer >>> ; bitwise, fixed-width
                      V_integer& val = lhs.open_integer();
                      int64_t n = ::rocket::min(irhs, 63);
                      reinterpret_cast<uint64_t&>(val) >>= n;
                      reinterpret_cast<uint64_t&>(val) >>= n != irhs;
                    }
                    else if(lhs.is_string()) {
                      // string >>> ; bytewise, fixed-width
                      V_string& val = lhs.open_string();
                      size_t n = ::rocket::clamp_cast<size_t>(irhs, 0, val.ssize());
                      val.pop_back(n);
                      val.insert(0, n, '\0');
                    }
                    else if(lhs.is_array()) {
                      // array >>> ; element-wise, fixed-width
                      V_array& val = lhs.open_array();
                      size_t n = ::rocket::clamp_cast<size_t>(irhs, 0, val.ssize());
                      val.pop_back(n);
                      val.insert(0, n);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Logical right shift not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sla:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(irhs < 0)
                      throw Runtime_Error(xtc_format,
                         "Negative shift count (operands were `$1` and `$2`)", lhs, irhs);

                    if(lhs.is_integer()) {
                      // integer <<< ; bitwise, variable-width, may overflow
                      V_integer& val = lhs.open_integer();
                      int64_t n = ::rocket::min(irhs, 63);
                      if((val != 0) && ((n != irhs) || (val >> (63 - n) != val >> 63)))
                        throw Runtime_Error(xtc_format,
                           "Arithmetic left shift overflow (operands were `$1` and `$2`)", lhs, irhs);
                      reinterpret_cast<uint64_t&>(val) <<= n;
                    }
                    else if(lhs.is_string()) {
                      // string <<< ; bytewise, variable-width
                      V_string& val = lhs.open_string();
                      size_t n = ::rocket::clamp_cast<size_t>(irhs, 0, PTRDIFF_MAX);
                      val.append(n, '\0');
                    }
                    else if(lhs.is_array()) {
                      // array <<< ; element-wise, variable-width
                      V_array& val = lhs.open_array();
                      size_t n = ::rocket::clamp_cast<size_t>(irhs, 0, PTRDIFF_MAX);
                      val.append(n);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Arithmetic left shift not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sra:
              rod.push_function(
                +[](Executive_Context& ctx, const AVM_Rod::Header* head)
                  __attribute__((__hot__, __flatten__))
                  {
                    const bool assign = head->uparam.b0;
                    const V_integer irhs = head->uparam.i2345;
                    Reference& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(irhs < 0)
                      throw Runtime_Error(xtc_format,
                         "Negative shift count (operands were `$1` and `$2`)", lhs, irhs);

                    if(lhs.is_integer()) {
                      // integer <<< ; bitwise, variable-width, may overflow
                      V_integer& val = lhs.open_integer();
                      int64_t n = ::rocket::min(irhs, 63);
                      val >>= n;
                    }
                    else if(lhs.is_string()) {
                      // string <<< ; bytewise, variable-width
                      V_string& val = lhs.open_string();
                      size_t n = ::rocket::clamp_cast<size_t>(irhs, 0, val.ssize());
                      val.pop_back(n);
                    }
                    else if(lhs.is_array()) {
                      // array <<< ; element-wise, variable-width
                      V_array& val = lhs.open_array();
                      size_t n = ::rocket::clamp_cast<size_t>(irhs, 0, val.ssize());
                      val.pop_back(n);
                    }
                    else throw Runtime_Error(xtc_format,
                            "Arithmetic right shift not applicable (operands were `$1` and `$2`)", lhs, irhs);
                  }

                // Uparam
                , up2
                , 0, nullptr, nullptr, nullptr
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            default:
              ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), altr.xop);
            }
        }

      case index_return_statement_bi32:
        {
          const auto& altr = this->m_stor.as<S_return_statement_bi32>();

          AVM_Rod::Uparam up2;
          up2.u0 = altr.type;
          up2.i2345 = altr.irhs;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const Type type = static_cast<Type>(head->uparam.u0);
                const V_integer irhs = head->uparam.i2345;
                const auto& sloc = *(head->pv_meta->sloc_opt);

                // Push the result as a temporary value.
                if(type == type_null)
                  ctx.stack().push().set_temporary(nullopt);
                else if(type == type_boolean)
                  ctx.stack().push().set_temporary(-irhs < 0);
                else
                  ctx.stack().push().set_temporary(irhs);

                ctx.global().call_hook(&Abstract_Hooks::on_return, sloc, ptc_aware_none);
                ctx.status() = air_status_return;
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_check_null:
        {
          const auto& altr = this->m_stor.as<S_check_null>();

          AVM_Rod::Uparam up2;
          up2.b0 = altr.negative;

          rod.push_function(
            +[](Executive_Context& ctx, const AVM_Rod::Header* head)
              __attribute__((__hot__, __flatten__))
              {
                const bool negative = head->uparam.b0;
                Reference& top = ctx.stack().mut_top();

                // Set the result as a temporary value.
                bool value = top.dereference_readonly().is_null() != negative;
                top.set_temporary(value);
              }

            // Uparam
            , up2
            , 0, nullptr, nullptr, nullptr
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }
    }
  }

}  // namespace asteria
