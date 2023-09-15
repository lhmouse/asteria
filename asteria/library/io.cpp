// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "io.hpp"
#include "../argument_reader.hpp"
#include "../binding_generator.hpp"
#include "../runtime/runtime_error.hpp"
#include "../utils.hpp"
namespace asteria {
namespace {

enum IOF_Mode : uint8_t
  {
    iof_mode_input_narrow    = 0b00,
    iof_mode_input_wide      = 0b01,
    iof_mode_output_narrow   = 0b10,
    iof_mode_output_wide     = 0b11,
  };

class IOF_Sentry
  {
  private:
    ::FILE* m_fp;

  public:
    explicit
    IOF_Sentry(::FILE* sentry, IOF_Mode mode)
      : m_fp(sentry)
      {
        int orient_req = (mode & 0b01) ? +1 : -1;
        char mode_req[4] = "?b";
        mode_req[0] = (mode & 0b10) ? 'w' : 'r';

        ::flockfile(this->m_fp);

        if((::fwide(this->m_fp, orient_req) ^ orient_req) < 0) {
          // Clear the current orientation.
          // XXX: Is it safe to do so when the file has been locked?
          if(::freopen(nullptr, mode_req, this->m_fp) == nullptr)
            ASTERIA_THROW((
                "Could not reopen standard stream (mode `$1`)",
                "[`freopen` failed: ${errno:full}]"),
                mode_req);

          // Retry now. This really should not fail.
          if((::fwide(this->m_fp, orient_req) ^ orient_req) < 0)
            ASTERIA_THROW((
                "Could not set standard stream orientation (mode `$2`)"),
                mode_req);
        }

        // Check for earlier errors.
        if(::ferror_unlocked(sentry))
          ASTERIA_THROW_RUNTIME_ERROR((
              "Standard stream error (mode `$2`, error bit set)"),
              mode_req);
      }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(IOF_Sentry)
      {
        ::funlockfile(this->m_fp);
      }

    operator
    ::FILE*() const noexcept
      { return this->m_fp;  }
  };

size_t
do_write_utf8_common(const IOF_Sentry& sentry, stringR text)
  {
    size_t ncps = 0;
    size_t off = 0;

    while(off < text.size()) {
      // Decode a code point from `text`.
      char32_t cp;
      if(!utf8_decode(cp, text, off))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Invalid UTF-8 string (text `$1`, byte offset `$2`)"),
            text, off);

      // Insert it into the output stream.
      if(::fputwc_unlocked((wchar_t)cp, sentry) == WEOF)
        ASTERIA_THROW_RUNTIME_ERROR((
            "Error writing standard output",
            "[`fputwc_unlocked()` failed: ${errno:full}]"));

      // The return value is the number of code points rather than bytes.
      ncps += 1;
    }
    return ncps;
  }

size_t
do_format_write_utf8_common(const IOF_Sentry& sentry, const V_string& templ, const cow_vector<Value>& values)
  {
    // Prepare inserters.
    cow_vector<::rocket::formatter> insts;
    insts.reserve(values.size());

    for(const auto& val : values)
      insts.push_back({
        [](tinyfmt& fmt, const void* ptr) {
          const Value& arg = *(const Value*) ptr;
          if(arg.is_string())
            fmt << arg.as_string();
          else
            arg.print(fmt);
        },
        &val });

    // Compose the string into a stream and write it.
    ::rocket::tinyfmt_str fmt;
    vformat(fmt, templ.safe_c_str(), insts.data(), insts.size());
    return do_write_utf8_common(sentry, fmt.get_string());
  }

}  // namespace

optV_integer
std_io_getc()
  {
    wint_t wch;
    const IOF_Sentry sentry(stdin, iof_mode_input_wide);

    wch = ::fgetwc_unlocked(sentry);
    if((wch == WEOF) && ::ferror_unlocked(sentry))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error reading standard input",
          "[`fgetwc_unlocked()` failed: ${errno:full}]"));

    if(wch == WEOF)
      return nullopt;

    return (int64_t)(uint32_t) wch;
  }

optV_string
std_io_getln()
  {
    cow_string u8str;
    wint_t wch;
    const IOF_Sentry sentry(stdin, iof_mode_input_wide);

    for(;;) {
      wch = ::fgetwc_unlocked(sentry);
      if((wch == WEOF) && ::ferror_unlocked(sentry))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Error reading standard input",
            "[`fgetwc_unlocked()` failed: ${errno:full}]"));

      if((wch == L'\n') || (wch == WEOF))
        break;

      if(!utf8_encode(u8str, (char32_t) wch))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Invalid UTF code point from standard input (value `$1`)"),
            wch);
    }

    if((wch == WEOF) && u8str.empty())
      return nullopt;

    return ::std::move(u8str);
  }

optV_integer
std_io_putc(V_integer value)
  {
    char u8discard[4];
    char* u8ptr = u8discard;
    const IOF_Sentry sentry(stdout, iof_mode_output_wide);

    if((value < 0) || (value > 0x10FFFF))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid UTF code point (value `$1`)"),
          value);

    if(!utf8_encode(u8ptr, (char32_t) value))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid UTF code point (value `$1`)"),
          value);

    if(::fputwc_unlocked((wchar_t) value, sentry) == WEOF)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error writing standard output",
          "[`fputwc_unlocked()` failed: ${errno:full}]"));

    return 1;
  }

