// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "infix_element.hpp"
#include "../utilities.hpp"

namespace Asteria {

Infix_Element::Precedence Infix_Element::tell_precedence() const noexcept
  {
    switch(this->index()) {
    case index_head:
      {
        // const auto& altr = this->m_stor.as<index_head>();
        return precedence_lowest;
      }
    case index_ternary:
      {
        // const auto& altr = this->m_stor.as<index_ternary>();
        return precedence_assignment;
      }
    case index_logical_and:
      {
        const auto& altr = this->m_stor.as<index_logical_and>();
        if(altr.assign) {
          return precedence_assignment;
        }
        return precedence_logical_and;
      }
    case index_logical_or:
      {
        const auto& altr = this->m_stor.as<index_logical_or>();
        if(altr.assign) {
          return precedence_assignment;
        }
        return precedence_logical_or;
      }
    case index_coalescence:
      {
        const auto& altr = this->m_stor.as<index_coalescence>();
        if(altr.assign) {
          return precedence_assignment;
        }
        return precedence_coalescence;
      }
    case index_general:
      {
        const auto& altr = this->m_stor.as<index_general>();
        if(altr.assign) {
          return precedence_assignment;
        }
        switch(rocket::weaken_enum(altr.xop)) {
        case Xprunit::xop_infix_mul:
        case Xprunit::xop_infix_div:
        case Xprunit::xop_infix_mod:
          {
            return precedence_multiplicative;
          }
        case Xprunit::xop_infix_add:
        case Xprunit::xop_infix_sub:
          {
            return precedence_additive;
          }
        case Xprunit::xop_infix_sla:
        case Xprunit::xop_infix_sra:
        case Xprunit::xop_infix_sll:
        case Xprunit::xop_infix_srl:
          {
            return precedence_shift;
          }
        case Xprunit::xop_infix_cmp_lt:
        case Xprunit::xop_infix_cmp_lte:
        case Xprunit::xop_infix_cmp_gt:
        case Xprunit::xop_infix_cmp_gte:
          {
            return precedence_relational;
          }
        case Xprunit::xop_infix_cmp_eq:
        case Xprunit::xop_infix_cmp_ne:
        case Xprunit::xop_infix_cmp_3way:
          {
            return precedence_equality;
          }
        case Xprunit::xop_infix_andb:
          {
            return precedence_bitwise_and;
          }
        case Xprunit::xop_infix_xorb:
          {
            return precedence_bitwise_xor;
          }
        case Xprunit::xop_infix_orb:
          {
            return precedence_bitwise_or;
          }
        case Xprunit::xop_infix_assign:
          {
            return precedence_assignment;
          }
        default:
          ASTERIA_TERMINATE("An invalid infix operator `", altr.xop, "` has been encountered. This is likely a bug. Please report.");
        }
      }
    default:
      ASTERIA_TERMINATE("An unknown infix-element type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

void Infix_Element::extract(cow_vector<Xprunit>& units)
  {
    switch(this->index()) {
    case index_head:
      {
        auto& altr = this->m_stor.as<index_head>();
        // Move-append all units into `units`.
        std::move(altr.units.mut_begin(), altr.units.mut_end(), std::back_inserter(units));
        return;
      }
    case index_ternary:
      {
        auto& altr = this->m_stor.as<index_ternary>();
        // Construct a branch unit from both branches, then append it to `units`.
        Xprunit::S_branch xunit = { rocket::move(altr.branch_true), rocket::move(altr.branch_false), altr.assign };
        units.emplace_back(rocket::move(xunit));
        return;
      }
    case index_logical_and:
      {
        auto& altr = this->m_stor.as<index_logical_and>();
        // Construct a branch unit from the TRUE branch and an empty FALSE branch, then append it to `units`.
        Xprunit::S_branch xunit = { rocket::move(altr.branch_true), rocket::clear, altr.assign };
        units.emplace_back(rocket::move(xunit));
        return;
      }
    case index_logical_or:
      {
        auto& altr = this->m_stor.as<index_logical_or>();
        // Construct a branch unit from an empty TRUE branch and the FALSE branch, then append it to `units`.
        Xprunit::S_branch xunit = { rocket::clear, rocket::move(altr.branch_false), altr.assign };
        units.emplace_back(rocket::move(xunit));
        return;
      }
    case index_coalescence:
      {
        auto& altr = this->m_stor.as<index_coalescence>();
        // Construct a branch unit from the NULL branch, then append it to `units`.
        Xprunit::S_coalescence xunit = { rocket::move(altr.branch_null), altr.assign };
        units.emplace_back(rocket::move(xunit));
        return;
      }
    case index_general:
      {
        auto& altr = this->m_stor.as<index_general>();
        // N.B. `units` is the LHS operand.
        // Append the RHS operand to the LHS operand, followed by the operator, forming the Reverse Polish Notation (RPN).
        std::move(altr.rhs.mut_begin(), altr.rhs.mut_end(), std::back_inserter(units));
        // Append the operator itself.
        Xprunit::S_operator_rpn xunit = { altr.xop, altr.assign };
        units.emplace_back(rocket::move(xunit));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown infix-element type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

cow_vector<Xprunit>& Infix_Element::open_junction() noexcept
  {
    switch(this->index()) {
    case index_head:
      {
        auto& altr = this->m_stor.as<index_head>();
        return altr.units;
      }
    case index_ternary:
      {
        auto& altr = this->m_stor.as<index_ternary>();
        return altr.branch_false;
      }
    case index_logical_and:
      {
        auto& altr = this->m_stor.as<index_logical_and>();
        return altr.branch_true;
      }
    case index_logical_or:
      {
        auto& altr = this->m_stor.as<index_logical_or>();
        return altr.branch_false;
      }
    case index_coalescence:
      {
        auto& altr = this->m_stor.as<index_coalescence>();
        return altr.branch_null;
      }
    case index_general:
      {
        auto& altr = this->m_stor.as<index_general>();
        return altr.rhs;
      }
    default:
      ASTERIA_TERMINATE("An unknown infix-element type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
