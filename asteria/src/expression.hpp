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
    Expression() noexcept
      : m_nodes()
      {
      }
    Expression(Vector<Xpnode> &&nodes) noexcept
      : m_nodes(std::move(nodes))
      {
      }
    ~Expression();

  public:
    bool empty() const noexcept;
    Expression bind(const Analytic_context &ctx) const;
    bool evaluate_partial(Vector<Reference> &stack_io, const Executive_context &ctx) const;
    Reference evaluate(const Executive_context &ctx) const;
  };

}

#endif
