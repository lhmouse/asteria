// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SHARED_OPAQUE_WRAPPER_HPP_
#define ASTERIA_SHARED_OPAQUE_WRAPPER_HPP_

#include "fwd.hpp"
#include "abstract_opaque.hpp"

namespace Asteria {

class Shared_opaque_wrapper
  {
  private:
    Rcptr<Abstract_opaque> m_ptr;

  public:
    template<typename ElementT, typename std::enable_if<std::is_base_of<Abstract_opaque, typename std::decay<ElementT>::type>::value>::type * = nullptr>
      explicit Shared_opaque_wrapper(ElementT &&elem)
        : m_ptr(rocket::make_refcounted<typename std::decay<ElementT>::type>(std::forward<ElementT>(elem)))
        {
        }

  public:
    const Abstract_opaque * get() const noexcept
      {
        return this->m_ptr.get();
      }
    Abstract_opaque * mut()
      {
        if(this->m_ptr.unique() == false) {
          this->m_ptr->clone(this->m_ptr);
          ROCKET_ASSERT(this->m_ptr.unique());
        }
        return this->m_ptr.get();
      }
  };

}

#endif
