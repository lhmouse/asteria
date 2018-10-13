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
    String m_filename;
    Block m_code;

  public:
    Simple_source_file(std::istream &cstrm_io, String filename);
    ~Simple_source_file();

  public:
    const String & get_filename() const noexcept;
    Reference execute(Global_context &global, Vector<Reference> args) const;
  };

}

#endif
