// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "expression_unit.hpp"
#include "statement.hpp"
#include "compiler_error.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/air_optimizer.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/enums.hpp"
#include "../utils.hpp"
namespace asteria {

bool
Expression_Unit::
clobbers_alt_stack() const noexcept
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_literal:
      case index_local_reference:
      case index_closure_function:
      case index_operator_rpn:
      case index_unnamed_array:
      case index_unnamed_object:
      case index_global_reference:
      case index_check_argument:
        return false;

      case index_variadic_call:
      case index_import_call:
      case index_function_call:
      case index_catch:
        return true;

      case index_branch:
        return ::rocket::any_of(this->m_stor.as<S_branch>().branches,
            [](const branch& br) {
              return ::rocket::any_of(br.units,
                  [](const Expression_Unit& unit) { return unit.clobbers_alt_stack();  });
            });

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

bool
Expression_Unit::
may_be_void() const noexcept
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_literal:
      case index_closure_function:
      case index_operator_rpn:
      case index_unnamed_array:
      case index_unnamed_object:
      case index_check_argument:
      case index_catch:
        return false;

      case index_local_reference:
      case index_global_reference:
      case index_function_call:
      case index_variadic_call:
      case index_import_call:
        return true;

      case index_branch:
        return ::rocket::any_of(this->m_stor.as<S_branch>().branches,
            [](const branch& br) {
              return br.units.size() && br.units.back().may_be_void();
            });

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

void
Expression_Unit::
generate_code(cow_vector<AIR_Node>& code, const Compiler_Options& opts,
              const Global_Context& global, const Analytic_Context& ctx,
              PTC_Aware ptc) const
  {
    if(!code.empty() && code.back().is_terminator())
      return;

    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_literal:
        {
          const auto& altr = this->m_stor.as<S_literal>();

          // Encode a constant.
          AIR_Node::S_push_constant xnode = { altr.value };
          code.emplace_back(move(xnode));
          return;
        }

      case index_local_reference:
        {
          const auto& altr = this->m_stor.as<S_local_reference>();

          // Perform early lookup when the expression is defined.
          // If a named reference is found, it will not be replaced or hidden by a
          // later-declared one.
          const Abstract_Context* qctx = &ctx;
          uint32_t depth = 0;

          for(;;) {
            // Look for the name in the current context.
            auto qref = qctx->get_named_reference_opt(altr.name);
            if(qref) {
              // A reference declared later has been found.
              // Record the context depth for later lookups.
              AIR_Node::S_push_local_reference xnode = { altr.sloc, (uint16_t) depth, altr.name };
              code.emplace_back(move(xnode));
              return;
            }

            // Step out to its parent context.
            qctx = qctx->get_parent_opt();
            if(!qctx) {
              // No name has been found so far.
              // Assume that the name will be found in the global context.
              if(!opts.implicit_global_names) {
                // If implicit global names are not allowed, it must be resolved now.
                qref = global.get_named_reference_opt(altr.name);
                if(!qref)
                  throw Compiler_Error(xtc_format,
                            compiler_status_undeclared_identifier, altr.sloc,
                            "Undeclared identifier `$1`", altr.name);
              }

              AIR_Node::S_push_global_reference xnode = { altr.sloc, altr.name };
              code.emplace_back(move(xnode));
              return;
            }

            // Search in the outer context.
            ++ depth;
            if(depth > 0xFFFF)
              throw Compiler_Error(xtc_status,
                        compiler_status_too_many_nested_levels, altr.sloc);
          }
        }

      case index_closure_function:
        {
          const auto& altr = this->m_stor.as<S_closure_function>();

          // Generate code
          AIR_Optimizer optmz(opts);
          optmz.reload(&ctx, altr.params, global, altr.body);

          AIR_Node::S_define_function xnode = { opts, altr.sloc, altr.unique_name, altr.params,
                                                optmz.get_code() };
          code.emplace_back(move(xnode));
          return;
        }

      case index_branch:
        {
          const auto& altr = this->m_stor.as<S_branch>();

          // Generate code for branches.
          array<cow_vector<AIR_Node>, 2> code_branches;
          for(size_t k = 0;  k < altr.branches.size();  ++k)
            for(size_t i = 0;  i < altr.branches.at(k).units.size();  ++i)
              altr.branches.at(k).units.at(i).generate_code(code_branches.mut(k), opts, global, ctx,
                                      ((i != altr.branches.at(k).units.size() - 1) || altr.assign)
                                        ? ptc_aware_none : ptc);

          if((altr.branches.size() == 1) && (altr.branches.at(0).type == branch_type_null)) {
            // Encode a coalescence node.
            AIR_Node::S_coalesce_expression xnode = { altr.sloc, move(code_branches.mut(0)),
                                                      altr.assign };
            code.emplace_back(move(xnode));
            return;
          }

          // Encode a conditional branch node.
          AIR_Node::S_branch_expression xnode;
          xnode.sloc = altr.sloc;
          xnode.assign = altr.assign;

          for(size_t k = 0;  k < altr.branches.size();  ++k)
            if(altr.branches.at(k).type == branch_type_true)
              xnode.code_true = move(code_branches.mut(k));
            else if(altr.branches.at(k).type == branch_type_false)
              xnode.code_false = move(code_branches.mut(k));
            else
              ASTERIA_TERMINATE(("Invalid branch type `$1`"), altr.branches.at(k).type);

          code.emplace_back(move(xnode));
          return;
        }

      case index_function_call:
        {
          const auto& altr = this->m_stor.as<S_function_call>();

          // Check whether PTC is disabled.
          auto rptc = opts.proper_tail_calls ? ptc : ptc_aware_none;

          if((opts.optimization_level >= 1)
             && ::rocket::none_of(altr.args,  // read: none of `args` clobbers `alt_stack`
                    [](const argument& arg) {
                      return ::rocket::any_of(arg.units,
                          [](const Expression_Unit& unit) { return unit.clobbers_alt_stack();  });
                    })) {
            // Evaluate arguments and store results directly on the alternative
            // stack, so they need not be moved around.
            // https://github.com/lhmouse/asteria/issues/136
            AIR_Node::S_alt_clear_stack xstart = { };
            code.emplace_back(move(xstart));

            for(const auto& arg : altr.args)
              for(const auto& unit : arg.units)
                unit.generate_code(code, opts, global, ctx, ptc_aware_none);

            AIR_Node::S_alt_function_call xnode = { altr.sloc, rptc };
            code.emplace_back(move(xnode));
            return;
          }

          // Evaluate arguments and store results on the argument stack, which will
          // be popped into the alternative stack by `S_function_call`.
          for(const auto& arg : altr.args)
            for(const auto& unit : arg.units)
              unit.generate_code(code, opts, global, ctx, ptc_aware_none);

          AIR_Node::S_function_call xnode = { altr.sloc, (uint32_t) altr.args.size(), rptc };
          code.emplace_back(move(xnode));
          return;
        }

      case index_operator_rpn:
        {
          const auto& altr = this->m_stor.as<S_operator_rpn>();

          if(opts.optimization_level >= 1) {
            // Can this be folded?
            opt<Value> qrhs;
            if(!code.empty())
              qrhs = code.back().get_constant_opt();

            if(qrhs) {
              if((altr.xop == xop_pos) && !altr.assign)
                return;

              if(qrhs->is_string() && (altr.xop == xop_index)) {
                // Encode a pre-hashed object key.
                AIR_Node::S_member_access xnode = { altr.sloc, phsh_string(qrhs->as_string()) };
                code.mut_back() = move(xnode);
                return;
              }

              if(qrhs->is_integer() && (altr.xop == xop_notb)) {
                // Encode the result directly, as the result will never overflow.
                AIR_Node::S_push_constant xnode = { qrhs->as_integer() ^ -1 };
                code.mut_back() = move(xnode);
                return;
              }

              if(qrhs->is_integer() && ((int32_t) qrhs->as_integer() == qrhs->as_integer())
                 && ::rocket::is_any_of(altr.xop,
                       { xop_assign, xop_index, xop_cmp_eq, xop_cmp_ne, xop_cmp_un, xop_cmp_lt,
                         xop_cmp_gt, xop_cmp_lte, xop_cmp_gte, xop_cmp_3way, xop_add, xop_sub,
                         xop_mul, xop_div, xop_mod, xop_andb, xop_orb, xop_xorb, xop_addm,
                         xop_subm, xop_mulm, xop_adds, xop_subs, xop_muls, xop_sll, xop_srl,
                         xop_sla, xop_sra })) {
                // Fold this constant.
                AIR_Node::S_apply_operator_bi32 xnode = { altr.sloc, altr.xop, altr.assign,
                                                          (int32_t) qrhs->as_integer() };
                code.mut_back() = move(xnode);
                return;
              }
            }
          }

          AIR_Node::S_apply_operator xnode = { altr.sloc, altr.xop, altr.assign };
          code.emplace_back(move(xnode));
          return;
        }

      case index_unnamed_array:
        {
          const auto& altr = this->m_stor.as<S_unnamed_array>();

          AIR_Node::S_push_unnamed_array xnode = { altr.sloc, altr.nelems };
          code.emplace_back(move(xnode));
          return;
        }

      case index_unnamed_object:
        {
          const auto& altr = this->m_stor.as<S_unnamed_object>();

          AIR_Node::S_push_unnamed_object xnode = { altr.sloc, altr.keys };
          code.emplace_back(move(xnode));
          return;
        }

      case index_global_reference:
        {
          const auto& altr = this->m_stor.as<S_global_reference>();

          // This name is always looked up in the global context.
          AIR_Node::S_push_global_reference xnode = { altr.sloc, altr.name };
          code.emplace_back(move(xnode));
          return;
        }

      case index_variadic_call:
        {
          const auto& altr = this->m_stor.as<S_variadic_call>();

          // Arguments can be evaluated directly on the stack.
          for(const auto& arg : altr.args)
            for(const auto& unit : arg.units)
              unit.generate_code(code, opts, global, ctx, ptc_aware_none);

          // Check whether PTC is disabled.
          auto rptc = opts.proper_tail_calls ? ptc : ptc_aware_none;

          AIR_Node::S_variadic_call xnode = { altr.sloc, rptc };
          code.emplace_back(move(xnode));
          return;
        }

      case index_check_argument:
        {
          const auto& altr = this->m_stor.as<S_check_argument>();

          AIR_Node::S_check_argument xnode = { altr.sloc, altr.by_ref };
          code.emplace_back(move(xnode));
          return;
        }

      case index_import_call:
        {
          const auto& altr = this->m_stor.as<S_import_call>();

          // Arguments can be evaluated directly on the stack.
          for(const auto& arg : altr.args)
            for(const auto& unit : arg.units)
              unit.generate_code(code, opts, global, ctx, ptc_aware_none);

          AIR_Node::S_import_call xnode = { opts, altr.sloc, (uint32_t) altr.args.size() };
          code.emplace_back(move(xnode));
          return;
        }

      case index_catch:
        {
          const auto& altr = this->m_stor.as<S_catch>();

          // Generate code for the operand, which shall be evaluated on a separate
          // context and shall not be PTC'd.
          cow_vector<AIR_Node> code_op;
          for(const auto& unit : altr.operand)
            unit.generate_code(code_op, opts, global, ctx, ptc_aware_none);

          AIR_Node::S_catch_expression xnode = { move(code_op) };
          code.emplace_back(move(xnode));
          return;
        }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

}  // namespace asteria
