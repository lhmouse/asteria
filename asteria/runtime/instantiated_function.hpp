// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_
#define ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_

#include "../fwd.hpp"
#include "../llds/avm_rod.hpp"
namespace asteria {

class Instantiated_Function
  :
    public Abstract_Function
  {
  private:
    Source_Location m_sloc;
    cow_string m_func;
    cow_vector<phcow_string> m_params;
    AVM_Rod m_rod;

  public:
    Instantiated_Function(const Source_Location& xsloc, const cow_string& xfunc,
                          const cow_vector<phcow_string>& xparams, const cow_vector<AIR_Node>& code);

  public:
    Instantiated_Function(const Instantiated_Function&) = delete;
    Instantiated_Function& operator=(const Instantiated_Function&) & = delete;
    ~Instantiated_Function();

    const Source_Location&
    sloc() const noexcept
      { return this->m_sloc;  }

    const cow_string&
    func() const noexcept
      { return this->m_func;  }

    const cow_vector<phcow_string>&
    params() const noexcept
      { return this->m_params;  }

    tinyfmt&
    describe(tinyfmt& fmt) const override;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const override;

    void
    invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override;
  };

}  // namespace asteria
#endif
