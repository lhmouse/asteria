// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SIMPLE_SOURCE_FILE_HPP_
#define ASTERIA_SIMPLE_SOURCE_FILE_HPP_

#include "fwd.hpp"
#include "block.hpp"
#include "reference.hpp"

namespace Asteria {

class Simple_source_file
  {
  private:
    String m_file;
    Block m_code;

  public:
    Simple_source_file(std::istream &cstrm_io, const String &file);
    ~Simple_source_file();

  public:
    const String & get_file() const noexcept
      {
        return this->m_file;
      }

    Reference execute(Global_context &global, Vector<Reference> args) const;
  };

}

#endif
