// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
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
namespace {

cow_vector<AIR_Node>
do_generate_code_branch(const Compiler_Options& opts, const Global_Context& global,
                        Analytic_Context& ctx, PTC_Aware ptc,
                        const cow_vector<Expression_Unit>& units)
  {
    cow_vector<AIR_Node> code;
    if(units.empty())
      return code;

    // Expression units other than the last one cannot be PTC'd.
    for(size_t i = 0;  i + 1 < units.size();  ++i)
      units.at(i).generate_code(code, opts, global, ctx, ptc_aware_none);

    units.back().generate_code(code, opts, global, ctx, ptc);
    return code;
  }

}  // namespace

cow_vector<AIR_Node>&
Expression_Unit::
generate_code(cow_vector<AIR_Node>& code, const Compiler_Options& opts,
              const Global_Context& global, Analytic_Context& ctx, PTC_Aware ptc) const
  {
    switch(this->index()) {
      case index_literal: {
        const auto& altr = this->m_stor.as<index_literal>();

        // Copy the value as is.
        AIR_Node::S_push_temporary xnode = { altr.value };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_local_reference: {
        const auto& altr = this->m_stor.as<index_local_reference>();

        // Perform early lookup when the expression is defined.
        // If a named reference is found, it will not be replaced or hidden by a
        // later-declared one.
        const Reference* qref;
        Abstract_Context* qctx = &ctx;
        uint32_t depth = 0;

        for(;;) {
          // Look for the name in the current context.
          size_t hint = SIZE_MAX;
          qref = qctx->get_named_reference_with_hint_opt(hint, altr.name);
          if(qref) {
            // A reference declared later has been found.
            // Record the context depth for later lookups.
            AIR_Node::S_push_local_reference xnode = { altr.sloc, depth, hint, altr.name };
            code.emplace_back(::std::move(xnode));
            return code;
          }

          // Step out to its parent context.
          qctx = qctx->get_parent_opt();
          if(!qctx) {
            // No name has been found so far.
            // Assume that the name will be found in the global context.
            hint = SIZE_MAX;
            if(!opts.implicit_global_names) {
              // If implicit global names are not allowed, it must be resolved now.
              qref = global.get_named_reference_with_hint_opt(hint, altr.name);
              if(!qref)
                throw Compiler_Error(Compiler_Error::M_format(),
                          compiler_status_undeclared_identifier, altr.sloc,
                          "undeclared identifier `$1`", altr.name);
            }

            AIR_Node::S_push_global_reference xnode = { altr.sloc, hint, altr.name };
            code.emplace_back(::std::move(xnode));
            return code;
          }

          // Search in the outer context.
          ++depth;
        }
      }

      case index_closure_function: {
        const auto& altr = this->m_stor.as<index_closure_function>();

        // Generate code
        AIR_Optimizer optmz(opts);
        optmz.reload(&ctx, altr.params, global, altr.body);

        // Encode arguments.
        AIR_Node::S_define_function xnode = { opts, altr.sloc, altr.unique_name, altr.params,
                                              optmz };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_branch: {
        const auto& altr = this->m_stor.as<index_branch>();

        // Both branches may be PTC'd unless this is a compound assignment operation.
        auto rptc = altr.assign ? ptc_aware_none : ptc;

        // Generate code for both branches.
        auto code_true = do_generate_code_branch(opts, global, ctx, rptc, altr.branch_true);
        auto code_false = do_generate_code_branch(opts, global, ctx, rptc, altr.branch_false);

        // Encode arguments.
        AIR_Node::S_branch_expression xnode = { altr.sloc, ::std::move(code_true),
                                                ::std::move(code_false), altr.assign };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_function_call: {
        const auto& altr = this->m_stor.as<index_function_call>();

        // Check whether PTC is disabled.
        auto rptc = !opts.proper_tail_calls ? ptc_aware_none : ptc;

        // Encode arguments.
        AIR_Node::S_function_call xnode = { altr.sloc, altr.nargs, rptc };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_member_access: {
        const auto& altr = this->m_stor.as<index_member_access>();

        // Encode arguments.
        AIR_Node::S_member_access xnode = { altr.sloc, altr.name };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_operator_rpn: {
        const auto& altr = this->m_stor.as<index_operator_rpn>();

        // Encode arguments.
        AIR_Node::S_apply_operator xnode = { altr.sloc, altr.xop, altr.assign };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_unnamed_array: {
        const auto& altr = this->m_stor.as<index_unnamed_array>();

        // Encode arguments.
        AIR_Node::S_push_unnamed_array xnode = { altr.sloc, altr.nelems };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_unnamed_object: {
        const auto& altr = this->m_stor.as<index_unnamed_object>();

        // Encode arguments.
        AIR_Node::S_push_unnamed_object xnode = { altr.sloc, altr.keys };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();

        // The branch may be PTC'd unless this is a compound assignment operation.
        auto rptc = altr.assign ? ptc_aware_none : ptc;

        // Generate code for the branch.
        auto code_null = do_generate_code_branch(opts, global, ctx, rptc, altr.branch_null);

        // Encode arguments.
        AIR_Node::S_coalescence xnode = { altr.sloc, ::std::move(code_null), altr.assign };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_global_reference: {
        const auto& altr = this->m_stor.as<index_global_reference>();

        // This name is always looked up in the global context.
        AIR_Node::S_push_global_reference xnode = { altr.sloc, UINT16_MAX, altr.name };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_variadic_call: {
        const auto& altr = this->m_stor.as<index_variadic_call>();

        // Encode arguments.
        AIR_Node::S_variadic_call xnode = { altr.sloc, opts.proper_tail_calls
                                                         ? ptc : ptc_aware_none };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_argument_finish: {
        const auto& altr = this->m_stor.as<index_argument_finish>();

        // Encode arguments.
        AIR_Node::S_check_argument xnode = { altr.sloc, altr.by_ref };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_import_call: {
        const auto& altr = this->m_stor.as<index_import_call>();

        // Encode arguments.
        AIR_Node::S_import_call xnode = { opts, altr.sloc, altr.nargs };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_catch: {
        const auto& altr = this->m_stor.as<index_catch>();

        // Generate code for the operand, which shall be evaluated on a separate
        // context and shall not be PTC'd.
        auto code_op = do_generate_code_branch(opts, global, ctx, ptc_aware_none, altr.operand);

        // Encode arguments.
        AIR_Node::S_catch_expression xnode = { ::std::move(code_op) };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      default:
        ASTERIA_TERMINATE("invalid expression unit type (index `$1`)", this->index());
    }
  }

}  // namespace asteria
