// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "expression_node.hpp"

namespace Asteria {

class Expression {
	friend Expression_node;

private:
	boost::container::vector<Expression_node> m_nodes;

public:
	Expression(boost::container::vector<Expression_node> nodes)
		: m_nodes(std::move(nodes))
	{ }
	Expression(Expression &&);
	Expression &operator=(Expression &&);
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
	const Expression_node &at(std::size_t n) const {
		return m_nodes.at(n);
	}
};

extern Xptr<Reference> evaluate_expression_opt(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt);

}

#endif
