// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "runtime_error.hpp"
#include "enums.hpp"
namespace asteria {

Runtime_Error::
~Runtime_Error()
  {
  }

void
Runtime_Error::
do_backtrace()
  {
    try {
      if(auto eptr = ::std::current_exception())
        ::std::rethrow_exception(eptr);
    }
    catch(Runtime_Error& nested)
    { this->m_frames.append(nested.m_frames.begin(), nested.m_frames.end());  }
    catch(...) { }  // ignore
  }

void
Runtime_Error::
do_insert_frame(Frame_Type type, const Source_Location* sloc_opt, const Value& val)
  {
    Frame xfrm;
    xfrm.type = type;
    if(sloc_opt)
      xfrm.sloc = *sloc_opt;
    xfrm.value = val;
    this->m_frames.insert(this->m_frame_ins, move(xfrm));
    this->m_frame_ins ++;

    // Rebuild the message using new frames. The storage may be reused.
    // Strings are written verbatim. All the others are formatted.
    this->m_fmt.clear_string();
    this->m_fmt << "runtime error: ";

    if(this->m_value.is_string())
      this->m_fmt << this->m_value.as_string();
    else
      this->m_fmt << this->m_value;

    // Get the width of the frame number column.
    ::rocket::ascii_numput nump;
    nump.put_DU(this->m_frames.size());
    static_vector<char, 24> sbuf(nump.size(), ' ');
    sbuf.emplace_back();

    // Append stack frames.
    this->m_fmt << "\n[backtrace frames:";
    for(size_t k = 0;  k < this->m_frames.size();  ++k) {
      const auto& r = this->m_frames.at(k);

      // Write frame information.
      nump.put_DU(k + 1);
      ::std::copy_backward(nump.begin(), nump.end(), sbuf.mut_end() - 1);
      const char* ftype = describe_frame_type(r.type);
      format(this->m_fmt, "\n  $1) $2 at '$3': ", sbuf.data(), ftype, r.sloc);

      // Write the value in this frame.
      this->m_tempf.clear_string();
      this->m_tempf << r.value;

      if(this->m_tempf.size() > 80) {
        // Truncate the message.
        this->m_fmt.putn(this->m_tempf.data(), 60);
        format(this->m_fmt, " ... ($1 characters omitted)", this->m_tempf.size() - 60);
      }
      else
        this->m_fmt.putn(this->m_tempf.data(), this->m_tempf.size());
    }
    this->m_fmt << "\n  -- end of backtrace frames]";
  }

}  // namespace asteria