V_integer
std_io_putc(V_string value)
  {
    size_t ncps;
    const IOF_Sentry sentry(stdout, iof_mode_output_wide);

    ncps = do_write_utf8_common(sentry, value);
    return (int64_t) ncps;
  }

V_integer
std_io_putln(V_string value)
  {
    size_t ncps;
    const IOF_Sentry sentry(stdout, iof_mode_output_wide);

    ncps = do_write_utf8_common(sentry, value);

    if(::fputwc_unlocked(L'\n', sentry) == WEOF)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error writing standard output",
          "[`fputwc_unlocked()` failed: ${errno:full}]"));

    return (int64_t) ncps + 1;
  }

V_integer
std_io_putf(V_string templ, cow_vector<Value> values)
  {
    size_t ncps;
    const IOF_Sentry sentry(stdout, iof_mode_output_wide);

    ncps = do_format_write_utf8_common(sentry, templ, values);
    return (int64_t) ncps;
  }

V_integer
std_io_putfln(V_string templ, cow_vector<Value> values)
  {
    size_t ncps;
    const IOF_Sentry sentry(stdout, iof_mode_output_wide);

    ncps = do_format_write_utf8_common(sentry, templ, values);

    if(::fputwc_unlocked(L'\n', sentry) == WEOF)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error writing standard output",
          "[`fputwc_unlocked()` failed: ${errno:full}]"));

    return (int64_t) ncps;
  }

optV_string
std_io_read(optV_integer limit)
  {
    const size_t ntotal = ::rocket::clamp_cast<size_t>(limit.value_or(INT_MAX), 0, INT_MAX);
    size_t nbatch, nread;
    V_string data;
    const IOF_Sentry sentry(stdin, iof_mode_input_narrow);

    for(;;) {
      ROCKET_ASSERT(data.size() <= ntotal);
      nbatch = ::std::min(ntotal - data.size(), data.size() / 2 + 127);
      if(nbatch == 0)
        break;

      auto batch_pos = data.insert(data.end(), nbatch, '*');
      nread = ::fread_unlocked(&*batch_pos, 1, nbatch, sentry);
      if((nread != nbatch) && ::ferror_unlocked(sentry))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Error reading standard input",
            "[`fgetwc_unlocked()` failed: ${errno:full}]"));

      if(nread != nbatch) {
        data.erase(batch_pos + (ptrdiff_t) nread, data.end());
        break;
      }
    }

    if(((nbatch != 0) || ::feof_unlocked(sentry)) && data.empty())
      return nullopt;

    return ::std::move(data);
  }

V_integer
std_io_write(V_string data)
  {
    const IOF_Sentry sentry(stdout, iof_mode_output_narrow);

    if(::fwrite_unlocked(data.data(), 1, data.size(), sentry) != data.size())
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error writing standard output",
          "[`fwrite_unlocked()` failed: ${errno:full}]"));

    return (int64_t) data.size();
  }

void
std_io_flush()
  {
    if(::fflush(nullptr) == EOF)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error flushing standard streams",
          "[`fflush()` failed: ${errno:full}]"));
  }

void
create_bindings_io(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("getc"),
      ASTERIA_BINDING(
        "std.io.getc", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_io_getc();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("getln"),
      ASTERIA_BINDING(
        "std.io.getln", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_io_getln();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("putc"),
      ASTERIA_BINDING(
        "std.io.putc", "value",
        Argument_Reader&& reader)
      {
        V_integer ch;
        V_string str;

        reader.start_overload();
        reader.required(ch);
        if(reader.end_overload())
          return (Value) std_io_putc(ch);

        reader.start_overload();
        reader.required(str);
        if(reader.end_overload())
          return (Value) std_io_putc(str);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("putln"),
      ASTERIA_BINDING(
        "std.io.putln", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_io_putln(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("putf"),
      ASTERIA_BINDING(
        "std.io.putf", "templ, ...",
        Argument_Reader&& reader)
      {
        V_string templ;
        cow_vector<Value> values;

        reader.start_overload();
        reader.required(templ);
        if(reader.end_overload(values))
          return (Value) std_io_putf(templ, values);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("putfln"),
      ASTERIA_BINDING(
        "std.io.putfln", "templ, ...",
        Argument_Reader&& reader)
      {
        V_string templ;
        cow_vector<Value> values;

        reader.start_overload();
        reader.required(templ);
        if(reader.end_overload(values))
          return (Value) std_io_putfln(templ, values);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("read"),
      ASTERIA_BINDING(
        "std.io.read", "[limit]",
        Argument_Reader&& reader)
      {
        optV_integer limit;

        reader.start_overload();
        reader.optional(limit);
        if(reader.end_overload())
          return (Value) std_io_read(limit);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("write"),
      ASTERIA_BINDING(
        "std.io.write", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_io_write(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("flush"),
      ASTERIA_BINDING(
        "std.io.flush", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (void) std_io_flush();

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
