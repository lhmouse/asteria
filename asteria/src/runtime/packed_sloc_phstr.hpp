// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_PACKED_SLOC_PHSTR_HPP_
#define ASTERIA_RUNTIME_PACKED_SLOC_PHSTR_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"
#include "source_location.hpp"

namespace Asteria {

class Packed_sloc_phstr : public virtual Rcbase
  {
  private:
    Source_Location m_sloc;
    phsh_string m_phstr;

  public:
    Packed_sloc_phstr(Source_Location xsloc, phsh_string xphstr) noexcept
      : m_sloc(rocket::move(xsloc)), m_phstr(rocket::move(xphstr))
      {
      }
    ~Packed_sloc_phstr() override;

  public:
    const Source_Location& sloc() const noexcept
      {
        return this->m_sloc;
      }
    const phsh_string& phstr() const noexcept
      {
        return this->m_phstr;
      }
  };

}  // namespace Asteria

#endif
