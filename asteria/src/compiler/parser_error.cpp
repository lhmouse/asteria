// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "parser_error.hpp"
#include "../utilities.hpp"

namespace Asteria {

std::ostream& Parser_Error::print(std::ostream& ostrm) const
  {
    // Write the status code in digital form.
    ostrm << std::dec << "error " << this->m_status << " at ";
    // Append the source location.
    if(this->m_line <= 0) {
      ostrm << "the end of input data";
    }
    else {
      ostrm << "line " << this->m_line << ", offset " << this->m_offset << ", length " << this->m_length;
    }
    // Append the status description.
    ostrm << ": " << describe_parser_status(this->m_status);
    return ostrm;
  }

void Parser_Error::convert_to_runtime_error_and_throw() const
  {
    cow_osstream fmtss;
    fmtss.imbue(std::locale::classic());
    this->print(fmtss);
    throw_runtime_error(__func__, fmtss.extract_string());
  }

}  // namespace Asteria
