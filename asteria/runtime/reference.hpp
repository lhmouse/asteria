// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_
#define ASTERIA_RUNTIME_REFERENCE_

#include "../fwd.hpp"
#include "../value.hpp"
#include "subscript.hpp"
namespace asteria {

class Reference
  {
  private:
    struct St_invalid  { };
    struct St_void  { };
    using St_ptc = rcfwd_ptr<PTC_Arguments>;

    struct St_temp
      {
        Value val;
        cow_vector<Subscript> subs;
      };

    struct St_var
      {
        rcfwd_ptr<Variable> var;
        cow_vector<Subscript> subs;
      };

    ::rocket::variant<St_invalid, St_void, St_temp, St_var, St_ptc> m_stor;

  public:
    // Constructors and assignment operators
    constexpr Reference() noexcept { }

    Reference(const Reference& other) noexcept
      :
        m_stor(other.m_stor)
      { }

    Reference&
    operator=(const Reference& other) & noexcept
      {
        this->m_stor = other.m_stor;
        return *this;
      }

    Reference(Reference&& other) noexcept
      :
        m_stor(move(other.m_stor))
      { }

    Reference&
    operator=(Reference&& other) & noexcept
      {
        this->m_stor = move(other.m_stor);
        return *this;
      }

    Reference&
    swap(Reference& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  private:
    [[noreturn]]
    void
    do_throw_not_dereferenceable() const;

    const Value&
    do_dereference_readonly_slow() const;

    Value&
    do_dereference_copy_slow();

    void
    do_use_function_result_slow(Global_Context& global);

  public:
    ~Reference();

    // Accessors
    bool
    is_invalid() const noexcept
      { return this->m_stor.ptr<St_invalid>() != nullptr;  }

    Reference&
    clear() noexcept
      {
        this->m_stor.emplace<St_invalid>();
        return *this;
      }

    bool
    is_void() const noexcept
      { return this->m_stor.ptr<St_void>() != nullptr;  }

    Reference&
    set_void() noexcept
      {
        this->m_stor.emplace<St_void>();
        return *this;
      }

    bool
    is_temporary() const noexcept
      { return this->m_stor.ptr<St_temp>() != nullptr;  }

    template<typename xValue,
    ROCKET_ENABLE_IF(::std::is_assignable<Value&, xValue&&>::value)>
    Reference&
    set_temporary(xValue&& xval)
      {
        auto& st = this->m_stor.emplace<St_temp>();
        st.val = forward<xValue>(xval);
        return *this;
      }

    bool
    is_variable() const noexcept
      { return this->m_stor.ptr<St_var>() != nullptr;  }

    ASTERIA_INCOMPLET(Variable)
    Reference&
    set_variable(const refcnt_ptr<Variable>& var)
      {
        ROCKET_ASSERT(var != nullptr);
        auto& st = this->m_stor.emplace<St_var>();
        st.var = var;
        return *this;
      }

    bool
    is_ptc() const noexcept
      { return this->m_stor.ptr<St_ptc>() != nullptr;  }

    ASTERIA_INCOMPLET(PTC_Arguments)
    Reference&
    set_ptc(const refcnt_ptr<PTC_Arguments>& ptc)
      {
        ROCKET_ASSERT(ptc != nullptr);
        this->m_stor.emplace<St_ptc>(ptc);
        return *this;
      }

    template<typename xSubscript,
    ROCKET_ENABLE_IF(::std::is_constructible<Subscript, xSubscript&&>::value)>
    Reference&
    push_subscript(xSubscript&& xsub)
      {
        cow_vector<Subscript>* subs;
        if(auto st1 = this->m_stor.mut_ptr<St_temp>())
          subs = &(st1->subs);
        else if(auto st2 = this->m_stor.mut_ptr<St_var>())
          subs = &(st2->subs);
        else
          this->do_throw_not_dereferenceable();

        subs->emplace_back(forward<xSubscript>(xsub));
        return *this;
      }

    Reference&
    pop_subscript(size_t count = 1) noexcept
      {
        cow_vector<Subscript>* subs;
        if(auto st1 = this->m_stor.mut_ptr<St_temp>())
          subs = &(st1->subs);
        else if(auto st2 = this->m_stor.mut_ptr<St_var>())
          subs = &(st2->subs);
        else
          this->do_throw_not_dereferenceable();

        if(count <= subs->size())
          subs->pop_back(count);
        else
          this->m_stor.emplace<St_void>();
        return *this;
      }

    // These functions are used internally by the runtime, whose names have
    // been obscured to make them less likely to be misused. In order to
    // access the value, `dereference_*()` functions shall be called instead.
    ASTERIA_INCOMPLET(Variable)
    refcnt_ptr<Variable>
    unphase_variable_opt() const noexcept
      {
        auto st = this->m_stor.ptr<St_var>();
        if(!st)
          return nullptr;
        return unerase_pointer_cast<Variable>(st->var);
      }

    ASTERIA_INCOMPLET(PTC_Arguments)
    refcnt_ptr<PTC_Arguments>
    unphase_ptc_opt() const noexcept
      {
        auto st = this->m_stor.ptr<St_ptc>();
        if(!st)
          return nullptr;
        return unerase_pointer_cast<PTC_Arguments>(*st);
      }

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const;

    // Get the target value.
    const Value&
    dereference_readonly() const
      {
        auto st = this->m_stor.ptr<St_temp>();
        if(st && st->subs.empty())
          return st->val;
        return this->do_dereference_readonly_slow();
      }

    Value&
    dereference_mutable() const;

    Value&
    dereference_copy()
      {
        auto st = this->m_stor.mut_ptr<St_temp>();
        if(st && st->subs.empty())
          return st->val;
        return this->do_dereference_copy_slow();
      }

    Value
    dereference_unset() const;

    void
    check_function_result(Global_Context& global)
      {
        if(this->m_stor.ptr<St_ptc>())
          this->do_use_function_result_slow(global);

        if(!(this->m_stor.ptr<St_temp>() || this->m_stor.ptr<St_var>()
             || this->m_stor.ptr<St_void>()))
          this->do_throw_not_dereferenceable();
      }
  };

inline
void
swap(Reference& lhs, Reference& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
