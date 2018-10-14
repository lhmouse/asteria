// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_EXECUTIVE_CONTEXT_HPP_

#include "fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Executive_context : public Abstract_context
  {
  private:
    const Executive_context *m_parent_opt;
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
    ~Executive_context();

  public:
    bool is_analytic() const noexcept override;
    const Executive_context * get_parent_opt() const noexcept override;
    const Reference * get_named_reference_opt(const String &name) const override;

    void initialize_for_function(Global_context &global, const Function_header &head, const Shared_function_wrapper *zvarg_opt, Reference self, Vector<Reference> args);
  };

}

#endif
