// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_HPP_
#define ASTERIA_RUNTIME_VARIABLE_HPP_

#include "../fwd.hpp"
#include "../rcbase.hpp"
#include "../value.hpp"

namespace Asteria {

class Variable final : public virtual Rcbase
  {
  private:
    // contents
    Value m_value;
    bool m_immut = true;
    // garbage collection support
    pair<long, double> m_gcref;

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
    template<typename XValT> Variable& reset(XValT&& xval, bool immutable)
      {
        this->m_value = ::rocket::forward<XValT>(xval);
        this->m_immut = immutable;
        return *this;
      }

    long gcref_split() const noexcept
      {
        return this->m_value.gcref_split();
      }
    long get_gcref() const noexcept
      {
        return this->m_gcref.first;
      }
    Variable& reset_gcref(long iref) noexcept
      {
        return this->m_gcref = ::std::make_pair(iref, 0x1p-26), *this;
      }
    long increment_gcref(long split) noexcept;

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

}  // namespace Asteria

#endif
