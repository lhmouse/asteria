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

extern void bind_expression(Xptr<Expression> &bound_result_out, Spparam<const Expression> expression_opt, Spparam<const Scope> scope);
extern void evaluate_expression(Xptr<Reference> &reference_out, Spparam<Recycler> recycler, Spparam<const Expression> expression_opt, Spparam<const Scope> scope);

}

#endif
