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
    // garbage collection support
    mutable pair<long, double> m_gcref;

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
    template<typename XvalueT> void reset(XvalueT&& xvalue, bool immutable)
      {
        this->m_value = rocket::forward<XvalueT>(xvalue);
        this->m_immutable = immutable;
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
        this->m_gcref = std::make_pair(iref, 1 / 0x1p26);
      }
    long increment_gcref(long split) const noexcept;

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

}  // namespace Asteria

#endif
