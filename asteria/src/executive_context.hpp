// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_EXECUTIVE_CONTEXT_HPP_

#include "fwd.hpp"
#include "abstract_context.hpp"
#include "reference_dictionary.hpp"

namespace Asteria {

class Executive_context : public Abstract_context
  {
  private:
    const Executive_context *const m_parent_opt;
    Reference_dictionary m_dict;
    Reference m_file;
    Reference m_line;
    Reference m_func;
    Reference m_self;
    Reference m_varg;

  public:
    explicit Executive_context(const Executive_context *parent_opt = nullptr) noexcept
      : m_parent_opt(parent_opt)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Executive_context);

  public:
    bool is_analytic() const noexcept override;
    const Executive_context * get_parent_opt() const noexcept override;

    const Reference * get_named_reference_opt(const rocket::prehashed_string &name) const override;
    void set_named_reference(const rocket::prehashed_string &name, Reference ref) override;

    void initialize_for_function(Global_context &global, const Function_header &head, const Shared_function_wrapper *zvarg_opt, Reference self, rocket::cow_vector<Reference> args);
  };

}

#endif
