// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "token.hpp"
#include "enums.hpp"
#include "../utils.hpp"
namespace asteria {

tinyfmt&
Token::
print(tinyfmt& fmt) const
  {
    switch(this->index()) {
      case index_keyword:
        // keyword `if`
        return fmt << "keyword `"
                   << stringify_keyword(this->m_stor.as<index_keyword>().kwrd)
                   << "`";

      case index_punctuator:
        // punctuator `;`
        return fmt << "punctuator `"
                   << stringify_punctuator(this->m_stor.as<index_punctuator>().punct)
                   << "`";

      case index_identifier:
        // identifier `meow`
        return fmt << "identifier `"
                   << this->m_stor.as<index_identifier>().name
                   << "`";

      case index_integer_literal:
        // integer-literal `42`
        return fmt << "integer-literal `"
                   << this->m_stor.as<index_integer_literal>().val
                   << "`";

      case index_real_literal:
        // real-literal `123.456`
        return fmt << "real-literal `"
                   << this->m_stor.as<index_real_literal>().val
                   << "`";

      case index_string_literal:
        // string-literal "hello world"
        return fmt << "string-literal `"
                   << quote(this->m_stor.as<index_string_literal>().val)
                   << "`";

      default:
        ASTERIA_TERMINATE((
            "Invalid token type (index `$1`)"),
            this->index());
    }
  }

}  // namespace asteria
