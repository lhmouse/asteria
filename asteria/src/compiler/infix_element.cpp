// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "infix_element.hpp"
#include "../utilities.hpp"

namespace Asteria {

Infix_Element::Precedence Infix_Element::tell_precedence() const noexcept
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_head:
      {
        // const auto& alt = this->m_stor.as<S_head>();
        return precedence_lowest;
      }
    case index_ternary:
      {
        // const auto& alt = this->m_stor.as<S_ternary>();
        return precedence_assignment;
      }
    case index_logical_and:
      {
        const auto& alt = this->m_stor.as<S_logical_and>();
        if(alt.assign) {
          return precedence_assignment;
        }
        return precedence_logical_and;
      }
    case index_logical_or:
      {
        const auto& alt = this->m_stor.as<S_logical_or>();
        if(alt.assign) {
          return precedence_assignment;
        }
        return precedence_logical_or;
      }
    case index_coalescence:
      {
        const auto& alt = this->m_stor.as<S_coalescence>();
        if(alt.assign) {
          return precedence_assignment;
        }
        return precedence_coalescence;
      }
    case index_general:
      {
        const auto& alt = this->m_stor.as<S_general>();
        if(alt.assign) {
          return precedence_assignment;
        }
        switch(rocket::weaken_enum(alt.xop)) {
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
          ASTERIA_TERMINATE("An invalid infix operator `", alt.xop, "` has been encountered.");
        }
      }
    default:
      ASTERIA_TERMINATE("An unknown infix-element type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

void Infix_Element::extract(Cow_Vector<Xprunit>& units)
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_head:
      {
        auto& alt = this->m_stor.as<S_head>();
        // Move-append all units into `units`.
        std::move(alt.units.mut_begin(), alt.units.mut_end(), std::back_inserter(units));
        return;
      }
    case index_ternary:
      {
        auto& alt = this->m_stor.as<S_ternary>();
        // Construct a branch unit from both branches, then append it to `units`.
        Xprunit::S_branch unit_c = { rocket::move(alt.branch_true), rocket::move(alt.branch_false), alt.assign };
        units.emplace_back(rocket::move(unit_c));
        return;
      }
    case index_logical_and:
      {
        auto& alt = this->m_stor.as<S_logical_and>();
        // Construct a branch unit from the TRUE branch and an empty FALSE branch, then append it to `units`.
        Xprunit::S_branch unit_c = { rocket::move(alt.branch_true), rocket::clear, alt.assign };
        units.emplace_back(rocket::move(unit_c));
        return;
      }
    case index_logical_or:
      {
        auto& alt = this->m_stor.as<S_logical_or>();
        // Construct a branch unit from an empty TRUE branch and the FALSE branch, then append it to `units`.
        Xprunit::S_branch unit_c = { rocket::clear, rocket::move(alt.branch_false), alt.assign };
        units.emplace_back(rocket::move(unit_c));
        return;
      }
    case index_coalescence:
      {
        auto& alt = this->m_stor.as<S_coalescence>();
        // Construct a branch unit from the NULL branch, then append it to `units`.
        Xprunit::S_coalescence unit_c = { rocket::move(alt.branch_null), alt.assign };
        units.emplace_back(rocket::move(unit_c));
        return;
      }
    case index_general:
      {
        auto& alt = this->m_stor.as<S_general>();
        // N.B. `units` is the LHS operand.
        // Append the RHS operand to the LHS operand, followed by the operator, forming the Reverse Polish Notation (RPN).
        std::move(alt.rhs.mut_begin(), alt.rhs.mut_end(), std::back_inserter(units));
        // Append the operator itself.
        Xprunit::S_operator_rpn unit_c = { alt.xop, alt.assign };
        units.emplace_back(rocket::move(unit_c));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown infix-element type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

Cow_Vector<Xprunit>& Infix_Element::open_junction() noexcept
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_head:
      {
        auto& alt = this->m_stor.as<S_head>();
        return alt.units;
      }
    case index_ternary:
      {
        auto& alt = this->m_stor.as<S_ternary>();
        return alt.branch_false;
      }
    case index_logical_and:
      {
        auto& alt = this->m_stor.as<S_logical_and>();
        return alt.branch_true;
      }
    case index_logical_or:
      {
        auto& alt = this->m_stor.as<S_logical_or>();
        return alt.branch_false;
      }
    case index_coalescence:
      {
        auto& alt = this->m_stor.as<S_coalescence>();
        return alt.branch_null;
      }
    case index_general:
      {
        auto& alt = this->m_stor.as<S_general>();
        return alt.rhs;
      }
    default:
      ASTERIA_TERMINATE("An unknown infix-element type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}  // namespace Asteria
