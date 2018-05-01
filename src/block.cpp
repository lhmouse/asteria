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

}
