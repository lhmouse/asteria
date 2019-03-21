// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "evaluation_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

Evaluation_Stack::~Evaluation_Stack()
  {
  }

void Evaluation_Stack::set_temporary_result(bool assign, Value&& value)
  {
    // Do not play with this at home.
    ROCKET_ASSERT(this->m_refs.size() >= 1);
    if(assign) {
      // Write the value to the top refernce.
      this->m_refs.get(0).open() = rocket::move(value);
      return;
    }
    // Replace the top reference to a temporary reference to the value.
    Reference_Root::S_temporary ref_c = { rocket::move(value) };
    this->m_refs.mut(0) = rocket::move(ref_c);
  }

void Evaluation_Stack::forward_result(bool assign)
  {
    // Do not play with this at home.
    ROCKET_ASSERT(this->m_refs.size() >= 2);
    if(assign) {
      // Read a temporary value from the top reference and pop it.
      // Set the new top to a temporary reference to the value.
      Reference_Root::S_temporary ref_c = { this->m_refs.get(0).read() };
      this->m_refs.mut(1) = rocket::move(ref_c);
      this->m_refs.pop();
      return;
    }
    // Remove the next reference from the top.
    this->m_refs.mut(1) = rocket::move(this->m_refs.mut(0));
    this->m_refs.pop();
  }

}  // namespace Asteria
