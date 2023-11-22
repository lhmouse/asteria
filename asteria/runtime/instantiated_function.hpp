// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_
#define ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_

#include "../fwd.hpp"
#include "variadic_arguer.hpp"
#include "../llds/avm_rod.hpp"
namespace asteria {

class Instantiated_Function final
  :
    public Abstract_Function
  {
  private:
    cow_vector<phsh_string> m_params;
    refcnt_ptr<Variadic_Arguer> m_zvarg;
    AVM_Rod m_rod;

  public:
    explicit
    Instantiated_Function(const cow_vector<phsh_string>& params, refcnt_ptr<Variadic_Arguer>&& zvarg,
                          const cow_vector<AIR_Node>& code);

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Instantiated_Function);

    tinyfmt&
    describe(tinyfmt& fmt) const override;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const override;

    Reference&
    invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override;
  };

}  // namespace asteria
#endif
