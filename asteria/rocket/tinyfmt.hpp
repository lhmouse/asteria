// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_HPP_
#define ROCKET_TINYFMT_HPP_

#include "utilities.hpp"
#include "throw.hpp"
#include <streambuf>  // std::basic_streambuf

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>> class basic_tinyfmt;

    namespace details_tinyfmt {

    inline const char* stringify_dir(ios_base::seekdir dir) noexcept
      {
        // Stringify the direction.
        if(dir == ios_base::beg)
          return "the beginning";
        else if(dir == ios_base::end)
          return "the end";
        else
          return "the current position";
      }

    template<typename charT, typename traitsT, size_t nmaxT> void xformat_write(basic_tinyfmt<charT, traitsT>& fmt, const char (&str)[nmaxT], long n)
      {
        // Widen and write all characters.
        charT wstr[nmaxT];
        for(long i = 0; i != n; ++i)
          wstr[i] = traitsT::to_char_type(static_cast<unsigned char>(str[i]));
        fmt.write(wstr, n);
      }
    template<typename traitsT, size_t nmaxT> inline void xformat_write(basic_tinyfmt<char, traitsT>& fmt, const char (&str)[nmaxT], long n)
      {
        // Write all characters verbatim.
        fmt.write(str, n);
      }

    // These functions assume the C locale and does not check for buffer overflows.
    // Don't play with these functions at home!
    extern long xformat_Fb(char* str, bool value) noexcept;
    extern long xformat_Zp(char* str, const void* value) noexcept;
    extern long xformat_Di(char* str, signed value) noexcept;
    extern long xformat_Du(char* str, unsigned value) noexcept;
    extern long xformat_Qi(char* str, signed long long value) noexcept;
    extern long xformat_Qu(char* str, unsigned long long value) noexcept;
    extern long xformat_Qf(char* str, double value) noexcept;

    }

