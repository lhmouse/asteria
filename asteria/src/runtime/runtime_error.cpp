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
do_backtrace(Backtrace_Frame&& new_frm)
  {
    // Unpack nested exceptions, if any.
    try {
      if(auto eptr = ::std::current_exception())
        ::std::rethrow_exception(eptr);
    }
    catch(Runtime_Error& nested)
      { this->m_frames.append(nested.m_frames.begin(), nested.m_frames.end());  }
    catch(exception&)
      { }

    // Push a new frame.
    this->do_insert_frame(::std::move(new_frm));
  }

void
Runtime_Error::
do_insert_frame(Backtrace_Frame&& new_frm)
  {
    // Insert the frame. Note exception safety.
    auto ipos = this->m_frames.begin() + this->m_ins_at;
    ipos = this->m_frames.insert(ipos, ::std::move(new_frm));
    this->m_ins_at = ipos + 1 - this->m_frames.begin();

    // Rebuild the message using new frames.
    // The storage may be reused.
    ::rocket::tinyfmt_str fmt;
    fmt.set_string(::std::move(this->m_what));
    fmt.clear_string();

    // Write the value. Strings are written as is. ALl other values are prettified.
    fmt << "asteria runtime error: ";
    if(this->m_value.is_string())
      fmt << this->m_value.as_string();
    else
      fmt << this->m_value;

    // Get the width of the frame number colomn.
    ::rocket::ascii_numput nump;
    nump.put(this->m_frames.size() - 1);

    sso_vector<char, 24> sbuf(nump.size(), ' ');
    sbuf.emplace_back();

    // Append stack frames.
    fmt << "\n[backtrace frames:";
    for(size_t k = 0;  k < this->m_frames.size();  ++k) {
      const auto& frm = this->m_frames[k];
      nump.put(k);
      ::std::reverse_copy(nump.begin(), nump.end(), sbuf.mut_rbegin() + 1);
      format(fmt, "\n  $1: $2 at '$3': ", sbuf.data(), frm.what_type(), frm.sloc());
      frm.value().dump(fmt, 0, 0);
    }
    fmt << "\n  -- end of backtrace frames]";

    // Set the string.
    this->m_what = fmt.extract_string();
  }

}  // namespace asteria
