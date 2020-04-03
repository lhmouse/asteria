// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "expression_unit.hpp"
#include "statement.hpp"
#include "../runtime/enums.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/analytic_context.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

cow_vector<AIR_Node> do_generate_code_branch(const Compiler_Options& opts, PTC_Aware ptc,
                                             const Analytic_Context& ctx, const cow_vector<Expression_Unit>& units)
  {
    // Expression units other than the last one cannot be PTC'd.
    cow_vector<AIR_Node> code;
    for(size_t i = 0;  i < units.size();  ++i)
      units[i].generate_code(code, opts, ctx,
                   (i + 1 == units.size()) ? ptc     // last expression unit
                                           : ptc_aware_none);
    return code;
  }

}  // namespace

cow_vector<AIR_Node>& Expression_Unit::generate_code(cow_vector<AIR_Node>& code, const Compiler_Options& opts,
                                                     const Analytic_Context& ctx, PTC_Aware ptc) const
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
            AIR_Node::S_push_local_reference xnode = { depth, altr.name };
            code.emplace_back(::std::move(xnode));
            return code;
          }
          // Step out to its parent context.
          qctx = qctx->get_parent_opt();
          if(!qctx) {
            // No name has been found so far. Assume that the name will be found in the global context.
            AIR_Node::S_push_global_reference xnode = { altr.name };
            code.emplace_back(::std::move(xnode));
            return code;
          }
          ++depth;
        }
      }

    case index_closure_function: {
        const auto& altr = this->m_stor.as<index_closure_function>();

        // Name the closure.
        ::rocket::tinyfmt_str fmt;
        fmt << "<closure>._" << altr.unique_id << '(';
        // Append the parameter list. Parameters are separated by commas.
        for(size_t i = 0;  i < altr.params.size();  ++i)
          ((i == 0) ? fmt : (fmt << ", ")) << altr.params[i];
        fmt << ')';

        // Encode arguments.
        AIR_Node::S_define_function xnode = { altr.sloc, fmt.extract_string(), altr.params,
                                              do_generate_function(opts, altr.params, &ctx, altr.body) };
        code.emplace_back(::std::move(xnode));
        return code;
      }

    case index_branch: {
        const auto& altr = this->m_stor.as<index_branch>();

        // Generate code for both branches.
        // Both branches may be PTC'd.
        auto code_true = do_generate_code_branch(opts, ptc, ctx, altr.branch_true);
        auto code_false = do_generate_code_branch(opts, ptc, ctx, altr.branch_false);

        // Encode arguments.
        AIR_Node::S_branch_expression xnode = { ::std::move(code_true), ::std::move(code_false), altr.assign };
        code.emplace_back(::std::move(xnode));
        return code;
      }

    case index_function_call: {
        const auto& altr = this->m_stor.as<index_function_call>();

        // Encode arguments.
        AIR_Node::S_function_call xnode = { altr.sloc, altr.nargs, opts.proper_tail_calls ? ptc : ptc_aware_none };
        code.emplace_back(::std::move(xnode));
        return code;
      }

    case index_member_access: {
        const auto& altr = this->m_stor.as<index_member_access>();

        // Encode arguments.
        AIR_Node::S_member_access xnode = { altr.name };
        code.emplace_back(::std::move(xnode));
        return code;
      }

    case index_operator_rpn: {
        const auto& altr = this->m_stor.as<index_operator_rpn>();

        // Encode arguments.
        AIR_Node::S_apply_operator xnode = { altr.xop, altr.assign };
        code.emplace_back(::std::move(xnode));
        return code;
      }

    case index_unnamed_array: {
        const auto& altr = this->m_stor.as<index_unnamed_array>();

        // Encode arguments.
        AIR_Node::S_push_unnamed_array xnode = { altr.nelems };
        code.emplace_back(::std::move(xnode));
        return code;
      }

    case index_unnamed_object: {
        const auto& altr = this->m_stor.as<index_unnamed_object>();

        // Encode arguments.
        AIR_Node::S_push_unnamed_object xnode = { altr.keys };
        code.emplace_back(::std::move(xnode));
        return code;
      }

    case index_coalescence: {
        const auto& altr = this->m_stor.as<index_coalescence>();

        // Generate code for the branch. This branch may be PTC'd.
        auto code_null = do_generate_code_branch(opts, ptc, ctx, altr.branch_null);

        // Encode arguments.
        AIR_Node::S_coalescence xnode = { ::std::move(code_null), altr.assign };
        code.emplace_back(::std::move(xnode));
        return code;
      }

    case index_global_reference: {
        const auto& altr = this->m_stor.as<index_global_reference>();

        // This name is always looked up in the global context.
        AIR_Node::S_push_global_reference xnode = { altr.name };
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
        if(!altr.by_ref) {
          // Encode arguments.
          AIR_Node::S_glvalue_to_rvalue xnode = { };
          code.emplace_back(::std::move(xnode));
        }
        return code;
      }

    case index_import_call: {
        const auto& altr = this->m_stor.as<index_import_call>();

        // Encode arguments.
        AIR_Node::S_import_call xnode = { altr.sloc, altr.nargs, opts };
        code.emplace_back(::std::move(xnode));
        return code;
      }

    default:
      ASTERIA_TERMINATE("invalid expression unit type (index `$1`)", this->index());
    }
  }

}  // namespace Asteria