template<typename charT, typename traitsT> class basic_tinyfmt
  {
  public:
    using char_type    = charT;
    using traits_type  = traitsT;

    using int_type  = typename traits_type::int_type;
    using pos_type  = typename traits_type::pos_type;
    using off_type  = typename traits_type::off_type;

    // N.B. These are non-standard extensions.
    using streambuf_type  = ::std::basic_streambuf<charT, traitsT>;
    using stream_type     = basic_tinyfmt;

  private:
    streambuf_type* m_sb;

  protected:
    // These protected functions are provided in alignment with `std::ostream`.
    basic_tinyfmt(basic_tinyfmt&& /*other*/) noexcept
      : m_sb(nullptr)
      {
      }
    basic_tinyfmt& operator=(basic_tinyfmt&& /*other*/) noexcept
      {
        return *this;
      }
    void swap(basic_tinyfmt& /*other*/) noexcept
      {
        // There is nothing to swap.
      }
    streambuf_type* set_rdbuf(streambuf_type* sb) noexcept
      {
        return ::std::exchange(this->m_sb, sb);
      }

  public:
    explicit basic_tinyfmt(streambuf_type* sb)
      : m_sb(sb)
      {
      }
    virtual ~basic_tinyfmt();

  private:
    streambuf_type& do_check_buf() const
      {
        auto sb = this->m_sb;
        if(!sb) {
          noadl::sprintf_and_throw<invalid_argument>("basic_tinyfmt: No buffer was associated with this stream.");
        }
        return *sb;
      }

  public:
    // Buffer operations
    streambuf_type* rdbuf() const noexcept
      {
        return this->m_sb;
      }
    streambuf_type* rdbuf(streambuf_type* sb) noexcept
      {
        return ::std::exchange(this->m_sb, sb);
      }

    // Stream positioning
    pos_type tellp()
      {
        auto rpos = this->do_check_buf().pubseekoff(0, ios_base::cur, ios_base::out);
        if(rpos == pos_type(off_type(-1))) {
          // Throw an exception on failure, unlike `std::ostream`.
          noadl::sprintf_and_throw<runtime_error>("basic_tinyfmt: The in-place seek operation failed.");
        }
        return rpos;
      }
    basic_tinyfmt& seekp(pos_type pos)
      {
        auto rpos = this->do_check_buf().pubseekpos(pos, ios_base::out);
        if(rpos == pos_type(off_type(-1))) {
          // Throw an exception on failure, unlike `std::ostream`.
          noadl::sprintf_and_throw<runtime_error>("basic_tinyfmt: The absolute seek operation to `%lld` characters failed.",
                                                  static_cast<long long>(pos));
        }
        return *this;
      }
    basic_tinyfmt& seekp(off_type off, ios_base::seekdir dir)
      {
        auto rpos = this->do_check_buf().pubseekoff(off, dir, ios_base::out);
        if(rpos == pos_type(off_type(-1))) {
          // Throw an exception on failure, unlike `std::ostream`.
          noadl::sprintf_and_throw<runtime_error>("basic_tinyfmt: The seek operation by `%lld` characters relative to %s failed.",
                                                  static_cast<long long>(off), details_tinyfmt::stringify_dir(dir));
        }
        return *this;
      }

    // Flush
    basic_tinyfmt& flush()
      {
        auto res = this->do_check_buf().pubsync();
        if(res == -1) {
          // Throw an exception on failure, unlike `std::ostream`.
          noadl::sprintf_and_throw<runtime_error>("basic_tinyfmt: The flush operation failed.");
        }
        return *this;
      }

    // Unformatted output functions
    basic_tinyfmt& put(char_type c)
      {
        // Write a single character.
        int_type ch = this->do_check_buf().sputc(c);
        if(traits_type::eq_int_type(ch, traits_type::eof())) {
          // Throw an exception on failure, unlike `std::ostream`.
          noadl::sprintf_and_throw<runtime_error>("basic_tinyfmt: The put operation failed.");
        }
        return *this;
      }
    basic_tinyfmt& write(const char_type* s, streamsize n)
      {
        // Write all characters.
        const char_type* pos = s;
        while(pos != s + n) {
          auto nput = this->do_check_buf().sputn(pos, s + n - pos);
          if(nput <= 0) {
            // Throw an exception on failure, unlike `std::ostream`.
            noadl::sprintf_and_throw<runtime_error>("basic_tinyfmt: The write operation failed.");
          }
          pos += nput;
        }
        return *this;
      }

    // Formatted output functions
    basic_tinyfmt& operator<<(char_type c)
      {
        // Write a single character.
        this->put(c);
        return *this;
      }
    basic_tinyfmt& operator<<(const char_type* s)
      {
        // Write a sequence until a null character is encountered.
        const char_type* pos = s;
        while(!traits_type::eq(*pos, char_type())) {
          this->put(*pos);
          ++pos;
        }
        return *this;
      }

    basic_tinyfmt& operator<<(bool value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Fb(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(const void* value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Zp(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }

    basic_tinyfmt& operator<<(signed char value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Di(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(unsigned char value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Du(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(signed short value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Di(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(unsigned short value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Du(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(signed value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Di(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(unsigned value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Du(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(signed long value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Qi(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(unsigned long value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Qu(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(signed long long value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Qi(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(unsigned long long value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Qu(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }

    basic_tinyfmt& operator<<(float value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Qf(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
    basic_tinyfmt& operator<<(double value)
      {
        // Format the value reversely.
        char str[32];
        long n = details_tinyfmt::xformat_Qf(str, value);
        details_tinyfmt::xformat_write(*this, str, n);
        return *this;
      }
  };

template<typename charT, typename traitsT> basic_tinyfmt<charT, traitsT>::~basic_tinyfmt()
  = default;

// Inserter for enumeraions
template<typename charT, typename traitsT, typename valueT,
         ROCKET_ENABLE_IF(is_enum<valueT>::value)> basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt, const valueT& value)
  {
    return fmt << static_cast<typename underlying_type<valueT>::type>(value);
  }

// Inserter for rvalues
template<typename charT, typename traitsT,
         typename valueT> basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>&& fmt, valueT&& value)
  {
    return fmt << noadl::forward<valueT>(value);
  }

extern template class basic_tinyfmt<char>;
extern template class basic_tinyfmt<wchar_t>;

using tinyfmt = basic_tinyfmt<char>;
using wtinyfmt = basic_tinyfmt<wchar_t>;

}  // namespace rocket

#endif
