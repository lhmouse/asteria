// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "expression_node.hpp"

namespace Asteria {

class Expression {
private:
	T_vector<Expression_node> m_nodes;

public:
	Expression(T_vector<Expression_node> nodes)
		: m_nodes(std::move(nodes))
	{ }
	Expression(Expression &&) noexcept;
	Expression & operator=(Expression &&) noexcept;
	~Expression();

public:
	bool empty() const noexcept {
		return m_nodes.empty();
	}
	std::size_t size() const noexcept {
		return m_nodes.size();
	}
	const Expression_node & at(std::size_t n) const {
		return m_nodes.at(n);
	}
	const Expression_node * begin() const noexcept {
		return m_nodes.data();
	}
	const Expression_node * end() const noexcept {
		return m_nodes.data() + m_nodes.size();
	}
};

extern void bind_expression(Vp<Expression> &bound_expr_out, Spr<const Expression> expression_opt, Spr<const Scope> scope);
extern void evaluate_expression(Vp<Reference> &reference_out, Spr<Recycler> recycler_inout, Spr<const Expression> expression_opt, Spr<const Scope> scope);

}

#endif
