// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_BLOCK_HPP_
#define ASTERIA_BLOCK_HPP_

#include "fwd.hpp"
#include "statement.hpp"

namespace Asteria {

class Block {
public:
	enum Execution_result : std::uint8_t {
		execution_result_end_of_block          = 0,
		execution_result_break_unspecified     = 1,
		execution_result_break_switch          = 2,
		execution_result_break_while           = 3,
		execution_result_break_for             = 4,
		execution_result_continue_unspecified  = 5,
		execution_result_continue_while        = 6,
		execution_result_continue_for          = 7,
		execution_result_return                = 8,
	};

private:
	T_vector<Statement> m_statements;

public:
	Block(T_vector<Statement> statements)
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

extern void bind_block_in_place(Vp<Block> &bound_result_out, Spr<Scope> scope, Spr<const Block> block_opt);
extern Block::Execution_result execute_block_in_place(Vp<Reference> &reference_out, Spr<Scope> scope, Spr<Recycler> recycler, Spr<const Block> block_opt);

extern void bind_block(Vp<Block> &bound_result_out, Spr<const Block> block_opt, Spr<const Scope> scope);
extern Block::Execution_result execute_block(Vp<Reference> &reference_out, Spr<Recycler> recycler, Spr<const Block> block_opt, Spr<const Scope> scope);

}

#endif
