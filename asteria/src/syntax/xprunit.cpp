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

    cow_vector<AIR_Node> do_generate_code_branch(const Compiler_Options& options, TCO_Aware tco_aware, const Analytic_Context& ctx,
                                                 const cow_vector<Xprunit>& units)
      {
        cow_vector<AIR_Node> code;
        size_t epos = units.size() - 1;
        if(epos != SIZE_MAX) {
          // Expression units other than the last one cannot be TCO'd.
          for(size_t i = 0; i != epos; ++i) {
            units[i].generate_code(code, options, tco_aware_none, ctx);
          }
          // The last unit may be TCO'd.
          units[epos].generate_code(code, options, tco_aware, ctx);
        }
        return code;
      }

    }

cow_vector<AIR_Node>& Xprunit::generate_code(cow_vector<AIR_Node>& code,
                                             const Compiler_Options& options, TCO_Aware tco_aware, const Analytic_Context& ctx) const
  {
    switch(this->index()) {
    case index_literal:
      {
        const auto& altr = this->m_stor.as<index_literal>();
        // Encode arguments.
        AIR_Node::S_push_literal xnode = { altr.val };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_named_reference:
      {
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
          AIR_Node::S_push_local_reference xnode = { altr.name, depth };
          code.emplace_back(rocket::move(xnode));
        }
        else {
          // A reference declared previously has been found. Bind it immediately.
          AIR_Node::S_push_bound_reference xnode = { *qref };
          code.emplace_back(rocket::move(xnode));
        }
        return code;
      }
    case index_closure_function:
      {
        const auto& altr = this->m_stor.as<index_closure_function>();
        // Encode arguments.
        AIR_Node::S_define_function xnode = { options, altr.sloc, rocket::sref("<closure>"), altr.params, altr.body };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_branch:
      {
        const auto& altr = this->m_stor.as<index_branch>();
        // Generate code for both branches.
        // Both branches may be TCO'd.
        auto code_true = do_generate_code_branch(options, tco_aware, ctx, altr.branch_true);
        auto code_false = do_generate_code_branch(options, tco_aware, ctx, altr.branch_false);
        // Encode arguments.
        AIR_Node::S_branch_expression xnode = { rocket::move(code_true), rocket::move(code_false), altr.assign };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_function_call:
      {
        const auto& altr = this->m_stor.as<index_function_call>();
        // Encode arguments.
        AIR_Node::S_function_call xnode = { altr.sloc, altr.args_by_refs, options.proper_tail_calls ? tco_aware : tco_aware_none };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_member_access:
      {
        const auto& altr = this->m_stor.as<index_member_access>();
        // Encode arguments.
        AIR_Node::S_member_access xnode = { altr.name };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_operator_rpn:
      {
        const auto& altr = this->m_stor.as<index_operator_rpn>();
        switch(altr.xop) {
        case xop_inc_post:
          {
            AIR_Node::S_apply_xop_inc_post xnode = { };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_dec_post:
          {
            AIR_Node::S_apply_xop_dec_post xnode = { };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_subscr:
          {
            AIR_Node::S_apply_xop_subscr xnode = { };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_pos:
          {
            AIR_Node::S_apply_xop_pos xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_neg:
          {
            AIR_Node::S_apply_xop_neg xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_notb:
          {
            AIR_Node::S_apply_xop_notb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_notl:
          {
            AIR_Node::S_apply_xop_notl xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_inc_pre:
          {
            AIR_Node::S_apply_xop_inc_pre xnode = { };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_dec_pre:
          {
            AIR_Node::S_apply_xop_dec_pre xnode = { };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_unset:
          {
            AIR_Node::S_apply_xop_unset xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_lengthof:
          {
            AIR_Node::S_apply_xop_lengthof xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_typeof:
          {
            AIR_Node::S_apply_xop_typeof xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_sqrt:
          {
            AIR_Node::S_apply_xop_sqrt xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_isnan:
          {
            AIR_Node::S_apply_xop_isnan xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_isinf:
          {
            AIR_Node::S_apply_xop_isinf xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_abs:
          {
            AIR_Node::S_apply_xop_abs xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_signb:
          {
            AIR_Node::S_apply_xop_signb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_round:
          {
            AIR_Node::S_apply_xop_round xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_floor:
          {
            AIR_Node::S_apply_xop_floor xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_ceil:
          {
            AIR_Node::S_apply_xop_ceil xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_trunc:
          {
            AIR_Node::S_apply_xop_trunc xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_iround:
          {
            AIR_Node::S_apply_xop_iround xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_ifloor:
          {
            AIR_Node::S_apply_xop_ifloor xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_iceil:
          {
            AIR_Node::S_apply_xop_iceil xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_itrunc:
          {
            AIR_Node::S_apply_xop_itrunc xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_cmp_eq:
          {
            AIR_Node::S_apply_xop_cmp_xeq xnode = { altr.assign, false };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_cmp_ne:
          {
            AIR_Node::S_apply_xop_cmp_xeq xnode = { altr.assign, true };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_cmp_lt:
          {
            AIR_Node::S_apply_xop_cmp_xrel xnode = { altr.assign, compare_less, false };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_cmp_gt:
          {
            AIR_Node::S_apply_xop_cmp_xrel xnode = { altr.assign, compare_greater, false };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_cmp_lte:
          {
            AIR_Node::S_apply_xop_cmp_xrel xnode = { altr.assign, compare_greater, true };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_cmp_gte:
          {
            AIR_Node::S_apply_xop_cmp_xrel xnode = { altr.assign, compare_less, true };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_cmp_3way:
          {
            AIR_Node::S_apply_xop_cmp_3way xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_add:
          {
            AIR_Node::S_apply_xop_add xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_sub:
          {
            AIR_Node::S_apply_xop_sub xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_mul:
          {
            AIR_Node::S_apply_xop_mul xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_div:
          {
            AIR_Node::S_apply_xop_div xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_mod:
          {
            AIR_Node::S_apply_xop_mod xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_sll:
          {
            AIR_Node::S_apply_xop_sll xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_srl:
          {
            AIR_Node::S_apply_xop_srl xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_sla:
          {
            AIR_Node::S_apply_xop_sla xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_sra:
          {
            AIR_Node::S_apply_xop_sra xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_andb:
          {
            AIR_Node::S_apply_xop_andb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_orb:
          {
            AIR_Node::S_apply_xop_orb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_xorb:
          {
            AIR_Node::S_apply_xop_xorb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        case xop_assign:
          {
            AIR_Node::S_apply_xop_assign xnode = { };
            code.emplace_back(rocket::move(xnode));
            return code;
          }
        default:
          ASTERIA_TERMINATE("An unknown operator type enumeration `", altr.xop, "` has been encountered. This is likely a bug. Please report.");
        }
      }
    case index_unnamed_array:
      {
        const auto& altr = this->m_stor.as<index_unnamed_array>();
        // Encode arguments.
        AIR_Node::S_push_unnamed_array xnode = { altr.nelems };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_unnamed_object:
      {
        const auto& altr = this->m_stor.as<index_unnamed_object>();
        // Encode arguments.
        AIR_Node::S_push_unnamed_object xnode = { altr.keys };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_coalescence:
      {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // Generate code for the branch.
        // This branch may be TCO'd.
        auto code_null = do_generate_code_branch(options, tco_aware, ctx, altr.branch_null);
        // Encode arguments.
        AIR_Node::S_coalescence xnode = { rocket::move(code_null), altr.assign };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    case index_operator_fma:
      {
        const auto& altr = this->m_stor.as<index_operator_fma>();
        // Encode arguments.
        AIR_Node::S_apply_xop_fma xnode = { altr.assign };
        code.emplace_back(rocket::move(xnode));
        return code;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression unit type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
