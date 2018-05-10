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
	std::vector<Expression_node> m_nodes;

public:
	Expression(std::vector<Expression_node> nodes)
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

extern void bind_expression(Xptr<Expression> &bound_result_out, Spcref<const Expression> expression_opt, Spcref<const Scope> scope);
extern void evaluate_expression(Xptr<Reference> &reference_out, Spcref<Recycler> recycler, Spcref<const Expression> expression_opt, Spcref<const Scope> scope);

}

#endif
