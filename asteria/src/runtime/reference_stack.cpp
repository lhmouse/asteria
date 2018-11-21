// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_stack.hpp"

namespace Asteria {

Reference_stack::~Reference_stack()
  {
    auto cur = this->m_scur;
    while(cur) {
      auto prev = cur->prev;
      if(cur != &(this->m_head)) {
        delete cur;
      }
      cur = prev;
    }
  }

Reference_stack::Chunk * Reference_stack::do_reserve_one_more()
  {
    auto cur = this->m_scur;
    if(!cur) {
      return &(this->m_head);
    }
    auto next = cur->next;
    if(!next) {
      next = new Chunk(cur);
      cur->next = next;
    }
    return next;
  }

}
