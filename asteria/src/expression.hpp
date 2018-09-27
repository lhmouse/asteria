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
    template<typename ...XnodesT, typename std::enable_if<std::is_constructible<Vector<Xpnode>, XnodesT &&...>::value>::type * = nullptr>
      Expression(XnodesT &&...xnodes)
        : m_nodes(std::forward<XnodesT>(xnodes)...)
        {
        }
    ~Expression();

  public:
    bool empty() const noexcept;

    Expression bind(const Analytic_context &ctx) const;
    Reference evaluate(Vector<Reference> &stack, const Executive_context &ctx) const;
  };

}

#endif
