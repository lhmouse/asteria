// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_
#define ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_

#include "../fwd.hpp"
#include "../syntax/block.hpp"

namespace Asteria {

class Simple_source_file
  {
  private:
    rocket::cow_string m_file;
    Block m_code;

  public:
    Simple_source_file(std::istream &cstrm_io, const rocket::cow_string &file);
    ROCKET_COPYABLE_DESTRUCTOR(Simple_source_file);

  public:
    const rocket::cow_string & get_file() const noexcept
      {
        return this->m_file;
      }

    Reference execute(Global_context &global, rocket::cow_vector<Reference> &&args) const;
  };

}

#endif
