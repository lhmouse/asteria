// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "block.hpp"
#include "statement.hpp"
#include "utilities.hpp"

namespace Asteria {

Block::Block(Block &&) noexcept = default;
Block &Block::operator=(Block &&) = default;
Block::~Block() = default;

void bind_block(Xptr<Block> &block_out, Spcref<const Block> source_opt, Spcref<const Scope> scope){
	(void)block_out;
	(void)source_opt;
	(void)scope;
}
Block::Execution_result execute_block(Xptr<Reference> &reference_out, Spcref<Recycler> recycler, Spcref<const Block> block_opt, Spcref<const Scope> scope){
	(void)reference_out;
	(void)recycler;
	(void)block_opt;
	(void)scope;
	return Block::execution_result_next;
}

}
