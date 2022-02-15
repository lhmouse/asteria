// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_INFIX_ELEMENT_HPP_
#define ASTERIA_COMPILER_INFIX_ELEMENT_HPP_

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
    ::rocket::variant<
      ROCKET_CDR(
        ,S_head         // 0,
        ,S_ternary      // 1,
        ,S_logical_and  // 2,
        ,S_logical_or   // 3,
        ,S_coalescence  // 4,
        ,S_general      // 5,
      )>
      m_stor;

  public:
    // Constructors and assignment operators
    template<typename XElemT,
    ROCKET_ENABLE_IF(::std::is_constructible<decltype(m_stor), XElemT&&>::value)>
    constexpr
    Infix_Element(XElemT&& xelem)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor), XElemT&&>::value)
      : m_stor(::std::forward<XElemT>(xelem))
      { }

    template<typename XElemT,
    ROCKET_ENABLE_IF(::std::is_assignable<decltype(m_stor)&, XElemT&&>::value)>
    Infix_Element&
    operator=(XElemT&& xelem)
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&, XElemT&&>::value)
      { this->m_stor = ::std::forward<XElemT>(xelem);
        return *this;  }

    Infix_Element&
    swap(Infix_Element& other) noexcept
      { this->m_stor.swap(other.m_stor);
        return *this;  }

  public:
    Index
    index() const noexcept
      { return static_cast<Index>(this->m_stor.index());  }

    // Returns the precedence of this element.
    Precedence
    tell_precedence() const noexcept;

    // Moves all units into `units`.
    Infix_Element&
    extract(cow_vector<Expression_Unit>& units);

    // Returns a reference where new units will be appended.
    cow_vector<Expression_Unit>&
    open_junction() noexcept;
  };

inline void
swap(Infix_Element& lhs, Infix_Element& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
