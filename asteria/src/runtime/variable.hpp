// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_HPP_
#define ASTERIA_RUNTIME_VARIABLE_HPP_

#include "../fwd.hpp"
#include "../rcbase.hpp"
#include "../value.hpp"

namespace Asteria {

class Variable final : public virtual Rcbase
  {
  private:
    Value m_value;
    bool m_immut = false;
    bool m_alive = false;

    // These are reference counters for garbage collection and are uninitialized by default.
    // As values are reference-counting, reference counts can be fractional. For example,
    // if three variablesshare a single instance of a function, then each of them is supposed
    // to have 1/3 of the object.
    long m_gcref_i;
    double m_gcref_f;

  public:
    Variable() noexcept
      {
      }
    ~Variable() override;

    Variable(const Variable&)
      = delete;
    Variable& operator=(const Variable&)
      = delete;

  public:
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
        return this->m_immut;
      }
    Variable& set_immutable(bool immutable) noexcept
      {
        return this->m_immut = immutable, *this;
      }

    bool is_initialized() const noexcept
      {
        return this->m_alive;
      }
    template<typename XValT> Variable& initialize(XValT&& xval, bool immut)
      {
        this->m_value = ::rocket::forward<XValT>(xval);
        this->m_immut = immut;
        this->m_alive = true;
        return *this;
      }
    Variable& uninitialize() noexcept
      {
        this->m_value = INT64_C(0x6eef8badf00ddead);
        this->m_immut = true;
        this->m_alive = false;
        return *this;
      }

    long gcref_split() const noexcept
      {
        return this->m_value.gcref_split();
      }
    long get_gcref() const noexcept
      {
        return this->m_gcref_i;
      }
    Variable& reset_gcref(long iref) noexcept
      {
        this->m_gcref_i = iref;
        this->m_gcref_f = 0x1p-26;
        return *this;
      }
    Variable& increment_gcref(long split) noexcept
      {
        // Optimize for the non-split case.
        if(split > 1) {
          // Update the fractional part.
          this->m_gcref_f += 1 / static_cast<double>(split);
          // Check and accumulate the carry bit.
          if(static_cast<long>(this->m_gcref_f) == 0)
            return *this;
          this->m_gcref_f -= 1;
        }
        this->m_gcref_i += 1;
        return *this;
      }

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

}  // namespace Asteria

#endif
