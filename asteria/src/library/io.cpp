// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "io.hpp"
#include "../runtime/argument_reader.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

class IOF_Sentry
  {
  private:
    ::FILE* m_fp;

  public:
    explicit
    IOF_Sentry(::FILE* fp)
      noexcept
      : m_fp(fp)
      { ::flockfile(this->m_fp);  }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(IOF_Sentry)
      { ::funlockfile(this->m_fp);  }

  public:
    operator
    ::FILE*()
      const noexcept
      { return this->m_fp;  }
  };

int
do_recover(::FILE* fp)
  {
    // Note `errno` is meaningful only when an error has occurred. EOF is not an error.
    int err = 0;
    if(ROCKET_UNEXPECT(::ferror_unlocked(fp))) {
      // If the preceding operation failed for these non-fatal errors, clear the error bit.
      // This makes such operations retryable.
      // But note that we always throw an exception despite such recovery.
      err = errno;
      if(::rocket::is_any_of(err, { EINTR, EAGAIN, EWOULDBLOCK }))
        ::clearerr_unlocked(fp);
    }
    return err;
  }

constexpr
int
do_normalize_fwide(int wide)
  noexcept
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
        ASTERIA_THROW("Invalid UTF-8 string (text `$1`, byte offset `$2`)", text, off);

      // Insert it into the output stream.
      if(::fputwc_unlocked(static_cast<wchar_t>(cp), fp) == WEOF)
        ASTERIA_THROW("Error writing standard output\n"
                      "[`fputwc_unlocked()` failed: $1]",
                      format_errno(errno));

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
    for(size_t i = 0;  i < values.size();  ++i)
      insts.push_back({
        [](tinyfmt& fmt, const void* ptr) -> tinyfmt&
          { return static_cast<const Value*>(ptr)->print(fmt);  },
        values.data() + i
      });

    // Compose the string into a stream and write it.
    ::rocket::tinyfmt_str fmt;
    vformat(fmt, templ.data(), templ.size(), insts.data(), insts.size());
    return do_write_utf8_common(fp, fmt.get_string());
  }

}  // namespace

Opt_integer
std_io_getc()
  {
    const IOF_Sentry fp(stdin);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW("Standard input failure (error bit set)");

    if(!do_set_wide(fp, "r", +1))
      ASTERIA_THROW("Invalid text read from binary-oriented input");

    // Read a UTF code point.
    wint_t wch = ::fgetwc_unlocked(fp);
    if(wch == WEOF) {
      // Throw an exception on error.
      int err = do_recover(fp);
      if(err != 0)
        ASTERIA_THROW("Error reading standard input\n"
                      "[`fgetwc_unlocked()` failed: $1]",
                      format_errno(err));

      // Return `null` on EOF.
      return nullopt;
    }

    // Zero-extend the code point to an integer.
    return static_cast<uint32_t>(wch);
  }

Opt_string
std_io_getln()
  {
    const IOF_Sentry fp(stdin);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW("Standard input failure (error bit set)");

    if(!do_set_wide(fp, "r", +1))
      ASTERIA_THROW("Invalid text read from binary-oriented input");

    // Read a UTF-8 string.
    cow_string u8str;
    for(;;) {
      wint_t wch = ::fgetwc_unlocked(fp);
      if(wch == WEOF) {
        // If at least a character has been read, don't fail.
        if(!u8str.empty())
          break;

        // Throw an exception on error.
        int err = do_recover(fp);
        if(err != 0)
          ASTERIA_THROW("Error reading standard input\n"
                        "[`fgetwc_unlocked()` failed: $1]",
                        format_errno(err));

        // Return `null` on EOF.
        return nullopt;
      }
      // If a LF is encountered, finish this line.
      if(wch == L'\n')
        break;

      // Append the non-LF character to the result string.
      char32_t cp = static_cast<uint32_t>(wch);
      if(!utf8_encode(u8str, cp))
        ASTERIA_THROW("Invalid UTF code point from standard input (value `$1`)", wch);
    }
    // Return the UTF-8 string.
    return u8str;
  }

Opt_integer
std_io_putc(V_integer value)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW("Standard output failure (error bit set)");

    if(!do_set_wide(fp, "w", +1))
      ASTERIA_THROW("Invalid text write to binary-oriented output");

    // Validate the code point.
    char32_t cp = static_cast<uint32_t>(value);
    if(cp != value)
      ASTERIA_THROW("Invalid UTF code point (value `$1`)", value);

    // Check whether it is valid by try encoding it.
    // The result is discarded.
    char16_t sbuf[2];
    char16_t* sp = sbuf;
    if(!utf16_encode(sp, cp))
      ASTERIA_THROW("Invalid UTF code point (value `$1`)", value);

    // Write a UTF code point.
    if(::fputwc_unlocked(static_cast<wchar_t>(cp), fp) == WEOF)
      ASTERIA_THROW("Error writing standard output\n"
                    "[`fputwc_unlocked()` failed: $1]",
                    format_errno(errno));

    // Return the number of code points that have been written.
    // This is always 1 for this function.
    return 1;
  }

