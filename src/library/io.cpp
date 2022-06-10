// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "io.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/runtime_error.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

class IOF_Sentry
  {
  private:
    ::FILE* m_fp;

  public:
    explicit
    IOF_Sentry(::FILE* fp) noexcept
      : m_fp(fp)
      { ::flockfile(this->m_fp);  }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(IOF_Sentry)
      { ::funlockfile(this->m_fp);  }

  public:
    operator
    ::FILE*() const noexcept
      { return this->m_fp;  }
  };

int
do_recover_unlocked(::FILE* fp)
  {
    // Note `errno` is meaningful only when an error has occurred.
    // EOF is not an error.
    int err = 0;
    if(ROCKET_UNEXPECT(::ferror_unlocked(fp))) {
      err = errno;

      // If the preceding operation failed for these non-fatal errors,
      // clear the error bit. This makes such operations retryable.
      if(::rocket::is_any_of(err, { EINTR, EAGAIN, EWOULDBLOCK }))
        ::clearerr_unlocked(fp);
    }

    // Return the error code as is.
    // Note that we will always throw an exception in case of errors,
    // even when it is not fatal.
    return err;
  }

constexpr
int
do_normalize_fwide(int wide) noexcept
  {
    return (wide == 0) ? 0 : ((wide >> (WORD_BIT - 1)) | 1);
  }

bool
do_set_wide(::FILE* fp, const char* mode, int wide)
  {
    // Get the current orientation.
    int wcomp = do_normalize_fwide(wide);
    if(do_normalize_fwide(::fwide(fp, wide)) != wcomp) {
      // Clear the current orientation and try resetting it.
      // XXX: Is it safe to do so when the file has been locked?
      if(!::freopen(nullptr, mode, fp))
        ::abort();

      if(do_normalize_fwide(::fwide(fp, wide)) != wcomp)
        return false;
    }
    return true;
  }

size_t
do_write_utf8_common(::FILE* fp, const cow_string& text)
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
      if(::fputwc_unlocked((wchar_t)cp, fp) == WEOF)
        ASTERIA_THROW_RUNTIME_ERROR((
            "Error writing standard output",
            "[`fputwc_unlocked()` failed: $1]"),
            format_errno());

      // The return value is the number of code points rather than bytes.
      ncps += 1;
    }
    return ncps;
  }

size_t
do_format_write_utf8_common(::FILE* fp, const V_string& templ,
                            const cow_vector<Value>& values)
  {
    // Prepare inserters.
    cow_vector<::rocket::formatter> insts;
    insts.reserve(values.size());
    for(const auto& val : values)
      insts.push_back({
        [](tinyfmt& fmt, const void* ptr) { ((const Value*) ptr)->print(fmt);  },
        &val });

    // Compose the string into a stream and write it.
    ::rocket::tinyfmt_str fmt;
    vformat(fmt, templ.data(), templ.size(), insts.data(), insts.size());
    return do_write_utf8_common(fp, fmt.get_string());
  }

}  // namespace

optV_integer
std_io_getc()
  {
    const IOF_Sentry fp(stdin);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Standard input failure (error bit set)"));

    if(!do_set_wide(fp, "r", +1))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid text read from binary-oriented input"));

    // Read a UTF code point.
    wint_t wch = ::fgetwc_unlocked(fp);
    if(wch == WEOF) {
      // Throw an exception on error.
      int err = do_recover_unlocked(fp);
      if(err != 0)
        ASTERIA_THROW_RUNTIME_ERROR((
            "Error reading standard input",
            "[`fgetwc_unlocked()` failed: $1]"),
            format_errno(err));

      // Return `null` on EOF.
      return nullopt;
    }

    // Zero-extend the code point to an integer.
    return (char32_t)wch;
  }

optV_string
std_io_getln()
  {
    const IOF_Sentry fp(stdin);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Standard input failure (error bit set)"));

    if(!do_set_wide(fp, "r", +1))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid text read from binary-oriented input"));

    // Read a UTF-8 string.
    cow_string u8str;
    ::wint_t wch;
    while((wch = ::fgetwc_unlocked(fp)) != L'\n') {
      // Check for errors.
      if(wch == WEOF) {
        // If at least a character has been read, don't fail.
        if(!u8str.empty())
          break;

        // Throw an exception on error.
        int err = do_recover_unlocked(fp);
        if(err != 0)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Error reading standard input",
              "[`fgetwc_unlocked()` failed: $1]"),
              format_errno(err));

        // Return `null` on EOF.
        return nullopt;
      }

      // Append the non-LF character to the result string.
      char32_t cp = (char32_t)wch;
      if(!utf8_encode(u8str, cp))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Invalid UTF code point from standard input (value `$1`)"),
            wch);
    }
    return u8str;
  }

