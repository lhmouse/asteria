// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_PACKED_SLOC_STR_HPP_
#define ASTERIA_RUNTIME_PACKED_SLOC_STR_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"
#include "source_location.hpp"

namespace Asteria {

class Packed_sloc_str : public virtual Rcbase
  {
  private:
    Source_Location m_sloc;
    cow_string m_str;

  public:
    Packed_sloc_str(Source_Location xsloc, cow_string xstr) noexcept
      : m_sloc(rocket::move(xsloc)), m_str(rocket::move(xstr))
      {
      }
    ~Packed_sloc_str() override;

  public:
    const Source_Location& sloc() const noexcept
      {
        return this->m_sloc;
      }
    const cow_string& str() const noexcept
      {
        return this->m_str;
      }
  };

}  // namespace Asteria

#endif
