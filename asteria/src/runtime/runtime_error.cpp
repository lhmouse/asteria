// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "runtime_error.hpp"

namespace asteria {

static_assert(::std::is_nothrow_copy_constructible<Runtime_Error>::value &&
              ::std::is_nothrow_move_constructible<Runtime_Error>::value &&
              ::std::is_nothrow_copy_assignable<Runtime_Error>::value &&
              ::std::is_nothrow_move_assignable<Runtime_Error>::value, "");

Runtime_Error::
~Runtime_Error()
  {
  }

void
Runtime_Error::
do_backtrace()
  try {
    // Unpack nested exceptions, if any.
    if(auto eptr = ::std::current_exception())
      ::std::rethrow_exception(eptr);
  }
  catch(Runtime_Error& nested) {
    // Copy frames.
    if(this->m_frames.empty())
      this->m_frames = nested.m_frames;
    else
      this->m_frames.append(nested.m_frames.begin(), nested.m_frames.end());
  }
  catch(::std::exception& /*nested*/) {
    // Do nothing.
  }

void
Runtime_Error::
do_compose_message()
  {
    // Reuse the storage if any.
    ::rocket::tinyfmt_str fmt;
    fmt.set_string(::std::move(this->m_what));
    fmt.clear_string();

    // Write the value. Strings are written as is. ALl other values are prettified.
    fmt << "asteria runtime error: ";
    if(this->m_value.is_string())
      fmt << this->m_value.as_string();
    else
      fmt << this->m_value;

    // Append stack frames.
    fmt << "\n[backtrace frames:";
    for(size_t i = 0;  i < this->m_frames.size();  ++i) {
      const auto& frm = this->m_frames[i];
      format(fmt, "\n  #$1 $2 at '$3': ", i, frm.what_type(), frm.sloc());
      frm.value().dump(fmt);
    }
    fmt << "\n  -- end of backtrace frames]";

    // Set the string.
    this->m_what = fmt.extract_string();
  }

}  // namespace asteria
