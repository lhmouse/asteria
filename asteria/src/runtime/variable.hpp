// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_HPP_
#define ASTERIA_RUNTIME_VARIABLE_HPP_

#include "../fwd.hpp"
#include "refcnt_base.hpp"
#include "value.hpp"

namespace Asteria {

class Variable : public virtual RefCnt_Base
  {
  private:
    Value m_value;
    bool m_immutable;

    // These are only used during garbage collection and are uninitialized by default.
    mutable long m_gcref_intg;
    mutable double m_gcref_mant;

  public:
    Variable() noexcept
      : m_value(), m_immutable(true)
      {
      }

    Variable(const Variable &)
      = delete;
    Variable & operator=(const Variable &)
      = delete;

  private:
    [[noreturn]] void do_throw_immutable() const;

  public:
    const Value & get_value() const noexcept
      {
        return this->m_value;
      }
    Value & open_value()
      {
        if(this->m_immutable) {
          this->do_throw_immutable();
        }
        return this->m_value;
      }
    bool is_immutable() const noexcept
      {
        return this->m_immutable;
      }
    void set_immutable(bool immutable) noexcept
      {
        this->m_immutable = immutable;
      }
    template<typename XvalueT> void reset(XvalueT &&value, bool immutable)
      {
        this->m_value = std::forward<XvalueT>(value);
        this->m_immutable = immutable;
      }

    void enumerate_variables(const Abstract_Variable_Callback &callback) const
      {
        this->m_value.enumerate_variables(callback);
      }

    long get_gcref() const noexcept
      {
        return this->m_gcref_intg;
      }
    void init_gcref(long intg) const noexcept
      {
        this->m_gcref_intg = intg;
        this->m_gcref_mant = 1e-9;
      }
    void add_gcref(int dintg) const noexcept
      {
        this->m_gcref_intg += dintg;
      }
    void add_gcref(double dmant) const noexcept
      {
        this->m_gcref_mant += dmant;
        // Add with carry.
        const auto carry = static_cast<long>(this->m_gcref_mant);
        this->m_gcref_intg += carry;
        this->m_gcref_mant -= static_cast<double>(carry);
      }
  };

}

#endif
