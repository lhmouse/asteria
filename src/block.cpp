// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "block.hpp"
#include "statement.hpp"
#include "scope.hpp"
#include "expression.hpp"
#include "initializer.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Block::Block(Block &&) noexcept = default;
Block &Block::operator=(Block &&) = default;
Block::~Block() = default;

void bind_block(Xptr<Block> &bound_result_out, Spcref<const Block> block_opt, Spcref<const Scope> scope){
	(void)block_opt;
	(void)scope;
	return bound_result_out.reset();
}
Block::Execution_result execute_block(Xptr<Reference> &reference_out, Spcref<Recycler> recycler, Spcref<const Block> block_opt, Spcref<const Scope> scope, std::size_t offset){
	(void)reference_out;
	(void)recycler;
	(void)block_opt;
	(void)scope;
	(void)offset;
	return Block::execution_result_end_of_block;
}

}
