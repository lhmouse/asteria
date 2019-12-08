// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "xprunit.hpp"
#include "statement.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/analytic_context.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    cow_string do_name_closure(long line)
      {
        tinyfmt_str fmt;
        fmt << "<closure>._" << line;
        return fmt.extract_string();
      }

    cow_vector<AIR_Node> do_generate_code_branch(const Compiler_Options& opts, TCO_Aware tco_aware, const Analytic_Context& ctx,
                                                 const cow_vector<Xprunit>& units)
      {
        cow_vector<AIR_Node> code;
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

    }

cow_vector<AIR_Node>& Xprunit::generate_code(cow_vector<AIR_Node>& code,
                                             const Compiler_Options& opts, TCO_Aware tco_aware, const Analytic_Context& ctx) const
  {
    switch(this->index()) {
      {{
    case index_literal:
        const auto& altr = this->m_stor.as<index_literal>();
        // Encode arguments.
        AIR_Node::S_push_literal xnode = { altr.val };
        code.emplace_back(rocket::move(xnode));
        return code;
      }{
    case index_named_reference:
        const auto& altr = this->m_stor.as<index_named_reference>();
        // Perform early lookup when the expression is defined.
        // If a named reference is found, it will not be replaced or hidden by a later-declared one.
        const Abstract_Context* qctx = std::addressof(ctx);
        uint32_t depth = 0;
        const Reference* qref;
        for(;;) {
          // Look for the name in the current context.
          qref = qctx->get_named_reference_opt(altr.name);
          if(qref) {
            break;
          }
          // Step out to its parent context.
          qctx = qctx->get_parent_opt();
          if(!qctx) {
            break;
          }
          ++depth;
        }
        if(!qref) {
          // No name has been found so far. Assume that the name will be found in the global context later.
          AIR_Node::S_push_global_reference xnode = { altr.name };
          code.emplace_back(rocket::move(xnode));
        }
        else if(qctx->is_analytic()) {
          // A reference declared later has been found. Record the context depth for later lookups.
          AIR_Node::S_push_local_reference xnode = { depth, altr.name };
          code.emplace_back(rocket::move(xnode));
        }
        else {
          // A reference declared previously has been found. Bind it immediately.
          AIR_Node::S_push_bound_reference xnode = { *qref };
          code.emplace_back(rocket::move(xnode));
        }
        return code;
      }{
    case index_closure_function:
        const auto& altr = this->m_stor.as<index_closure_function>();
        // Encode arguments.
        AIR_Node::S_instantiate_function xnode = { opts, altr.sloc, do_name_closure(altr.sloc.line()), altr.params, altr.body };
        code.emplace_back(rocket::move(xnode));
        return code;
      }{
    case index_branch:
        const auto& altr = this->m_stor.as<index_branch>();
        // Generate code for both branches.
        // Both branches may be TCO'd.
        auto code_true = do_generate_code_branch(opts, tco_aware, ctx, altr.branch_true);
        auto code_false = do_generate_code_branch(opts, tco_aware, ctx, altr.branch_false);
        // Encode arguments.
        AIR_Node::S_branch_expression xnode = { rocket::move(code_true), rocket::move(code_false), altr.assign };
        code.emplace_back(rocket::move(xnode));
        return code;
      }{
    case index_function_call:
        const auto& altr = this->m_stor.as<index_function_call>();
        // Encode arguments.
        AIR_Node::S_function_call xnode = { altr.sloc, altr.args_by_refs, opts.no_proper_tail_calls ? tco_aware_none : tco_aware };
        code.emplace_back(rocket::move(xnode));
        return code;
      }{
    case index_member_access:
        const auto& altr = this->m_stor.as<index_member_access>();
        // Encode arguments.
        AIR_Node::S_member_access xnode = { altr.name };
        code.emplace_back(rocket::move(xnode));
        return code;
      }{
    case index_operator_rpn:
        const auto& altr = this->m_stor.as<index_operator_rpn>();
        // Encode arguments.
        AIR_Node::S_apply_operator xnode = { altr.xop, altr.assign };
        code.emplace_back(rocket::move(xnode));
        return code;
      }{
    case index_unnamed_array:
        const auto& altr = this->m_stor.as<index_unnamed_array>();
        // Encode arguments.
        AIR_Node::S_push_unnamed_array xnode = { altr.nelems };
        code.emplace_back(rocket::move(xnode));
        return code;
      }{
    case index_unnamed_object:
        const auto& altr = this->m_stor.as<index_unnamed_object>();
        // Encode arguments.
        AIR_Node::S_push_unnamed_object xnode = { altr.keys };
        code.emplace_back(rocket::move(xnode));
        return code;
      }{
    case index_coalescence:
        const auto& altr = this->m_stor.as<index_coalescence>();
        // Generate code for the branch.
        // This branch may be TCO'd.
        auto code_null = do_generate_code_branch(opts, tco_aware, ctx, altr.branch_null);
        // Encode arguments.
        AIR_Node::S_coalescence xnode = { rocket::move(code_null), altr.assign };
        code.emplace_back(rocket::move(xnode));
        return code;
      }{
    case index_operator_fma:
        const auto& altr = this->m_stor.as<index_operator_fma>();
        // Encode arguments.
        AIR_Node::S_apply_operator xnode = { xop_fma_3, altr.assign };
        code.emplace_back(rocket::move(xnode));
        return code;
      }}
    default:
      ASTERIA_TERMINATE("invalid expression unit type (index `$1`)", this->index());
    }
  }

}  // namespace Asteria
