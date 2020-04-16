// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "expression_unit.hpp"
#include "statement.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/air_optimizer.hpp"
#include "../runtime/enums.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

cow_vector<AIR_Node>
do_generate_code_branch(const Compiler_Options& opts, PTC_Aware ptc, const Analytic_Context& ctx,
                        const cow_vector<Expression_Unit>& units)
  {
    // Expression units other than the last one cannot be PTC'd.
    cow_vector<AIR_Node> code;
    for(size_t i = 0;  i < units.size();  ++i) {
      auto qnext = units.get_ptr(i + 1);
      units[i].generate_code(code, opts, ctx, qnext ? ptc_aware_none : ptc);
    }
    return code;
  }

}  // namespace

cow_vector<AIR_Node>&
Expression_Unit::
generate_code(cow_vector<AIR_Node>& code, const Compiler_Options& opts,
              const Analytic_Context& ctx, PTC_Aware ptc)
const
  {
    switch(this->index()) {
      case index_literal: {
        const auto& altr = this->m_stor.as<index_literal>();

        // Encode arguments.
        AIR_Node::S_push_immediate xnode = { altr.val };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_named_reference: {
        const auto& altr = this->m_stor.as<index_named_reference>();

        // Perform early lookup when the expression is defined.
        // If a named reference is found, it will not be replaced or hidden by a later-declared one.
        const Reference* qref;
        const Abstract_Context* qctx = ::std::addressof(ctx);
        uint32_t depth = 0;
        for(;;) {
          // Look for the name in the current context.
          qref = qctx->get_named_reference_opt(altr.name);
          if(qref) {
            // A reference declared later has been found. Record the context depth for later lookups.
            AIR_Node::S_push_local_reference xnode = { altr.sloc, depth, altr.name };
            code.emplace_back(::std::move(xnode));
            return code;
          }
          // Step out to its parent context.
          qctx = qctx->get_parent_opt();
          if(!qctx) {
            // No name has been found so far. Assume that the name will be found in the global context.
            AIR_Node::S_push_global_reference xnode = { altr.sloc, altr.name };
            code.emplace_back(::std::move(xnode));
            return code;
          }
          ++depth;
        }
      }

      case index_closure_function: {
        const auto& altr = this->m_stor.as<index_closure_function>();

        // Generate code
        AIR_Optimizer optmz(opts);
        optmz.reload(&ctx, altr.params, altr.body);

        // Encode arguments.
        AIR_Node::S_define_function xnode = { opts, altr.sloc, altr.unique_name, altr.params, optmz };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_branch: {
        const auto& altr = this->m_stor.as<index_branch>();

        // Both branches may be PTC'd unless this is a compound assignment operation.
        auto real_ptc = altr.assign ? ptc_aware_none : ptc;

        // Generate code for both branches.
        auto code_true = do_generate_code_branch(opts, real_ptc, ctx, altr.branch_true);
        auto code_false = do_generate_code_branch(opts, real_ptc, ctx, altr.branch_false);

        // Encode arguments.
        AIR_Node::S_branch_expression xnode = { altr.sloc, ::std::move(code_true), ::std::move(code_false),
                                                altr.assign };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_function_call: {
        const auto& altr = this->m_stor.as<index_function_call>();

        // Check whether PTC is disabled.
        auto real_ptc = !opts.proper_tail_calls ? ptc_aware_none : ptc;

        // Encode arguments.
        AIR_Node::S_function_call xnode = { altr.sloc, altr.nargs, real_ptc };
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
        auto real_ptc = altr.assign ? ptc_aware_none : ptc;

        // Generate code for the branch.
        auto code_null = do_generate_code_branch(opts, real_ptc, ctx, altr.branch_null);

        // Encode arguments.
        AIR_Node::S_coalescence xnode = { altr.sloc, ::std::move(code_null), altr.assign };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_global_reference: {
        const auto& altr = this->m_stor.as<index_global_reference>();

        // This name is always looked up in the global context.
        AIR_Node::S_push_global_reference xnode = { altr.sloc, altr.name };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_variadic_call: {
        const auto& altr = this->m_stor.as<index_variadic_call>();

        // Encode arguments.
        AIR_Node::S_variadic_call xnode = { altr.sloc, opts.proper_tail_calls ? ptc : ptc_aware_none };
        code.emplace_back(::std::move(xnode));
        return code;
      }

      case index_argument_finish: {
        const auto& altr = this->m_stor.as<index_argument_finish>();

        // Apply glvalue-to-rvalue conversion if the argument is to be passed by value.
        if(altr.by_ref)
          return code;

        // Encode arguments.
        AIR_Node::S_glvalue_to_prvalue xnode = { altr.sloc };
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

      default:
        ASTERIA_TERMINATE("invalid expression unit type (index `$1`)", this->index());
    }
  }

}  // namespace Asteria
