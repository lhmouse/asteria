// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ANALYTIC_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ANALYTIC_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Analytic_Context
  : public Abstract_Context
  {
  private:
    const Abstract_Context* m_parent_opt;

  public:
    Analytic_Context(ref_to<const Abstract_Context> parent, nullptr_t)  // for non-functions
      : m_parent_opt(parent.ptr())
      { }

    Analytic_Context(const Abstract_Context* parent_opt,  // for functions
                     const cow_vector<phsh_string>& params)
      : m_parent_opt(parent_opt)
      { this->do_prepare_function(params);  }

    ~Analytic_Context()
    override;

  private:
    void
    do_prepare_function(const cow_vector<phsh_string>& params);

  protected:
    bool
    do_is_analytic()
    const noexcept final
      { return this->is_analytic();  }

    const Abstract_Context*
    do_get_parent_opt()
    const noexcept override
      { return this->get_parent_opt();  }

    Reference*
    do_lazy_lookup_opt(const phsh_string& /*name*/)
    override
      { return nullptr;  }

  public:
    bool
    is_analytic()
    const noexcept
      { return true;  }

    const Abstract_Context*
    get_parent_opt()
    const noexcept
      { return this->m_parent_opt;  }
  };

}  // namespace Asteria

#endif
