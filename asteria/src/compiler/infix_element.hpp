// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_INFIX_ELEMENT_HPP_
#define ASTERIA_COMPILER_INFIX_ELEMENT_HPP_

#include "../fwd.hpp"
#include "../syntax/xprunit.hpp"

namespace Asteria {

class Infix_Element
  {
  public:
    enum Precedence : uint8_t
      {
        precedence_multiplicative  =  1,
        precedence_additive        =  2,
        precedence_shift           =  3,
        precedence_relational      =  4,
        precedence_equality        =  5,
        precedence_bitwise_and     =  6,
        precedence_bitwise_xor     =  7,
        precedence_bitwise_or      =  8,
        precedence_logical_and     =  9,
        precedence_logical_or      = 10,
        precedence_coalescence     = 11,
        precedence_assignment      = 12,
        precedence_lowest          = 99,
      };

    struct S_head
      {
        cow_vector<Xprunit> units;
      };
    struct S_ternary  // ? :
      {
        bool assign;
        cow_vector<Xprunit> branch_true;
        cow_vector<Xprunit> branch_false;
      };
    struct S_logical_and  // &&
      {
        bool assign;
        cow_vector<Xprunit> branch_true;
      };
    struct S_logical_or  // ||
      {
        bool assign;
        cow_vector<Xprunit> branch_false;
      };
    struct S_coalescence  // ??
      {
        bool assign;
        cow_vector<Xprunit> branch_null;
      };
    struct S_general  // no short circuit
      {
        Xprunit::Xop xop;
        bool assign;
        cow_vector<Xprunit> rhs;
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
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  private:
    Xvariant m_stor;

  public:
    template<typename XelemT, ASTERIA_SFINAE_CONSTRUCT(Xvariant, XelemT&&)> Infix_Element(XelemT&& elem) noexcept
      : m_stor(rocket::forward<XelemT>(elem))
      {
      }
    template<typename XelemT, ASTERIA_SFINAE_ASSIGN(Xvariant, XelemT&&)> Infix_Element& operator=(XelemT&& elem) noexcept
      {
        this->m_stor = rocket::forward<XelemT>(elem);
        return *this;
      }

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
      }

    void swap(Infix_Element& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
      }

    // Returns the precedence of this element.
    Precedence tell_precedence() const noexcept;
    // Moves all units into `units`.
    void extract(cow_vector<Xprunit>& units);
    // Returns a reference where new units will be appended.
    cow_vector<Xprunit>& open_junction() noexcept;
  };

inline void swap(Infix_Element& lhs, Infix_Element& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