Opt_integer
std_io_putc(V_string value)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW("Standard output failure (error bit set)");

    if(!do_set_wide(fp, "w", +1))
      ASTERIA_THROW("Invalid text write to binary-oriented output");

    // Write only the string.
    size_t ncps = do_write_utf8_common(fp, value);

    // Return the number of code points that have been written.
    return static_cast<int64_t>(ncps);
  }

Opt_integer
std_io_putln(V_string value)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW("Standard output failure (error bit set)");

    if(!do_set_wide(fp, "w", +1))
      ASTERIA_THROW("Invalid text write to binary-oriented output");

    // Write the string itself.
    size_t ncps = do_write_utf8_common(fp, value);

    // Append a line feed and flush.
    if(::fputwc_unlocked(L'\n', fp) == WEOF)
      ASTERIA_THROW("Error writing standard output\n"
                    "[`fputwc_unlocked()` failed: $1]",
                    format_errno(errno));

    // Return the number of code points that have been written.
    // The implicit LF also counts.
    return static_cast<int64_t>(ncps + 1);
  }

Opt_integer
std_io_putf(V_string templ, cow_vector<Value> values)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW("Standard output failure (error bit set)");

    if(!do_set_wide(fp, "w", +1))
      ASTERIA_THROW("Invalid text write to binary-oriented output");

    // Write the string itself.
    size_t ncps = do_format_write_utf8_common(fp, templ, values);

    // Return the number of code points that have been written.
    return static_cast<int64_t>(ncps);
  }

Opt_string
std_io_read(Opt_integer limit)
  {
    const IOF_Sentry fp(stdin);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW("Standard input failure (error bit set)");

    if(!do_set_wide(fp, "r", -1))
      ASTERIA_THROW("Invalid binary read from text-oriented input");

    V_string data;
    int64_t rlimit = limit.value_or(INT64_MAX);
    for(;;) {
      if(rlimit <= 0)
        break;

      size_t nbatch = static_cast<size_t>(::rocket::min(rlimit, 0x100000));
      auto insert_pos = data.insert(data.end(), nbatch, '/');

      // Read some bytes.
      size_t nread = ::fread_unlocked(&*insert_pos, 1, nbatch, fp);
      if(nread == 0) {
        int err = do_recover(fp);
        if(err != 0)
          ASTERIA_THROW("Error reading standard input\n"
                        "[`fread_unlocked()` failed: $1]",
                        format_errno(err));

        if(data.size())
          break;

        // If nothing has been read, fail.
        return nullopt;
      }
      data.erase(insert_pos + static_cast<ptrdiff_t>(nread), data.end());
      rlimit -= static_cast<int64_t>(nread);
    }
    return ::std::move(data);
  }

Opt_integer
std_io_write(V_string data)
  {
    const IOF_Sentry fp(stdout);

    // Check stream status.
    if(::ferror_unlocked(fp))
      ASTERIA_THROW("Standard output failure (error bit set)");

    if(!do_set_wide(fp, "w", -1))
      ASTERIA_THROW("Invalid binary write to text-oriented output");

    size_t ntotal = 0;
    while(ntotal < data.size()) {
      // Write some bytes.
      size_t nwrtn = ::fwrite_unlocked(data.data() + ntotal,
                                  1, data.size() - ntotal, fp);
      if(nwrtn == 0) {
        int err = do_recover(fp);
        if(err != 0)
          ASTERIA_THROW("Error writing standard output\n"
                        "[`fwrite_unlocked()` failed: $1]",
                        format_errno(err));

        // If nothing has been written, fail.
        return nullopt;
      }
      ntotal += nwrtn;
    }
    return static_cast<int64_t>(ntotal);
  }

void
std_io_flush()
  {
    // Flush standard output only.
    if(::fflush(stdout) == EOF)
      ASTERIA_THROW("Error flushing standard output\n"
                    "[`fflush()` failed: $1]",
                    format_errno(errno));
  }

void
create_bindings_io(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("getc"),
      ASTERIA_BINDING_BEGIN("std.io.getc", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_io_getc);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("getln"),
      ASTERIA_BINDING_BEGIN("std.io.getln", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_io_getln);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("putc"),
      ASTERIA_BINDING_BEGIN("std.io.putc", self, global, reader) {
        V_integer ch;
        V_string str;

        reader.start_overload();
        reader.required(ch);       // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_io_putc, ch);

        reader.start_overload();
        reader.required(str);      // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_io_putc, str);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("putln"),
      ASTERIA_BINDING_BEGIN("std.io.putln", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);      // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_io_putln, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("putf"),
      ASTERIA_BINDING_BEGIN("std.io.putf", self, global, reader) {
        V_string templ;
        cow_vector<Value> values;

        reader.start_overload();
        reader.required(templ);          // template
        if(reader.end_overload(values))  // ...
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_io_putf, templ, values);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("read"),
      ASTERIA_BINDING_BEGIN("std.io.read", self, global, reader) {
        Opt_integer limit;

        reader.start_overload();
        reader.optional(limit);     // [limit]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_io_read, limit);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("write"),
      ASTERIA_BINDING_BEGIN("std.io.write", self, global, reader) {
        V_string data;

        reader.start_overload();
        reader.required(data);      // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_io_write, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("flush"),
      ASTERIA_BINDING_BEGIN("std.io.flush", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_io_flush);
      }
      ASTERIA_BINDING_END);
  }

}  // namespace asteria
