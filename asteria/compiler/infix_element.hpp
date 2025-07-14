// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_INFIX_ELEMENT_
#define ASTERIA_COMPILER_INFIX_ELEMENT_

#include "../fwd.hpp"
#include "../source_location.hpp"
namespace asteria {

class Infix_Element
  {
  public:
    struct S_head
      {
        cow_vector<Expression_Unit> units;
      };

    struct S_ternary  // ? :
      {
        Source_Location sloc;
        bool assign;
        cow_vector<Expression_Unit> branch_true;
        cow_vector<Expression_Unit> branch_false;
      };

    struct S_logical_and  // &&
      {
        Source_Location sloc;
        bool assign;
        cow_vector<Expression_Unit> branch_true;
      };

    struct S_logical_or  // ||
      {
        Source_Location sloc;
        bool assign;
        cow_vector<Expression_Unit> branch_false;
      };

    struct S_coalescence  // ??
      {
        Source_Location sloc;
        bool assign;
        cow_vector<Expression_Unit> branch_null;
      };

    struct S_general  // no short circuit
      {
        Source_Location sloc;
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

  private:
    ASTERIA_VARIANT(
      m_stor
        , S_head         // 0
        , S_ternary      // 1
        , S_logical_and  // 2
        , S_logical_or   // 3
        , S_coalescence  // 4
        , S_general      // 5
      );

  public:
    // Constructors and assignment operators
    template<typename xElement,
    ROCKET_ENABLE_IF(::std::is_constructible<decltype(m_stor), xElement&&>::value)>
    constexpr Infix_Element(xElement&& xelem)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor), xElement&&>::value)
      :
        m_stor(forward<xElement>(xelem))
      { }

    template<typename xElement,
    ROCKET_ENABLE_IF(::std::is_assignable<decltype(m_stor)&, xElement&&>::value)>
    Infix_Element&
    operator=(xElement&& xelem)
      & noexcept(::std::is_nothrow_assignable<decltype(m_stor)&, xElement&&>::value)
      {
        this->m_stor = forward<xElement>(xelem);
        return *this;
      }

    Infix_Element&
    swap(Infix_Element& other)
      noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    // Returns the precedence of this element.
    Precedence
    tell_precedence()
      const noexcept;

    // Moves all units into `units`.
    void
    extract(cow_vector<Expression_Unit>& units);

    // Returns a reference where new units will be appended.
    cow_vector<Expression_Unit>&
    mut_junction()
      noexcept;
  };

inline
void
swap(Infix_Element& lhs, Infix_Element& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
