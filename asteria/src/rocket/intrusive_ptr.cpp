// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "intrusive_ptr.hpp"

namespace rocket {

namespace details_intrusive_ptr {

  refcount_base::~refcount_base()
    {
      // The reference count shall be either zero or one here.
      if(this->m_nref.load(::std::memory_order_relaxed) > 1) {
        ::std::terminate();
      }
    }

}

}
