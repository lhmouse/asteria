// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "xprunit.hpp"
#include "statement.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/analytic_context.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char* Xprunit::describe_operator(Xprunit::Xop xop) noexcept
  {
    switch(xop) {
    case xop_postfix_inc:
      {
        return "postfix increment";
      }
    case xop_postfix_dec:
      {
        return "postfix decrement";
      }
    case xop_postfix_subscr:
      {
        return "postfix subscript";
      }
    case xop_prefix_pos:
      {
        return "unary promotion";
      }
    case xop_prefix_neg:
      {
        return "unary negation";
      }
    case xop_prefix_notb:
      {
        return "bitwise not";
      }
    case xop_prefix_notl:
      {
        return "logical not";
      }
    case xop_prefix_inc:
      {
        return "prefix increment";
      }
    case xop_prefix_dec:
      {
        return "prefix decrement";
      }
    case xop_prefix_unset:
      {
        return "prefix `unset`";
      }
    case xop_prefix_lengthof:
      {
        return "prefix `lengthof`";
      }
    case xop_prefix_typeof:
      {
        return "prefix `typeof`";
      }
    case xop_prefix_sqrt:
      {
        return "prefix `__sqrt`";
      }
    case xop_prefix_isnan:
      {
        return "prefix `__isnan`";
      }
    case xop_prefix_isinf:
      {
        return "prefix `__isinf`";
      }
    case xop_prefix_abs:
      {
        return "prefix `__abs`";
      }
    case xop_prefix_signb:
      {
        return "prefix `__signb`";
      }
    case xop_prefix_round:
      {
        return "prefix `__round`";
      }
    case xop_prefix_floor:
      {
        return "prefix `__floor`";
      }
    case xop_prefix_ceil:
      {
        return "prefix `__ceil`";
      }
    case xop_prefix_trunc:
      {
        return "prefix `__trunc`";
      }
    case xop_prefix_iround:
      {
        return "prefix `__iround`";
      }
    case xop_prefix_ifloor:
      {
        return "prefix `__ifloor`";
      }
    case xop_prefix_iceil:
      {
        return "prefix `__iceil`";
      }
    case xop_prefix_itrunc:
      {
        return "prefix `__itrunc`";
      }
    case xop_infix_cmp_eq:
      {
        return "equality comparison";
      }
    case xop_infix_cmp_ne:
      {
        return "inequality comparison";
      }
    case xop_infix_cmp_lt:
      {
        return "less-than comparison";
      }
    case xop_infix_cmp_gt:
      {
        return "greater-than comparison";
      }
    case xop_infix_cmp_lte:
      {
        return "less-than-or-equal comparison";
      }
    case xop_infix_cmp_gte:
      {
        return "greater-than-or-equal comparison";
      }
    case xop_infix_cmp_3way:
      {
        return "three-way comparison";
      }
    case xop_infix_add:
      {
        return "addition";
      }
    case xop_infix_sub:
      {
        return "subtraction";
      }
    case xop_infix_mul:
      {
        return "multiplication";
      }
    case xop_infix_div:
      {
        return "division";
      }
    case xop_infix_mod:
      {
        return "modulo";
      }
    case xop_infix_sll:
      {
        return "logical left shift";
      }
    case xop_infix_srl:
      {
        return "arithmetic left shift";
      }
    case xop_infix_sla:
      {
        return "logical right shift";
      }
    case xop_infix_sra:
      {
        return "arithmetic right shift";
      }
    case xop_infix_andb:
      {
        return "bitwise and";
      }
    case xop_infix_orb:
      {
        return "bitwise or";
      }
    case xop_infix_xorb:
      {
        return "bitwise xor";
      }
    case xop_infix_assign:
      {
        return "assginment";
      }
    default:
      return "<unknown operator>";
    }
  }

    namespace {

    cow_vector<AIR_Node> do_generate_code_branch(const Compiler_Options& options, Xprunit::TCO_Awareness tco_awareness, const Analytic_Context& ctx,
                                                 const cow_vector<Xprunit>& units)
      {
        cow_vector<AIR_Node> code;
        size_t epos = units.size() - 1;
        if(epos != SIZE_MAX) {
          // Expression units other than the last one cannot be TCO'd.
          for(size_t i = 0; i != epos; ++i) {
            units[i].generate_code(code, options, Xprunit::tco_none, ctx);
          }
          // The last unit may be TCO'd.
          units[epos].generate_code(code, options, tco_awareness, ctx);
        }
        return code;
      }

    }

