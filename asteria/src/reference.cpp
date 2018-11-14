// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "global_context.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::~Reference()
  {
  }

void Reference::do_throw_no_modifier() const
  {
    ASTERIA_THROW_RUNTIME_ERROR("Only array elements or object members may be `unset`.");
  }

Reference & Reference::convert_to_temporary()
  {
    if(this->m_root.index() == Reference_root::index_temporary) {
      return *this;
    }
    // Create an rvalue.
    Reference_root::S_temporary ref_c = { this->read() };
    *this = std::move(ref_c);
    return *this;
  }

Reference & Reference::convert_to_variable(Global_context &global, bool immutable)
  {
    if(this->m_root.index() == Reference_root::index_variable) {
      return *this;
    }
    if((this->m_root.index() == Reference_root::index_constant) && immutable) {
      return *this;
    }
    // Create an lvalue by allocating a variable and assign it to `*this`.
    auto var = global.create_tracked_variable(this, immutable);
    Reference_root::S_variable ref_c = { std::move(var) };
    *this = std::move(ref_c);
    return *this;
  }

}
