// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_GLOBAL_CONTEXT_HPP_
#define ASTERIA_GLOBAL_CONTEXT_HPP_

#include "fwd.hpp"
#include "abstract_context.hpp"
#include "global_collector.hpp"

namespace Asteria {

class Global_context : public Abstract_context
  {
  private:
    rocket::refcounted_ptr<Global_collector> m_collector_opt;

  public:
    Global_context() noexcept
      : m_collector_opt()
      {
      }
    ~Global_context();

  public:
    bool is_analytic() const noexcept override;
    const Abstract_context * get_parent_opt() const noexcept override;

    rocket::refcounted_ptr<Variable> create_tracked_variable();
    void perform_garbage_collection(unsigned gen_max, bool unreserve);
  };

}

#endif