void Xprunit::generate_code(cow_vector<AIR_Node>& code, const Compiler_Options& options, Xprunit::TCO_Awareness tco_awareness, const Analytic_Context& ctx) const
  {
    switch(this->index()) {
    case index_literal:
      {
        const auto& altr = this->m_stor.as<index_literal>();
        // Encode arguments.
        AIR_Node::S_push_literal xnode = { altr.value };
        code.emplace_back(rocket::move(xnode));
        return;
      }
    case index_named_reference:
      {
        const auto& altr = this->m_stor.as<index_named_reference>();
        // Perform early lookup when the expression is defined.
        // If a named reference is found, it will not be replaced or hidden by a later-declared one.
        const Abstract_Context* qctx = std::addressof(ctx);
        size_t depth = 0;
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
        return;
      }
    case index_closure_function:
      {
        const auto& altr = this->m_stor.as<index_closure_function>();
        // Encode arguments.
        AIR_Node::S_define_function xnode = { options, altr.sloc, rocket::sref("<closure>"), altr.params, altr.body };
        code.emplace_back(rocket::move(xnode));
        return;
      }
    case index_branch:
      {
        const auto& altr = this->m_stor.as<index_branch>();
        // Generate code for both branches.
        // Both branches may be TCO'd.
        auto code_true = do_generate_code_branch(options, tco_awareness, ctx, altr.branch_true);
        auto code_false = do_generate_code_branch(options, tco_awareness, ctx, altr.branch_false);
        // Encode arguments.
        AIR_Node::S_branch_expression xnode = { rocket::move(code_true), rocket::move(code_false), altr.assign };
        code.emplace_back(rocket::move(xnode));
        return;
      }
    case index_function_call:
      {
        const auto& altr = this->m_stor.as<index_function_call>();
        // Encode arguments.
        if(options.proper_tail_calls && (tco_awareness != tco_none)) {
          uint32_t flags = 0;
          // Calculate TCO flags.
          switch(rocket::weaken_enum(tco_awareness)) {
          case tco_by_value:
            flags |= Reference_Root::tcof_by_value;
            break;
          case tco_nullify:
            flags |= Reference_Root::tcof_nullify;
            break;
          }
          // Generate a tail call.
          AIR_Node::S_function_call_tail xnode = { altr.sloc, altr.args_by_refs, flags };
          code.emplace_back(rocket::move(xnode));
        }
        else {
          // Generate a plain call.
          AIR_Node::S_function_call_plain xnode = { altr.sloc, altr.args_by_refs };
          code.emplace_back(rocket::move(xnode));
        }
        return;
      }
    case index_member_access:
      {
        const auto& altr = this->m_stor.as<index_member_access>();
        // Encode arguments.
        AIR_Node::S_member_access xnode = { altr.name };
        code.emplace_back(rocket::move(xnode));
        return;
      }
    case index_operator_rpn:
      {
        const auto& altr = this->m_stor.as<index_operator_rpn>();
        switch(altr.xop) {
        case xop_postfix_inc:
          {
            AIR_Node::S_apply_xop_inc_post xnode = { };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_postfix_dec:
          {
            AIR_Node::S_apply_xop_dec_post xnode = { };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_postfix_subscr:
          {
            AIR_Node::S_apply_xop_subscr xnode = { };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_pos:
          {
            AIR_Node::S_apply_xop_pos xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_neg:
          {
            AIR_Node::S_apply_xop_neg xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_notb:
          {
            AIR_Node::S_apply_xop_notb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_notl:
          {
            AIR_Node::S_apply_xop_notl xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_inc:
          {
            AIR_Node::S_apply_xop_inc xnode = { };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_dec:
          {
            AIR_Node::S_apply_xop_dec xnode = { };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_unset:
          {
            AIR_Node::S_apply_xop_unset xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_lengthof:
          {
            AIR_Node::S_apply_xop_lengthof xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_typeof:
          {
            AIR_Node::S_apply_xop_typeof xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_sqrt:
          {
            AIR_Node::S_apply_xop_sqrt xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_isnan:
          {
            AIR_Node::S_apply_xop_isnan xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_isinf:
          {
            AIR_Node::S_apply_xop_isinf xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_abs:
          {
            AIR_Node::S_apply_xop_abs xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_signb:
          {
            AIR_Node::S_apply_xop_signb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_round:
          {
            AIR_Node::S_apply_xop_round xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_floor:
          {
            AIR_Node::S_apply_xop_floor xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_ceil:
          {
            AIR_Node::S_apply_xop_ceil xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_trunc:
          {
            AIR_Node::S_apply_xop_trunc xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_iround:
          {
            AIR_Node::S_apply_xop_iround xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_ifloor:
          {
            AIR_Node::S_apply_xop_ifloor xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_iceil:
          {
            AIR_Node::S_apply_xop_iceil xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_prefix_itrunc:
          {
            AIR_Node::S_apply_xop_itrunc xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_cmp_eq:
          {
            AIR_Node::S_apply_xop_cmp_xeq xnode = { false, altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_cmp_ne:
          {
            AIR_Node::S_apply_xop_cmp_xeq xnode = { true, altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_cmp_lt:
          {
            AIR_Node::S_apply_xop_cmp_xrel xnode = { Value::compare_less, false, altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_cmp_gt:
          {
            AIR_Node::S_apply_xop_cmp_xrel xnode = { Value::compare_greater, false, altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_cmp_lte:
          {
            AIR_Node::S_apply_xop_cmp_xrel xnode = { Value::compare_greater, true, altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_cmp_gte:
          {
            AIR_Node::S_apply_xop_cmp_xrel xnode = { Value::compare_less, true, altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_cmp_3way:
          {
            AIR_Node::S_apply_xop_cmp_3way xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_add:
          {
            AIR_Node::S_apply_xop_add xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_sub:
          {
            AIR_Node::S_apply_xop_sub xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_mul:
          {
            AIR_Node::S_apply_xop_mul xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_div:
          {
            AIR_Node::S_apply_xop_div xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_mod:
          {
            AIR_Node::S_apply_xop_mod xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_sll:
          {
            AIR_Node::S_apply_xop_sll xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_srl:
          {
            AIR_Node::S_apply_xop_srl xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_sla:
          {
            AIR_Node::S_apply_xop_sla xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_sra:
          {
            AIR_Node::S_apply_xop_sra xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_andb:
          {
            AIR_Node::S_apply_xop_andb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_orb:
          {
            AIR_Node::S_apply_xop_orb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_xorb:
          {
            AIR_Node::S_apply_xop_xorb xnode = { altr.assign };
            code.emplace_back(rocket::move(xnode));
            return;
          }
        case xop_infix_assign:
          {
            AIR_Node::S_apply_xop_assign xnode = { };
            code.emplace_back(rocket::move(xnode));
            return;
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
        return;
      }
    case index_unnamed_object:
      {
        const auto& altr = this->m_stor.as<index_unnamed_object>();
        // Encode arguments.
        AIR_Node::S_push_unnamed_object xnode = { altr.keys };
        code.emplace_back(rocket::move(xnode));
        return;
      }
    case index_coalescence:
      {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // Generate code for the branch.
        // This branch may be TCO'd.
        auto code_null = do_generate_code_branch(options, tco_awareness, ctx, altr.branch_null);
        // Encode arguments.
        AIR_Node::S_coalescence xnode = { rocket::move(code_null), altr.assign };
        code.emplace_back(rocket::move(xnode));
        return;
      }
    case index_operator_fma:
      {
        const auto& altr = this->m_stor.as<index_operator_fma>();
        // Encode arguments.
        AIR_Node::S_apply_xop_fma xnode = { altr.assign };
        code.emplace_back(rocket::move(xnode));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression unit type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
