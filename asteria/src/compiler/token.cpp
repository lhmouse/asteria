// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "token.hpp"
#include "../utilities.hpp"

namespace Asteria {

std::ostream& Token::print(std::ostream& ostrm) const
  {
    switch(this->index()) {
    case index_keyword:
      {
        // keyword `if`
        return ostrm << "keyword `" << stringify_keyword(this->m_stor.as<index_keyword>().kwrd) << "`";
      }
    case index_punctuator:
      {
        // punctuator `;`
        return ostrm << "punctuator `" << stringify_punctuator(this->m_stor.as<index_punctuator>().punct) << "`";
      }
    case index_identifier:
      {
        // identifier `meow`
        return ostrm << "identifier `" << this->m_stor.as<index_identifier>().name << "`";
      }
    case index_integer_literal:
      {
        // integer-literal `42`
        return ostrm << "integer-literal `" << std::dec << this->m_stor.as<index_integer_literal>().val << "`";
      }
    case index_real_literal:
      {
        // real-literal `123.456`
        return ostrm << "real-literal `" << std::defaultfloat << std::nouppercase << std::setprecision(17) << this->m_stor.as<index_real_literal>().val << "`";
      }
    case index_string_literal:
      {
        // string-literal "hello world"
        return ostrm << "string-literal `" << quote(this->m_stor.as<index_string_literal>().val) << "`";
      }
    default:
      ASTERIA_TERMINATE("An unknown token type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
