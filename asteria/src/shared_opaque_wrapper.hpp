// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SHARED_OPAQUE_WRAPPER_HPP_
#define ASTERIA_SHARED_OPAQUE_WRAPPER_HPP_

#include "fwd.hpp"
#include "abstract_opaque.hpp"
#include "rocket/refcounted_ptr.hpp"
#include <functional>

namespace Asteria {

class Shared_opaque_wrapper
  {
  private:
    rocket::refcounted_ptr<Abstract_opaque> m_owns;
    std::reference_wrapper<Abstract_opaque> m_ref;

  public:
    template<typename ElementT, typename std::enable_if<std::is_base_of<Abstract_opaque, typename std::decay<ElementT>::type>::value>::type * = nullptr>
      explicit Shared_opaque_wrapper(ElementT &&elem)
      : m_owns(rocket::make_refcounted<typename std::decay<ElementT>::type>(std::forward<ElementT>(elem))), m_ref(*(this->m_owns))
      {
      }
    template<typename ElementT, typename std::enable_if<std::is_base_of<Abstract_opaque, typename std::decay<ElementT>::type>::value>::type * = nullptr>
      explicit Shared_opaque_wrapper(std::reference_wrapper<ElementT> elem)
      : m_owns(nullptr), m_ref(std::move(elem))
      {
      }

  public:
    const Abstract_opaque & get() const noexcept
      {
        return this->m_ref;
      }
    Abstract_opaque & mut()
      {
        if(this->m_owns && !this->m_owns.unique()) {
          rocket::refcounted_ptr<Abstract_opaque> owns;
          const auto ptr = this->m_owns->clone(owns);
          ROCKET_ASSERT(owns.unique());
          ROCKET_ASSERT(owns.get() == ptr);
          this->m_owns = std::move(owns);
          this->m_ref = *ptr;
        }
        return this->m_ref;
      }
    const Abstract_opaque & operator*() const noexcept
      {
        return this->m_ref;
      }
    const Abstract_opaque * operator->() const noexcept
      {
        return std::addressof(this->m_ref.get());
      }
  };

}

#endif
