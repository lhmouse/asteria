// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_BLOCK_HPP_
#define ASTERIA_BLOCK_HPP_

#include "fwd.hpp"
#include "statement.hpp"

namespace Asteria {

class Block {
	friend Statement;

public:
	enum Execution_result : unsigned {
		execution_result_next      = 0,
		execution_result_break     = 1,
		execution_result_continue  = 2,
		execution_result_return    = 3,
	};

private:
	std::vector<Statement> m_statements;

public:
	Block(std::vector<Statement> statements)
		: m_statements(std::move(statements))
	{ }
	Block(Block &&) noexcept;
	Block &operator=(Block &&);
	~Block();

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
	decltype(m_statements)::const_reference at(std::size_t n) const {
		return m_statements.at(n);
	}
};

extern void bind_block(Xptr<Block> &bound_result_out, Spcref<const Block> block_opt, Spcref<const Scope> scope);
extern Block::Execution_result execute_block(Xptr<Reference> &reference_out, Spcref<Recycler> recycler, Spcref<const Block> block_opt, Spcref<const Scope> scope);

}

#endif
