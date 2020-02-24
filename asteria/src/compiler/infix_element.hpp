// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_INFIX_ELEMENT_HPP_
#define ASTERIA_COMPILER_INFIX_ELEMENT_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Infix_Element
  {
  public:
    struct S_head
      {
        cow_vector<Expression_Unit> units;
      };
    struct S_ternary  // ? :
      {
        bool assign;
        cow_vector<Expression_Unit> branch_true;
        cow_vector<Expression_Unit> branch_false;
      };
    struct S_logical_and  // &&
      {
        bool assign;
        cow_vector<Expression_Unit> branch_true;
      };
    struct S_logical_or  // ||
      {
        bool assign;
        cow_vector<Expression_Unit> branch_false;
      };
    struct S_coalescence  // ??
      {
        bool assign;
        cow_vector<Expression_Unit> branch_null;
      };
    struct S_general  // no short circuit
      {
        Xop xop;
        bool assign;
        cow_vector<Expression_Unit> rhs;
      };

    enum Index : uint8_t
      {
        index_head         = 0,
        index_ternary      = 1,
        index_logical_and  = 2,
        index_logical_or   = 3,
        index_coalescence  = 4,
        index_general      = 5,
      };
    using Xvariant = variant<
      ROCKET_CDR(
      , S_head         // 0,
      , S_ternary      // 1,
      , S_logical_and  // 2,
      , S_logical_or   // 3,
      , S_coalescence  // 4,
      , S_general      // 5,
      )>;
    static_assert(::std::is_nothrow_copy_assignable<Xvariant>::value, "");

  private:
    Xvariant m_stor;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(Infix_Element, Xvariant, XElemT, xelem)
      :
        m_stor(::rocket::forward<XElemT>(xelem))
      {
      }
    ASTERIA_VARIANT_ASSIGNMENT(Infix_Element, Xvariant, XElemT, xelem)
      {
        this->m_stor = ::rocket::forward<XElemT>(xelem);
        return *this;
      }

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
      }

    Infix_Element& swap(Infix_Element& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    // Returns the precedence of this element.
    Precedence tell_precedence() const noexcept;
    // Moves all units into `units`.
    Infix_Element& extract(cow_vector<Expression_Unit>& units);
    // Returns a reference where new units will be appended.
    cow_vector<Expression_Unit>& open_junction() noexcept;
  };

inline void swap(Infix_Element& lhs, Infix_Element& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
