// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_OPS_HPP_
#define ROCKET_COW_STRING_OPS_HPP_

#include "cow_string.hpp"
#include <istream>  // std::streamsize, std::ios_base, std::basic_istream<>
#include <ostream>  // std::basic_ostream<>

namespace rocket {

template<typename charT, typename traitsT,
         typename allocatorT> basic_istream<charT, traitsT>& operator>>(basic_istream<charT, traitsT>& is,
                                                                        basic_cow_string<charT, traitsT, allocatorT>& str)
  {
    // Initiate this FormattedInputFunction.
    typename basic_istream<charT, traitsT>::sentry sentry(is, false);
    if(!sentry) {
      return is;
    }
    // Clear the contents of `str`. The C++ standard mandates use of `.erase()` rather than `.clear()`.
    str.erase();
    // Copy stream parameters.
    auto width = is.width();
    auto sloc = is.getloc();
    // We need to set stream state bits outside the `try` block.
    auto state = ios_base::goodbit;
    try {
      // Extract characters and append them to `str`.
      auto ch = is.rdbuf()->sgetc();
      for(;;) {
        if(traitsT::eq_int_type(ch, traitsT::eof())) {
          state |= ios_base::eofbit;
          break;
        }
        auto enough = (width > 0) ? (static_cast<streamsize>(str.size()) >= width)
                                  : (str.size() >= str.max_size());
        if(enough) {
          break;
        }
        auto c = traitsT::to_char_type(ch);
        if(::std::isspace<charT>(c, sloc)) {
          break;
        }
        str.push_back(c);
        // Read the next character.
        ch = is.rdbuf()->snextc();
      }
      if(str.empty()) {
        state |= ios_base::failbit;
      }
    }
    catch(...) {
      noadl::handle_ios_exception(is, state);
    }
    if(state) {
      is.setstate(state);
    }
    is.width(0);
    return is;
  }

extern template ::std::istream& operator>>(::std::istream& is, cow_string& str);
extern template ::std::wistream& operator>>(::std::wistream& is, cow_wstring& str);

template<typename charT, typename traitsT,
         typename allocatorT> basic_ostream<charT, traitsT>& operator<<(basic_ostream<charT, traitsT>& os,
                                                                        const basic_cow_string<charT, traitsT, allocatorT>& str)
  {
    // Initiate this FormattedOutputFunction.
    typename basic_ostream<charT, traitsT>::sentry sentry(os);
    if(!sentry) {
      return os;
    }
    // Copy stream parameters.
    auto width = os.width();
    auto fill = os.fill();
    auto left = (os.flags() & ios_base::adjustfield) == ios_base::left;
    auto len = static_cast<streamsize>(str.size());
    // We need to set stream state bits outside the `try` block.
    auto state = ios_base::goodbit;
    try {
      // Insert characters into `os`, which are from `str` if `off` is within `[0, len)` and are copied from `os.fill()` otherwise.
      auto rem = noadl::max(width, len);
      auto off = left ? 0 : (len - rem);
      for(;;) {
        if(rem == 0) {
          break;
        }
        auto written = ((0 <= off) && (off < len)) ? os.rdbuf()->sputn(str.data() + off, len - off)
                                                   : !(traitsT::eq_int_type(os.rdbuf()->sputc(fill), traitsT::eof()));
        if(written == 0) {
          state |= ios_base::failbit;
          break;
        }
        rem -= written;
        off += written;
      }
    }
    catch(...) {
      noadl::handle_ios_exception(os, state);
    }
    if(state) {
      os.setstate(state);
    }
    os.width(0);
    return os;
  }

extern template ::std::ostream& operator<<(::std::ostream& os, const cow_string& str);
extern template ::std::wostream& operator<<(::std::wostream& os, const cow_wstring& str);

template<typename charT, typename traitsT,
         typename allocatorT> basic_istream<charT, traitsT>& getline(basic_istream<charT, traitsT>& is,
                                                                     basic_cow_string<charT, traitsT, allocatorT>& str, charT delim)
  {
    // Initiate this UnformattedInputFunction.
    typename basic_istream<charT, traitsT>::sentry sentry(is, true);
    if(!sentry) {
      return is;
    }
    // Clear the contents of `str`. The C++ standard mandates use of `.erase()` rather than `.clear()`.
    str.erase();
    // We need to set stream state bits outside the `try` block.
    auto state = ios_base::goodbit;
    try {
      // Extract characters and append them to `str`.
      auto ch = is.rdbuf()->sgetc();
      for(;;) {
        if(traitsT::eq_int_type(ch, traitsT::eof())) {
          state |= ios_base::eofbit;
          break;
        }
        auto c = traitsT::to_char_type(ch);
        if(traitsT::eq(c, delim)) {
          is.rdbuf()->sbumpc();
          break;
        }
        if(str.size() >= str.max_size()) {
          state |= ios_base::failbit;
          break;
        }
        str.push_back(c);
        // Read the next character.
        ch = is.rdbuf()->snextc();
      }
      if(str.empty() && (state & ios_base::eofbit)) {
        state |= ios_base::failbit;
      }
    }
    catch(...) {
      noadl::handle_ios_exception(is, state);
    }
    if(state) {
      is.setstate(state);
    }
    return is;
  }
template<typename charT, typename traitsT,
         typename allocatorT> basic_istream<charT, traitsT>& getline(basic_istream<charT, traitsT>& is,
                                                                     basic_cow_string<charT, traitsT, allocatorT>& str)
  {
    return noadl::getline(is, str, is.widen('\n'));
  }
template<typename charT, typename traitsT,
         typename allocatorT> basic_istream<charT, traitsT>& getline(basic_istream<charT, traitsT>&& is,
                                                                     basic_cow_string<charT, traitsT, allocatorT>& str, charT delim)
  {
    return noadl::getline(is, str, delim);
  }
template<typename charT, typename traitsT,
         typename allocatorT> basic_istream<charT, traitsT>& getline(basic_istream<charT, traitsT>&& is,
                                                                     basic_cow_string<charT, traitsT, allocatorT>& str)
  {
    return noadl::getline(is, str);
  }

extern template ::std::istream& getline(::std::istream& is, cow_string& str, char delim);
extern template ::std::istream& getline(::std::istream& is, cow_string& str);
extern template ::std::wistream& getline(::std::wistream& is, cow_wstring& str, wchar_t delim);
extern template ::std::wistream& getline(::std::wistream& is, cow_wstring& str);

}  // namespace rocket

#endif
