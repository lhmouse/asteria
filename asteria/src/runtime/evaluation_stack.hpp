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
    Reference_Stack m_refs;
    Rcptr<Variable> m_lvar_opt;

  public:
    Evaluation_Stack() noexcept
      : m_refs()
      {
      }
    virtual ~Evaluation_Stack();

    Evaluation_Stack(const Evaluation_Stack&)
      = delete;
    Evaluation_Stack& operator=(const Evaluation_Stack&)
      = delete;

  public:
    std::size_t get_reference_count() const noexcept
      {
        return this->m_refs.size();
      }
    void clear_references() noexcept
      {
        this->m_refs.clear();
      }
   void reserve_references(Cow_Vector<Reference>&& stor) noexcept
     {
       this->m_refs.reserve(rocket::move(stor));
     }

    const Reference& get_top_reference() const noexcept
      {
        return this->m_refs.get(0);
      }
    Reference& open_top_reference() noexcept
      {
        return this->m_refs.mut(0);
      }
    template<typename ParamT> Reference& push_reference(ParamT&& param)
      {
        return this->m_refs.push(rocket::forward<ParamT>(param));
      }
    void pop_reference() noexcept
      {
        this->m_refs.pop();
      }

    void set_last_variable(Rcptr<Variable>&& var_opt) noexcept
      {
        this->m_lvar_opt = rocket::move(var_opt);
      }
    Rcptr<Variable> release_last_variable_opt() noexcept
      {
        return std::exchange(this->m_lvar_opt, nullptr);
      }

    // These are auxiliary functions purely for evaluation of expressions/
    // Do not play with these at home.
    void set_temporary_result(bool assign, Value&& value);
    void forward_result(bool assign);
  };

}  // namespace Asteria

#endif
