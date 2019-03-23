// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_HPP_
#define ASTERIA_RUNTIME_VARIABLE_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"
#include "value.hpp"
#include "../syntax/source_location.hpp"

namespace Asteria {

class Variable : public virtual Rcbase
  {
  private:
    Source_Location m_sloc;
    Value m_value;
    bool m_immutable;

    // These are only used during garbage collection and are uninitialized by default.
    mutable long m_gcref_intg;
    mutable double m_gcref_mant;

  public:
    explicit Variable(const Source_Location& sloc) noexcept
      : m_sloc(sloc),
        m_value(), m_immutable(true)
      {
      }
    ~Variable() override;

    Variable(const Variable&)
      = delete;
    Variable& operator=(const Variable&)
      = delete;

  public:
    const Source_Location& get_source_location() const noexcept
      {
        return this->m_sloc;
      }
    const Value& get_value() const noexcept
      {
        return this->m_value;
      }
    Value& open_value()
      {
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
    template<typename XvalueT> void reset(const Source_Location& sloc, XvalueT&& xvalue, bool immutable)
      {
        this->m_sloc = sloc;
        this->m_value = rocket::forward<XvalueT>(xvalue);
        this->m_immutable = immutable;
      }

    void enumerate_variables(const Abstract_Variable_Callback& callback) const
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
        auto carry = static_cast<int>(this->m_gcref_mant);
        this->m_gcref_intg += carry;
        this->m_gcref_mant -= carry;
      }
  };

}  // namespace Asteria

#endif
