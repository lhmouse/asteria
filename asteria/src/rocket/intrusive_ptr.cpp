// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "intrusive_ptr.hpp"

namespace rocket {

namespace details_intrusive_ptr {

  refcount_base::~refcount_base()
    {
      ROCKET_ASSERT(this->m_nref.load(::std::memory_order_relaxed) <= 1);
    }

}

}
