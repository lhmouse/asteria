// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "block.hpp"
#include "statement.hpp"
#include "reference.hpp"
#include "scope.hpp"
#include "utilities.hpp"

namespace Asteria {

Block::Block(Block &&) noexcept = default;
Block & Block::operator=(Block &&) noexcept = default;
Block::~Block() = default;

void bind_block_in_place(Vp<Block> &bound_block_out, Sp_ref<Scope> scope_inout, Sp_ref<const Block> block_opt){
	if(block_opt == nullptr){
		// Return a null block.
		bound_block_out.reset();
		return;
	}
	// Bind statements recursively.
	Vector<Statement> bound_stmts;
	bound_stmts.reserve(block_opt->size());
	for(const auto &stmt : *block_opt){
		bind_statement_in_place(bound_stmts, scope_inout, stmt);
	}
	bound_block_out.emplace(std::move(bound_stmts));
}
void fly_over_block_in_place(Sp_ref<Scope> scope_inout, Sp_ref<const Block> block_opt){
	if(block_opt == nullptr){
		// Nothing to do.
		return;
	}
	// Skip statements one by one.
	for(const auto &stmt : *block_opt){
		fly_over_statement_in_place(scope_inout, stmt);
	}
}
Statement::Execution_result execute_block_in_place(Vp<Reference> &ref_out, Sp_ref<Scope> scope_inout, Sp_ref<Recycler> recycler_out, Sp_ref<const Block> block_opt){
	if(block_opt == nullptr){
		// Nothing to do.
		move_reference(ref_out, nullptr);
		return Statement::execution_result_next;
	}
	// Execute statements one by one.
	for(const auto &stmt : *block_opt){
		const auto result = execute_statement_in_place(ref_out, scope_inout, recycler_out, stmt);
		if(result != Statement::execution_result_next){
			// Forward anything unexpected to the caller.
			return result;
		}
	}
	return Statement::execution_result_next;
}

void bind_block(Vp<Block> &bound_block_out, Sp_ref<const Block> block_opt, Sp_ref<const Scope> scope){
	if(block_opt == nullptr){
		// Return a null block.
		bound_block_out.reset();
		return;
	}
	const auto scope_working = std::make_shared<Scope>(Scope::purpose_lexical, scope);
	bind_block_in_place(bound_block_out, scope_working, block_opt);
}
Statement::Execution_result execute_block(Vp<Reference> &ref_out, Sp_ref<Recycler> recycler_out, Sp_ref<const Block> block_opt, Sp_ref<const Scope> scope){
	if(block_opt == nullptr){
		// Nothing to do.
		move_reference(ref_out, nullptr);
		return Statement::execution_result_next;
	}
	const auto scope_working = std::make_shared<Scope>(Scope::purpose_plain, scope);
	return execute_block_in_place(ref_out, scope_working, recycler_out, block_opt);
}

}
