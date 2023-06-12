// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "../utils.hpp"
#include <string.h>  // strerror_r
namespace asteria {
namespace details_utils {

const char ctrl_char_names[][8] =
  {
    "[NUL]", "[SOH]", "[STX]", "[ETX]", "[EOT]", "[ENQ]", "[ACK]", "[BEL]",
    "[BS]",  "\t",    "\n\t",  "[VT]",  "[FF]",  "",      "[SO]",  "[SI]",
    "[DLE]", "[DC1]", "[DC2]", "[DC3]", "[DC4]", "[NAK]", "[SYN]", "[ETB]",
    "[CAN]", "[EM]",  "[SUB]", "[ESC]", "[FS]",  "[GS]",  "[RS]",  "[US]",
  };

tinyfmt&
operator<<(tinyfmt& fmt, const Paragraph_Wrapper& q)
  {
    if(q.indent == 0) {
      // Write everything in a single line, separated by spaces.
      fmt << ' ';
    }
    else {
      // Terminate the current line.
      fmt << '\n';

      // Indent the next line accordingly.
      for(size_t i = 0;  i < q.hanging;  ++i)
        fmt << ' ';
    }
    return fmt;
  }

tinyfmt&
operator<<(tinyfmt& fmt, const Formatted_errno& e)
  {
    // Write the error number, followed by its description.
    char sbuf[256];
    const char* str = ::strerror_r(e.err, sbuf, sizeof(sbuf));
    return fmt << "error " << e.err << ": " << str;
  }

}  // namespace details_utils
}  // namespace asteria
