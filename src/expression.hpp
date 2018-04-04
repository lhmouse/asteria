// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "expression_node.hpp"

namespace Asteria {

class Expression {
private:
	boost::container::vector<Expression_node> m_nodes;

public:
	Expression()
		: m_nodes()
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
	decltype(m_nodes)::iterator begin() noexcept {
		return m_nodes.begin();
	}
	decltype(m_nodes)::const_iterator end() const noexcept {
		return m_nodes.end();
	}
	decltype(m_nodes)::iterator end() noexcept {
		return m_nodes.end();
	}
	const Expression_node &at(std::size_t n) const {
		return m_nodes.at(n);
	}
	Expression_node &at(std::size_t n){
		return m_nodes.at(n);
	}
	template<typename ValueT>
	void append(ValueT &&value){
		m_nodes.emplace_back(std::forward<ValueT>(value));
	}
	void clear() noexcept {
		return m_nodes.clear();
	}
};

extern Xptr<Reference> evaluate_expression_recursive_opt(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt);

}

#endif
