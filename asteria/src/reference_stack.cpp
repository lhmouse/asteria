// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_stack.hpp"

namespace Asteria {

Reference_stack::~Reference_stack()
  {
    auto cur = this->m_last;
    while(cur) {
      auto prev = cur->prev;
      if(cur != &(this->m_head)) {
        delete cur;
      }
      cur = prev;
    }
  }

Reference_stack::Storage * Reference_stack::do_reserve_one_more()
  {
    auto cur = this->m_last;
    auto off = this->m_size % this->m_head.refs.capacity();
    if(off != 0) {
      // The last block is not full.
      return cur;
    }
    // The last block is full so we have to allocate a new block.
    if(!cur) {
      return &(this->m_head);
    }
    auto next = cur->next;
    if(next) {
      return next;
    }
    next = new Storage(cur);
    cur->next = next;
    return next;
  }

}
