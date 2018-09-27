// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"

namespace Asteria {

class Expression
  {
  private:
    Vector<Xpnode> m_nodes;

  public:
    Expression() noexcept;
    Expression(Vector<Xpnode> &&nodes) noexcept;
    ~Expression();

  public:
    void swap(Expression &other) noexcept
      {
        rocket::adl_swap(this->m_nodes, other.m_nodes);
      }

    bool empty() const noexcept;
    Expression bind(const Analytic_context &ctx) const;
    Reference evaluate(const Executive_context &ctx) const;
  };

inline void swap(Expression &lhs, Expression &rhs) noexcept
  {
    lhs.swap(rhs);
  }

}

#endif
