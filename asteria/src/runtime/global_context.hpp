// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/unique_ptr.hpp"

namespace Asteria {

class Global_context : public Abstract_context
  {
  private:
    rocket::refcounted_ptr<Global_collector> m_gcoll;
    rocket::cow_vector<rocket::unique_ptr<Executive_context>> m_ectx_pool;
    rocket::cow_vector<rocket::unique_ptr<Reference_stack>> m_stack_pool;

  public:
    Global_context();
    ROCKET_NONCOPYABLE_DESTRUCTOR(Global_context);

  public:
    bool is_analytic() const noexcept override;
    const Abstract_context * get_parent_opt() const noexcept override;

    void track_variable(const rocket::refcounted_ptr<Variable> &var);
    bool untrack_variable(const rocket::refcounted_ptr<Variable> &var) noexcept;
    void perform_garbage_collection(unsigned gen_limit);

    rocket::unique_ptr<Executive_context> allocate_executive_context();
    bool return_executive_context(rocket::unique_ptr<Executive_context> &&ctx) noexcept;

    rocket::unique_ptr<Reference_stack> allocate_reference_stack();
    bool return_reference_stack(rocket::unique_ptr<Reference_stack> &&ctx) noexcept;
  };

}

#endif
