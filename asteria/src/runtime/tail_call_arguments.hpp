// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_TAIL_CALL_ARGUMENTS_HPP_
#define ASTERIA_RUNTIME_TAIL_CALL_ARGUMENTS_HPP_

#include "../fwd.hpp"
#include "tail_call_arguments_fwd.hpp"
#include "variadic_arguer.hpp"
#include "../source_location.hpp"
#include "../abstract_function.hpp"

namespace Asteria {

class Tail_Call_Arguments final : public Tail_Call_Arguments_Fwd
  {
  private:
    // These describe characteristics of the function call.
    Source_Location m_sloc;
    rcptr<Variadic_Arguer> m_zvarg;
    PTC_Aware m_ptc;

    // These are deferreed expressions.
    cow_bivector<Source_Location, AVMC_Queue> m_defer;

    // This is the target function.
    rcptr<Abstract_Function> m_target;
    // The last reference is `self`.
    // We can't store a `Reference` directly as the class `Reference` is incomplete here.
    cow_vector<Reference> m_args_self;

  public:
    Tail_Call_Arguments(const Source_Location& sloc, const rcptr<Variadic_Arguer>& zvarg, PTC_Aware ptc,
                        const rcptr<Abstract_Function>& target, cow_vector<Reference>&& args_self)
      :
        m_sloc(sloc), m_zvarg(zvarg), m_ptc(ptc),
        m_target(target), m_args_self(::rocket::move(args_self))
      {
      }
    ~Tail_Call_Arguments() override;

    Tail_Call_Arguments(const Tail_Call_Arguments&)
      = delete;
    Tail_Call_Arguments& operator=(const Tail_Call_Arguments&)
      = delete;

  public:
    const Source_Location& sloc() const noexcept
      {
        return this->m_sloc;
      }
    const rcptr<Variadic_Arguer>& zvarg() const noexcept
      {
        return this->m_zvarg;
      }
    PTC_Aware ptc_aware() const noexcept
      {
        return this->m_ptc;
      }

    const cow_bivector<Source_Location, AVMC_Queue>& get_defer_stack() const noexcept
      {
        return this->m_defer;
      }
    cow_bivector<Source_Location, AVMC_Queue>& open_defer_stack() noexcept
      {
        return this->m_defer;
      }

    const rcptr<Abstract_Function>& get_target() const noexcept
      {
        return this->m_target;
      }
    const cow_vector<Reference>& get_arguments_and_self() const noexcept
      {
        return this->m_args_self;
      }
    cow_vector<Reference>& open_arguments_and_self() noexcept
      {
        return this->m_args_self;
      }

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

}  // namespace Asteria

#endif
