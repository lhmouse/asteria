// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_GLOBAL_CONTEXT_HPP_
#define ASTERIA_GLOBAL_CONTEXT_HPP_

#include "fwd.hpp"
#include "abstract_context.hpp"
#include "reference_dictionary.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Global_context : public Abstract_context
  {
  private:
    Reference_dictionary m_dict;
    rocket::refcounted_ptr<Global_collector> m_gcoll;

  public:
    Global_context();
    ROCKET_NONCOPYABLE_DESTRUCTOR(Global_context);

  public:
    bool is_analytic() const noexcept override;
    const Abstract_context * get_parent_opt() const noexcept override;

    const Reference * get_named_reference_opt(const rocket::prehashed_string &name) const override;
    void set_named_reference(const rocket::prehashed_string &name, Reference ref) override;

    rocket::refcounted_ptr<Variable> create_tracked_variable();
    void perform_garbage_collection(unsigned gen_limit);
  };

}

#endif
