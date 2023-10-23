// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_
#define ASTERIA_RUNTIME_REFERENCE_

#include "../fwd.hpp"
#include "../value.hpp"
#include "reference_modifier.hpp"
namespace asteria {

class Reference
  {
  private:
    Value m_value;
    rcfwd_ptr<Variable> m_var;
    rcfwd_ptr<PTC_Arguments> m_ptc;
    cow_vector<Reference_Modifier> m_mods;
    Xref m_xref = xref_invalid;

  public:
    // Constructors and assignment operators
    constexpr
    Reference() noexcept
      { }

    Reference(const Reference& other) noexcept
      :
        m_var(other.m_var),
        m_ptc(other.m_ptc),
        m_mods(other.m_mods),
        m_xref(other.m_xref)
      {
        if(this->m_xref == xref_temporary)
          this->m_value = other.m_value;
      }

    Reference&
    operator=(const Reference& other) & noexcept
      {
        if(other.m_xref == xref_temporary)
          this->m_value = other.m_value;
        else if(other.m_xref == xref_variable)
          this->m_var = other.m_var;
        else if(other.m_xref == xref_ptc)
          this->m_ptc = other.m_ptc;

        this->m_mods = other.m_mods;
        this->m_xref = other.m_xref;
        return *this;
      }

    Reference(Reference&& other) noexcept
      :
        m_var(::std::move(other.m_var)),
        m_ptc(::std::move(other.m_ptc)),
        m_mods(::std::move(other.m_mods)),
        m_xref(::rocket::exchange(other.m_xref))
      {
        if(this->m_xref == xref_temporary)
          this->m_value.swap(other.m_value);
      }

    Reference&
    operator=(Reference&& other) & noexcept
      {
        if(other.m_xref == xref_temporary)
          this->m_value.swap(other.m_value);
        else if(other.m_xref == xref_variable)
          this->m_var.swap(other.m_var);
        else if(other.m_xref == xref_ptc)
          this->m_ptc.swap(other.m_ptc);

        this->m_mods.swap(other.m_mods);
        this->m_xref = ::rocket::exchange(other.m_xref);
        return *this;
      }

    Reference&
    swap(Reference& other) noexcept
      {
        this->m_value.swap(other.m_value);
        this->m_var.swap(other.m_var);
        this->m_ptc.swap(other.m_ptc);
        this->m_mods.swap(other.m_mods);
        ::std::swap(this->m_xref, other.m_xref);
        return *this;
      }

  private:
    [[noreturn]]
    void
    do_throw_not_dereferenceable() const;

    const Value&
    do_dereference_readonly_slow() const;

    void
    do_use_function_result_slow(Global_Context& global);

  public:
    // Accessors
    bool
    is_invalid() const noexcept
      { return this->m_xref == xref_invalid;  }

    Reference&
    clear() noexcept
      {
        this->m_value = nullopt;
        this->m_var.reset();
        this->m_ptc.reset();
        this->m_mods.clear();
        this->m_xref = xref_invalid;
        return *this;
      }

    bool
    is_void() const noexcept
      { return this->m_xref == xref_void;  }

    Reference&
    set_void() noexcept
      {
        this->m_xref = xref_void;
        return *this;
      }

    bool
    is_temporary() const noexcept
      { return this->m_xref == xref_temporary;  }

    template<typename XValT,
    ROCKET_ENABLE_IF(::std::is_assignable<Value&, XValT&&>::value)>
    Reference&
    set_temporary(XValT&& xval)
      {
        this->m_value = ::std::forward<XValT>(xval);
        this->m_mods.clear();
        this->m_xref = xref_temporary;
        return *this;
      }

    bool
    is_variable() const noexcept
      { return this->m_xref == xref_variable;  }

    ASTERIA_INCOMPLET(Variable)
    Reference&
    set_variable(const refcnt_ptr<Variable>& var)
      {
        ROCKET_ASSERT(var != nullptr);
        this->m_var = var;
        this->m_mods.clear();
        this->m_xref = xref_variable;
        return *this;
      }

    bool
    is_ptc() const noexcept
      { return this->m_xref == xref_ptc;  }

    ASTERIA_INCOMPLET(PTC_Arguments)
    Reference&
    set_ptc(const refcnt_ptr<PTC_Arguments>& ptc)
      {
        ROCKET_ASSERT(ptc != nullptr);
        this->m_ptc = ptc;
        this->m_mods.clear();
        this->m_xref = xref_ptc;
        return *this;
      }

    size_t
    count_modifiers() const noexcept
      { return this->m_mods.size();  }

    void
    clear_modifiers() noexcept
      { this->m_mods.clear();  }

    template<typename XModT,
    ROCKET_ENABLE_IF(::std::is_constructible<Reference_Modifier, XModT&&>::value)>
    Reference&
    push_modifier(XModT&& xmod)
      {
        if((this->m_xref != xref_temporary) && (this->m_xref != xref_variable))
          this->do_throw_not_dereferenceable();

        this->m_mods.emplace_back(::std::forward<XModT>(xmod));
        return *this;
      }

    Reference&
    pop_modifier(size_t count = 1) noexcept
      {
        if((this->m_xref != xref_temporary) && (this->m_xref != xref_variable))
          this->do_throw_not_dereferenceable();

        if(count <= this->m_mods.size()) {
          this->m_mods.pop_back(count);
          return *this;
        }

        this->m_value = nullopt;
        this->m_mods.clear();
        this->m_xref = xref_temporary;
        return *this;
      }

    // These functions are used internally by the runtime, whose names have been
    // obscured to make them less likely to be misused. In order to access the
    // value, `dereference_*()` functions shall be called instead.
    ASTERIA_INCOMPLET(Variable)
    refcnt_ptr<Variable>
    unphase_variable_opt() const noexcept
      { return unerase_pointer_cast<Variable>(this->m_var);  }

    ASTERIA_INCOMPLET(PTC_Arguments)
    refcnt_ptr<PTC_Arguments>
    unphase_ptc_opt() const noexcept
      { return unerase_pointer_cast<PTC_Arguments>(this->m_ptc);  }

    void
    check_function_result(Global_Context& global)
      {
        if(this->m_xref == xref_ptc)
          this->do_use_function_result_slow(global);

        if((this->m_xref != xref_void) && (this->m_xref != xref_temporary) && (this->m_xref != xref_variable))
          this->do_throw_not_dereferenceable();
      }

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const;

    // Get the target value.
    const Value&
    dereference_readonly() const
      {
        if(ROCKET_EXPECT(this->m_xref == xref_temporary) && ROCKET_EXPECT(this->m_mods.empty()))
          return this->m_value;

        return this->do_dereference_readonly_slow();
      }

    Value&
    dereference_mutable() const;

    Value&
    dereference_copy()
      {
        if(ROCKET_EXPECT(this->m_xref == xref_temporary) && ROCKET_EXPECT(this->m_mods.empty()))
          return this->m_value;

        if(this->m_xref == xref_temporary) {
          // Ensure the value is copied before being moved, so we never assign
          // an element into its own container.
          auto val = this->do_dereference_readonly_slow();
          this->m_value.swap(val);
          this->m_mods.clear();
          return this->m_value;
        }

        this->m_value = this->do_dereference_readonly_slow();
        this->m_mods.clear();
        this->m_xref = xref_temporary;
        return this->m_value;
      }

    Value
    dereference_unset() const;
  };

inline
void
swap(Reference& lhs, Reference& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace asteria
#endif
