// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "expression_node.hpp"

namespace Asteria {

class Expression {
private:
	boost::container::vector<Expression_node> m_node_list;

public:
	Expression()
		: m_node_list()
	{ }
	Expression(Expression &&);
	Expression &operator=(Expression &&);
	~Expression();

public:
	bool empty() const noexcept {
		return m_node_list.empty();
	}
	std::size_t size() const noexcept {
		return m_node_list.size();
	}
	decltype(m_node_list)::const_iterator begin() const noexcept {
		return m_node_list.begin();
	}
	decltype(m_node_list)::iterator begin() noexcept {
		return m_node_list.begin();
	}
	decltype(m_node_list)::const_iterator end() const noexcept {
		return m_node_list.end();
	}
	decltype(m_node_list)::iterator end() noexcept {
		return m_node_list.end();
	}
	const Expression_node &at(std::size_t n) const {
		return m_node_list.at(n);
	}
	Expression_node &at(std::size_t n){
		return m_node_list.at(n);
	}
	template<typename ValueT>
	void append(ValueT &&value){
		m_node_list.emplace_back(std::forward<ValueT>(value));
	}
	void clear() noexcept {
		return m_node_list.clear();
	}
};

extern Reference evaluate_expression_recursive(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt);

}

#endif
