// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "runtime_error.hpp"

namespace Asteria {

Runtime_Error::~Runtime_Error()
  {
  }

void Runtime_Error::do_compose_message()
  {
    // Reuse the string.
    tinyfmt_str fmt;
    this->m_what.clear();
    fmt.set_string(rocket::move(this->m_what));
    // Write the value.
    fmt << "asteria runtime error: " << this->m_value;
    // Append stack frames.
    for(unsigned long i = 0; i != this->m_frames.size(); ++i) {
      const auto& frm = this->m_frames[i];
      // Write a frame.
      fmt << "\n[frame #" << i << " at " << frm.sloc() << " (" << frm.what_type() << "): "
          << frm.value() << "]";
    }
    // Set the string.
    this->m_what = fmt.extract_string();
  }

static_assert(rocket::conjunction<std::is_nothrow_copy_constructible<Runtime_Error>,
                                  std::is_nothrow_move_constructible<Runtime_Error>,
                                  std::is_nothrow_copy_assignable<Runtime_Error>,
                                  std::is_nothrow_move_assignable<Runtime_Error>>::value, "");

}  // namespace Asteria
