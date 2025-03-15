// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "infix_element.hpp"
#include "statement.hpp"
#include "expression_unit.hpp"
#include "enums.hpp"
#include "../runtime/enums.hpp"
#include "../utils.hpp"
namespace asteria {

Precedence
Infix_Element::
tell_precedence() const noexcept
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_head:
        return precedence_lowest;

      case index_ternary:
        return precedence_assignment;

      case index_logical_and:
        {
          const auto& altr = this->m_stor.as<S_logical_and>();
          if(altr.assign)
            return precedence_assignment;
          else
            return precedence_logical_and;
        }

      case index_logical_or:
        {
          const auto& altr = this->m_stor.as<S_logical_or>();
          if(altr.assign)
            return precedence_assignment;
          else
            return precedence_logical_or;
        }

      case index_coalescence:
        {
          const auto& altr = this->m_stor.as<S_coalescence>();
          if(altr.assign)
            return precedence_assignment;
          else
            return precedence_coalescence;
        }

      case index_general:
        {
          const auto& altr = this->m_stor.as<S_general>();
          if(altr.assign)
            return precedence_assignment;
          else
            switch(static_cast<uint32_t>(altr.xop))
              {
              case xop_mul:
              case xop_div:
              case xop_mod:
                return precedence_multiplicative;

              case xop_add:
              case xop_sub:
                return precedence_additive;

              case xop_sla:
              case xop_sra:
              case xop_sll:
              case xop_srl:
                return precedence_shift;

              case xop_cmp_lt:
              case xop_cmp_lte:
              case xop_cmp_gt:
              case xop_cmp_gte:
                return precedence_relational;

              case xop_cmp_eq:
              case xop_cmp_ne:
              case xop_cmp_3way:
              case xop_cmp_un:
                return precedence_equality;

              case xop_andb:
                return precedence_bitwise_and;

              case xop_xorb:
              case xop_orb:
                return precedence_bitwise_or;

              case xop_assign:
                return precedence_assignment;

              default:
                ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), altr.xop);
            }
        }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

void
Infix_Element::
extract(cow_vector<Expression_Unit>& units)
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_head:
        {
          auto& altr = this->m_stor.mut<index_head>();

          // Move-append all units into `units`.
          units.append(altr.units.move_begin(), altr.units.move_end());
          return;
        }

      case index_ternary:
        {
          auto& altr = this->m_stor.mut<index_ternary>();

          // Construct two branch units from the TRUE and FALSE branches.
          cow_vector<Expression_Unit::branch> branches;
          branches.append(2);
          branches.mut(0).type = Expression_Unit::branch_type::branch_type_true;
          branches.mut(0).units = move(altr.branch_true);
          branches.mut(1).type = Expression_Unit::branch_type::branch_type_false;
          branches.mut(1).units = move(altr.branch_false);

          // Append them to `units`.
          Expression_Unit::S_branch xunit = { altr.sloc, move(branches), altr.assign };
          units.emplace_back(move(xunit));
          return;
        }

      case index_logical_and:
        {
          auto& altr = this->m_stor.mut<index_logical_and>();

          // Construct a branch unit from the TRUE branch.
          cow_vector<Expression_Unit::branch> branches;
          branches.append(1);
          branches.mut(0).type = Expression_Unit::branch_type::branch_type_true;
          branches.mut(0).units = move(altr.branch_true);

          // Append it to `units`.
          Expression_Unit::S_branch xunit = { altr.sloc, move(branches), altr.assign };
          units.emplace_back(move(xunit));
          return;
        }

      case index_logical_or:
        {
          auto& altr = this->m_stor.mut<index_logical_or>();

          // Construct a branch unit from the FALSE branch.
          cow_vector<Expression_Unit::branch> branches;
          branches.append(1);
          branches.mut(0).type = Expression_Unit::branch_type::branch_type_false;
          branches.mut(0).units = move(altr.branch_false);

          // Append it to `units`.
          Expression_Unit::S_branch xunit = { altr.sloc, move(branches), altr.assign };
          units.emplace_back(move(xunit));
          return;
        }

      case index_coalescence:
        {
          auto& altr = this->m_stor.mut<index_coalescence>();

          // Construct a branch unit from the NULL branch.
          cow_vector<Expression_Unit::branch> branches;
          branches.append(1);
          branches.mut(0).type = Expression_Unit::branch_type::branch_type_null;
          branches.mut(0).units = move(altr.branch_null);

          // Append it to `units`.
          Expression_Unit::S_branch xunit = { altr.sloc, move(branches), altr.assign };
          units.emplace_back(move(xunit));
          return;
        }

      case index_general:
        {
          auto& altr = this->m_stor.mut<index_general>();

          // N.B. `units` is the LHS operand.
          // Append the RHS operand to the LHS operand, followed by the operator, forming the
          // Reverse Polish Notation (RPN).
          units.append(altr.rhs.move_begin(), altr.rhs.move_end());

          // Append the operator itself.
          Expression_Unit::S_operator_rpn xunit = { altr.sloc, altr.xop, altr.assign };
          units.emplace_back(move(xunit));
          return;
        }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

cow_vector<Expression_Unit>&
Infix_Element::
mut_junction() noexcept
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_head:
        return this->m_stor.mut<index_head>().units;

      case index_ternary:
        return this->m_stor.mut<index_ternary>().branch_false;

      case index_logical_and:
        return this->m_stor.mut<index_logical_and>().branch_true;

      case index_logical_or:
        return this->m_stor.mut<index_logical_or>().branch_false;

      case index_coalescence:
        return this->m_stor.mut<index_coalescence>().branch_null;

      case index_general:
        return this->m_stor.mut<index_general>().rhs;

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

}  // namespace asteria
