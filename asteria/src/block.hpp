// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_BLOCK_HPP_
#define ASTERIA_BLOCK_HPP_

#include "fwd.hpp"
#include "statement.hpp"

namespace Asteria {

class Block {
private:
	Vector<Statement> m_statements;

public:
	Block(Vector<Statement> statements)
		: m_statements(std::move(statements))
	{ }
	Block(Block &&) noexcept;
	Block & operator=(Block &&) noexcept;
	~Block();

public:
	bool empty() const noexcept {
		return m_statements.empty();
	}
	std::size_t size() const noexcept {
		return m_statements.size();
	}
	const Statement & at(std::size_t n) const {
		return m_statements.at(n);
	}
	const Statement * begin() const noexcept {
		return m_statements.data();
	}
	const Statement * end() const noexcept {
		return m_statements.data() + m_statements.size();
	}
};

extern void bind_block_in_place(Vp<Block> &bound_block_out, Spr<Scope> scope_inout, Spr<const Block> block_opt);
extern void fly_over_block_in_place(Spr<Scope> scope_inout, Spr<const Block> block_opt);
extern Statement::Execution_result execute_block_in_place(Vp<Reference> &reference_out, Spr<Scope> scope_inout, Spr<Recycler> recycler_out, Spr<const Block> block_opt);

extern void bind_block(Vp<Block> &bound_block_out, Spr<const Block> block_opt, Spr<const Scope> scope);
extern Statement::Execution_result execute_block(Vp<Reference> &reference_out, Spr<Recycler> recycler_out, Spr<const Block> block_opt, Spr<const Scope> scope);

}

#endif
