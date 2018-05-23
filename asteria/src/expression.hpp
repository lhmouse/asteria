// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "expression_node.hpp"

namespace Asteria {

class Expression {
	friend Expression_node;

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
	decltype(m_nodes)::const_iterator begin() const noexcept {
		return m_nodes.begin();
	}
	decltype(m_nodes)::const_iterator end() const noexcept {
		return m_nodes.end();
	}
	decltype(m_nodes)::const_reference at(std::size_t n) const {
		return m_nodes.at(n);
	}
};

extern void bind_expression(Xptr<Expression> &bound_result_out, Spparam<const Expression> expression_opt, Spparam<const Scope> scope);
extern void evaluate_expression(Xptr<Reference> &reference_out, Spparam<Recycler> recycler, Spparam<const Expression> expression_opt, Spparam<const Scope> scope);

}

#endif
