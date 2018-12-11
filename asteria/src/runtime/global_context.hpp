// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "../rocket/unique_ptr.hpp"

namespace Asteria {

class Global_context : public Abstract_context
  {
  private:
    rocket::refcounted_ptr<Generational_collector> m_gen_coll;

  public:
    Global_context();
    ROCKET_NONCOPYABLE_DESTRUCTOR(Global_context);

  public:
    bool is_analytic() const noexcept override;
    const Abstract_context * get_parent_opt() const noexcept override;

    Generational_collector & get_generational_collector() const noexcept
      {
        return *(this->m_gen_coll);
      }
  };

}

#endif
