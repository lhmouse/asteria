// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPOUND_STATEMENT_HPP_
#define ASTERIA_COMPOUND_STATEMENT_HPP_

#include "fwd.hpp"
#include "statement.hpp"

namespace Asteria {

class Compound_statement {
	friend Statement;

public:
	enum Execute_result : unsigned {
		execute_result_next      = 0,
		execute_result_break     = 1,
		execute_result_continue  = 2,
		execute_result_return    = 3,
	};

private:
	std::vector<Statement> m_statements;

public:
	Compound_statement(std::vector<Statement> statements)
		: m_statements(std::move(statements))
	{ }
	Compound_statement(Compound_statement &&) noexcept;
	Compound_statement &operator=(Compound_statement &&);
	~Compound_statement();

public:
	bool empty() const noexcept {
		return m_statements.empty();
	}
	std::size_t size() const noexcept {
		return m_statements.size();
	}
	decltype(m_statements)::const_iterator begin() const noexcept {
		return m_statements.begin();
	}
	decltype(m_statements)::const_iterator end() const noexcept {
		return m_statements.end();
	}
	const Statement &at(std::size_t n) const {
		return m_statements.at(n);
	}
};

extern void bind_compound_statement(Xptr<Compound_statement> &compound_statement_out, Spcref<const Compound_statement> source_opt, Spcref<const Scope> scope);
extern Compound_statement::Execute_result execute_compound_statement(Xptr<Reference> &reference_out, Spcref<Recycler> recycler, Spcref<const Compound_statement> compound_statement_opt, Spcref<const Scope> scope);

}

#endif
