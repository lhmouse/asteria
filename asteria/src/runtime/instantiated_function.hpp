// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_

#include "../fwd.hpp"
#include "variadic_arguer.hpp"
#include "../llds/avmc_queue.hpp"

namespace Asteria {

class Instantiated_Function
final
  : public Abstract_Function
  {
  private:
    cow_vector<phsh_string> m_params;
    rcptr<Variadic_Arguer> m_zvarg;
    AVMC_Queue m_queue;

  public:
    Instantiated_Function(const cow_vector<phsh_string>& params, rcptr<Variadic_Arguer>&& zvarg,
                          const cow_vector<AIR_Node>& code)
      : m_params(params), m_zvarg(::std::move(zvarg))
      { this->do_solidify(code);  }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(Instantiated_Function);

  private:
    void do_solidify(const cow_vector<AIR_Node>& code);

  public:
    tinyfmt&
    describe(tinyfmt& fmt)
    const override;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const override;

    Reference&
    invoke_ptc_aware(Reference& self, Global_Context& global, cow_vector<Reference>&& args)
    const override;
  };

}  // namespace Asteria

#endif
