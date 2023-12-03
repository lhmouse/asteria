// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "runtime_error.hpp"
#include "enums.hpp"
namespace asteria {
namespace {

static_assert(::std::is_nothrow_copy_constructible<Runtime_Error>::value);
static_assert(::std::is_nothrow_move_constructible<Runtime_Error>::value);
static_assert(::std::is_nothrow_copy_assignable<Runtime_Error>::value);
static_assert(::std::is_nothrow_move_assignable<Runtime_Error>::value);

}  // namespace

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
    this->m_frames.insert(this->m_frame_ins, ::std::move(xfrm));
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
      this->m_fmt << "\n  " << sbuf.data() << ") " << describe_frame_type(r.type)
                  << " at '" << r.sloc << "': ";

      // Write the value in this frame.
      this->m_tempf.clear_string();
      r.value.print(this->m_tempf);
      const auto& vstr = this->m_tempf.get_string();

      if(vstr.size() > 100) {
        // Trim the message.
        constexpr size_t trunc_to = 80;
        this->m_fmt.putn(vstr.data(), trunc_to);
        this->m_fmt << " ... (" << (vstr.size() - trunc_to) << " characters omitted)";
      }
      else
        this->m_fmt.putn(vstr.data(), vstr.size());
    }
    this->m_fmt << "\n  -- end of backtrace frames]";
  }

}  // namespace asteria
