// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

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
        fmt << "keyword `";
        fmt << stringify_keyword(this->m_stor.as<S_keyword>().kwrd);
        return fmt << "`";

      case index_punctuator:
        // punctuator `;`
        fmt << "punctuator `";
        fmt << stringify_punctuator(this->m_stor.as<S_punctuator>().punct);
        return fmt << "`";

      case index_identifier:
        // identifier `meow`
        fmt << "identifier `";
        fmt << this->m_stor.as<S_identifier>().name;
        return fmt << "`";

      case index_integer_literal:
        // integer-literal `42`
        fmt << "integer-literal `";
        fmt << this->m_stor.as<S_integer_literal>().val;
        return fmt << "`";

      case index_real_literal:
        // real-literal `123.456`
        fmt << "real-literal `";
        fmt << this->m_stor.as<S_real_literal>().val;
        return fmt << "`";

      case index_string_literal: {
        // string-literal "hello world"
        fmt << "string-literal \"";
        const auto& val = this->m_stor.as<S_string_literal>().val;
        c_quote(fmt, val.data(), val.size());
        return fmt << "\"";
      }

      default:
        ASTERIA_TERMINATE(("Invalid token type `$1`"), this->index());
    }
  }

}  // namespace asteria
