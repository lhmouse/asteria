// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_TAIL_CALL_ARGUMENTS_HPP_
#define ASTERIA_RUNTIME_TAIL_CALL_ARGUMENTS_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"
#include "source_location.hpp"

namespace Asteria {

class Tail_Call_Arguments final : public virtual Rcbase
  {
  private:
    Source_Location m_sloc;
    cow_string m_func;
    TCO_Aware m_tco_aware;

    rcobj<Abstract_Function> m_target;
    // The last reference is `self`. We can't store a `Reference` directly as the class has not been defined here.
    cow_vector<Reference> m_args_s;

  public:
    Tail_Call_Arguments(const Source_Location& sloc, const cow_string& func, TCO_Aware tco_aware,
                        const rcobj<Abstract_Function>& target, cow_vector<Reference>&& args_s)
      : m_sloc(sloc), m_func(func), m_tco_aware(tco_aware),
        m_target(target), m_args_s(rocket::move(args_s))
      {
      }
    ~Tail_Call_Arguments() override;

    Tail_Call_Arguments(const Tail_Call_Arguments&)
      = delete;
    Tail_Call_Arguments& operator=(const Tail_Call_Arguments&)
      = delete;

  public:
    const Source_Location& get_source_location() const noexcept
      {
        return this->m_sloc;
      }
    const cow_string& get_function_signature() const noexcept
      {
        return this->m_func;
      }
    TCO_Aware get_tco_awareness() const noexcept
      {
        return this->m_tco_aware;
      }

    const rcobj<Abstract_Function>& get_target() const noexcept
      {
        return this->m_target;
      }
    rcobj<Abstract_Function>& open_target() noexcept
      {
        return this->m_target;
      }
    const cow_vector<Reference>& get_arguments_and_self() const noexcept
      {
        return this->m_args_s;
      }
    cow_vector<Reference>& open_arguments_and_self() noexcept
      {
        return this->m_args_s;
      }

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

}  // namespace Asteria

#endif
