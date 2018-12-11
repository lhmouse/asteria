// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

Reference_stack::~Reference_stack()
  {
    auto cur = this->m_scur;
    while(cur) {
      const auto prev = cur->prev;
      if(cur != &(this->m_head)) {
        delete cur;
      }
      cur = prev;
    }
  }

void Reference_stack::do_clear(Generational_collector *coll_opt) noexcept
  {
    auto cur = this->m_scur;
    while(cur) {
      const auto prev = cur->prev;
      while(!cur->refs.empty()) {
        cur->refs.back().dispose_variable(coll_opt);
        cur->refs.pop_back();
      }
      cur = prev;
    }
    this->m_scur = nullptr;
    this->m_size = 0;
  }

Reference_stack::Chunk * Reference_stack::do_reserve_one_more()
  {
    auto cur = this->m_scur;
    if(ROCKET_EXPECT(!cur)) {
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
