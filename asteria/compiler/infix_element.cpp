// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
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
    switch(this->index()) {
      case index_head:
        return precedence_lowest;

      case index_ternary:
        return precedence_assignment;

      case index_logical_and:
        return this->m_stor.as<index_logical_and>().assign
                 ? precedence_assignment
                 : precedence_logical_and;

      case index_logical_or:
        return this->m_stor.as<index_logical_or>().assign
                 ? precedence_assignment
                 : precedence_logical_or;

      case index_coalescence:
        return this->m_stor.as<index_coalescence>().assign
                 ? precedence_assignment
                 : precedence_coalescence;

      case index_general: {
        const auto& altr = this->m_stor.as<index_general>();
        if(altr.assign)
          return precedence_assignment;

        switch(weaken_enum(altr.xop)) {
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
            ASTERIA_TERMINATE((
                "Invalid operator type (xop `$1`)"),
                altr.xop);
        }
      }

      default:
        ASTERIA_TERMINATE((
            "Invalid infix element type (index `$1`)"),
            this->index());
    }
  }

Infix_Element&
Infix_Element::
extract(cow_vector<Expression_Unit>& units)
  {
    switch(this->index()) {
      case index_head: {
        auto& altr = this->m_stor.as<index_head>();

        // Move-append all units into `units`.
        units.append(altr.units.move_begin(), altr.units.move_end());
        return *this;
      }

      case index_ternary: {
        auto& altr = this->m_stor.as<index_ternary>();

        // Construct a branch unit from both branches, then append it to `units`.
        Expression_Unit::S_branch xunit = { altr.sloc, ::std::move(altr.branch_true),
                                            ::std::move(altr.branch_false), altr.assign };
        units.emplace_back(::std::move(xunit));
        return *this;
      }

      case index_logical_and: {
        auto& altr = this->m_stor.as<index_logical_and>();

        // Construct a branch unit from the TRUE branch and an empty FALSE branch, then
        // append it to `units`.
        Expression_Unit::S_branch xunit = { altr.sloc, ::std::move(altr.branch_true), { },
                                            altr.assign };
        units.emplace_back(::std::move(xunit));
        return *this;
      }

      case index_logical_or: {
        auto& altr = this->m_stor.as<index_logical_or>();

        // Construct a branch unit from an empty TRUE branch and the FALSE branch, then
        // append it to `units`.
        Expression_Unit::S_branch xunit = { altr.sloc, { }, ::std::move(altr.branch_false),
                                            altr.assign };
        units.emplace_back(::std::move(xunit));
        return *this;
      }

      case index_coalescence: {
        auto& altr = this->m_stor.as<index_coalescence>();

        // Construct a branch unit from the NULL branch, then append it to `units`.
        Expression_Unit::S_coalescence xunit = { altr.sloc, ::std::move(altr.branch_null),
                                                 altr.assign };
        units.emplace_back(::std::move(xunit));
        return *this;
      }

      case index_general: {
        auto& altr = this->m_stor.as<index_general>();

        // N.B. `units` is the LHS operand.
        // Append the RHS operand to the LHS operand, followed by the operator, forming the
        // Reverse Polish Notation (RPN).
        units.append(altr.rhs.move_begin(), altr.rhs.move_end());

        // Append the operator itself.
        Expression_Unit::S_operator_rpn xunit = { altr.sloc, altr.xop, altr.assign };
        units.emplace_back(::std::move(xunit));
        return *this;
      }

      default:
        ASTERIA_TERMINATE((
            "Invalid infix element type (index `$1`)"),
            this->index());
    }
  }

cow_vector<Expression_Unit>&
Infix_Element::
mut_junction() noexcept
  {
    switch(this->index()) {
      case index_head:
        return this->m_stor.as<index_head>().units;

      case index_ternary:
        return this->m_stor.as<index_ternary>().branch_false;

      case index_logical_and:
        return this->m_stor.as<index_logical_and>().branch_true;

      case index_logical_or:
        return this->m_stor.as<index_logical_or>().branch_false;

      case index_coalescence:
        return this->m_stor.as<index_coalescence>().branch_null;

      case index_general:
        return this->m_stor.as<index_general>().rhs;

      default:
        ASTERIA_TERMINATE((
            "Invalid infix element type (index `$1`)"),
            this->index());
    }
  }

}  // namespace asteria
