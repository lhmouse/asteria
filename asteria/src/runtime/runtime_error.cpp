// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "runtime_error.hpp"

namespace Asteria {

Runtime_Error::~Runtime_Error()
  {
  }

void Runtime_Error::do_backtrace()
  try {
    // Unpack nested exceptions, if any.
    auto eptr = ::std::current_exception();
    if(ROCKET_EXPECT(!eptr))
      return;
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

void Runtime_Error::do_compose_message()
  {
    // Reuse the string.
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
    for(unsigned long i = 0;  i < this->m_frames.size();  ++i) {
      const auto& frm = this->m_frames[i];
      format(fmt, "\n  #$1 $2 at '$3': $4", i, frm.what_type(), frm.sloc(), frm.value());
    }

    // Set the string.
    this->m_what = fmt.extract_string();
  }

static_assert(::rocket::conjunction<::std::is_nothrow_copy_constructible<Runtime_Error>,
                                    ::std::is_nothrow_move_constructible<Runtime_Error>,
                                    ::std::is_nothrow_copy_assignable<Runtime_Error>,
                                    ::std::is_nothrow_move_assignable<Runtime_Error>>::value, "");

}  // namespace Asteria
