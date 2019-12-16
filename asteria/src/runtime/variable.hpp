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
    mutable pair<long, double> m_gcref;

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
    void set_immutable(bool immutable) noexcept
      {
        this->m_immut = immutable;
      }
    template<typename XValT> void reset(XValT&& xval, bool immutable)
      {
        this->m_value = ::rocket::forward<XValT>(xval);
        this->m_immut = immutable;
      }

    long gcref_split() const noexcept
      {
        return this->m_value.gcref_split();
      }
    long get_gcref() const noexcept
      {
        return this->m_gcref.first;
      }
    void reset_gcref(long iref) const noexcept
      {
        this->m_gcref = ::std::make_pair(iref, 1 / 0x1p26);
      }
    long increment_gcref(long split) const noexcept;

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

}  // namespace Asteria

#endif
