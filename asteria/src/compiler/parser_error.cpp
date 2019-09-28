// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "parser_error.hpp"
#include "../utilities.hpp"

namespace Asteria {

Parser_Error::~Parser_Error()
  {
  }

void Parser_Error::do_compose_message()
  {
    tinyfmt_str fmt;
    // Write the status code in digital form.
    fmt << "error " << this->m_status << " at ";
    // Append the source location.
    if(this->m_line > 0) {
      fmt << "line " << this->m_line << ", offset " << this->m_offset << ", length " << this->m_length;
    }
    else {
      fmt << "the end of input data";
    }
    // Append the status description.
    fmt << ": " << describe_parser_status(this->m_status);
    // Set the string.
    this->m_what = fmt.extract_string();
  }

static_assert(std::is_nothrow_copy_constructible<Parser_Error>::value, "Copy constructors of exceptions are not allow to throw exceptions.");
static_assert(std::is_nothrow_move_constructible<Parser_Error>::value, "Move constructors of exceptions are not allow to throw exceptions.");
static_assert(std::is_nothrow_copy_assignable<Parser_Error>::value, "Copy assignment operators of exceptions are not allow to throw exceptions.");
static_assert(std::is_nothrow_move_assignable<Parser_Error>::value, "Move assignment operators of exceptions are not allow to throw exceptions.");

}  // namespace Asteria
