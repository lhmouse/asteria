// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EVALUATION_STACK_HPP_
#define ASTERIA_RUNTIME_EVALUATION_STACK_HPP_

#include "../fwd.hpp"
#include "reference_stack.hpp"

namespace Asteria {

class Evaluation_Stack
  {
  private:
    Reference_Stack m_references;
    Rcptr<Variable> m_last_variable_opt;

  public:
    Evaluation_Stack() noexcept
      {
      }
    virtual ~Evaluation_Stack();

    Evaluation_Stack(const Evaluation_Stack &)
      = delete;
    Evaluation_Stack & operator=(const Evaluation_Stack &)
      = delete;

  public:
    std::size_t get_reference_count() const noexcept
      {
        return this->m_references.size();
      }
    void clear_references() noexcept
      {
        this->m_references.clear();
      }

    const Reference & get_top_reference() const noexcept
      {
        return this->m_references.get(0);
      }
    Reference & open_top_reference() noexcept
      {
        return this->m_references.mut(0);
      }
    template<typename ParamT> Reference & push_reference(ParamT &&param)
      {
        return this->m_references.push(std::forward<ParamT>(param));
      }
    void pop_reference() noexcept
      {
        this->m_references.pop();
      }
    void pop_previous_reference() noexcept
      {
        this->m_references.mut(1) = rocket::move(this->m_references.mut(0));
        this->m_references.pop();
      }

    void set_last_variable(Rcptr<Variable> &&var_opt) noexcept
      {
        this->m_last_variable_opt = rocket::move(var_opt);
      }
    Rcptr<Variable> release_last_variable_opt() noexcept
      {
        return std::exchange(this->m_last_variable_opt, nullptr);
      }
  };

}  // namespace Asteria

#endif