optV_integer
std_io_putc(V_integer value)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Standard output failure (error bit set)"));

    if(!do_set_wide(fp, "w", +1))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid text write to binary-oriented output"));

    // Validate the code point.
    char32_t cp = (char32_t)value;
    if(cp != value)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid UTF code point (value `$1`)"), value);

    // Check whether it is valid by try encoding it.
    // The result is discarded.
    char16_t sbuf[2];
    char16_t* sp = sbuf;
    if(!utf16_encode(sp, cp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid UTF code point (value `$1`)"), value);

    // Write a UTF code point.
    if(::fputwc_unlocked((wchar_t)cp, fp) == WEOF)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error writing standard output",
          "[`fputwc_unlocked()` failed: $1]"),
          format_errno());

    // Return the number of code points that have been written.
    // This is always 1 for this function.
    return 1;
  }

optV_integer
std_io_putc(V_string value)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Standard output failure (error bit set)"));

    if(!do_set_wide(fp, "w", +1))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid text write to binary-oriented output"));

    // Write only the string.
    size_t ncps = do_write_utf8_common(fp, value);

    // Return the number of code points that have been written.
    return (int64_t)ncps;
  }

optV_integer
std_io_putln(V_string value)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Standard output failure (error bit set)"));

    if(!do_set_wide(fp, "w", +1))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid text write to binary-oriented output"));

    // Write the string itself.
    size_t ncps = do_write_utf8_common(fp, value);

    // Append a line feed and flush.
    if(::fputwc_unlocked(L'\n', fp) == WEOF)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error writing standard output",
          "[`fputwc_unlocked()` failed: $1]"),
          format_errno());

    // Return the number of code points that have been written.
    // The implicit LF also counts.
    return (int64_t)ncps + 1;
  }

optV_integer
std_io_putf(V_string templ, cow_vector<Value> values)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Standard output failure (error bit set)"));

    if(!do_set_wide(fp, "w", +1))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid text write to binary-oriented output"));

    // Write the string itself.
    size_t ncps = do_format_write_utf8_common(fp, templ, values);

    // Return the number of code points that have been written.
    return (int64_t)ncps;
  }

optV_integer
std_io_putfln(V_string templ, cow_vector<Value> values)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Standard output failure (error bit set)"));

    if(!do_set_wide(fp, "w", +1))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid text write to binary-oriented output"));

    // Write the string itself.
    size_t ncps = do_format_write_utf8_common(fp, templ, values);

    // Append a line feed and flush.
    if(::fputwc_unlocked(L'\n', fp) == WEOF)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error writing standard output",
          "[`fputwc_unlocked()` failed: $1]"),
          format_errno());

    // Return the number of code points that have been written.
    // The implicit LF also counts.
    return (int64_t)ncps + 1;
  }

optV_string
std_io_read(optV_integer limit)
  {
    const IOF_Sentry fp(stdin);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Standard input failure (error bit set)"));

    if(!do_set_wide(fp, "r", -1))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid binary read from text-oriented input"));

    V_string data;
    int64_t rlimit = limit.value_or(INT64_MAX);
    while(rlimit > 0) {
      // Try reading some bytes.
      size_t nbatch = ::rocket::clamp_cast<size_t>(rlimit, 0, 0x100000);
      auto insert_pos = data.insert(data.end(), nbatch, '/');
      size_t nread = ::fread_unlocked(&*insert_pos, 1, nbatch, fp);
      data.erase(insert_pos + (ptrdiff_t)nread, data.end());

      // Check for errors.
      if(nread == 0) {
        // If at least a byte has been read, don't fail.
        if(!data.empty())
          break;

        // Throw an exception on error.
        int err = do_recover_unlocked(fp);
        if(err != 0)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Error reading standard input",
              "[`fgetwc_unlocked()` failed: $1]"),
              format_errno(err));

        // Return `null` on EOF.
        return nullopt;
      }

      rlimit -= (int64_t)nread;
    }
    return ::std::move(data);
  }

optV_integer
std_io_write(V_string data)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Standard output failure (error bit set)"));

    if(!do_set_wide(fp, "w", -1))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid binary write to text-oriented output"));

    size_t off = 0;
    while(off < data.size()) {
      // Try writing some bytes.
      size_t nwrtn = ::fwrite_unlocked(data.data() + off, 1, data.size() - off, fp);
      if(nwrtn == 0) {
        // Throw an exception on error.
        int err = do_recover_unlocked(fp);
        if(err != 0)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Error writing standard output",
              "[`fwrite_unlocked()` failed: $1]"),
              format_errno(err));

        // If nothing has been written, fail.
        return nullopt;
      }

      off += nwrtn;
    }
    return (int64_t)off;
  }

void
std_io_flush()
  {
    // Flush standard output only.
    if(::fflush(stdout) == EOF)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Error flushing standard output",
          "[`fflush()` failed: $1]"),
          format_errno());
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
