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
    Expression(Vector<Xpnode> &&nodes) noexcept;
    ~Expression();

    Expression(Expression &&) noexcept;
    Expression & operator=(Expression &&) noexcept;

  public:
    bool empty() const noexcept;
    Size size() const noexcept;

    Expression bind(const Analytic_context &ctx) const;
    Reference evaluate(const Executive_context &ctx) const;
  };

}

#endif
