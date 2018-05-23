// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "cow_string.hpp"
#include <stdexcept>
#include <cstdarg>
#include <cstdio>

#define VSNPRINTF(buf_, fmt_)	\
	do {	\
	  ::std::va_list ap_;	\
	  va_start(ap_, fmt_);	\
	  ::std::vsnprintf((buf_).data(), (buf_).size(), (fmt_), ap_);	\
	  va_end(ap_);	\
	} while(false)

namespace rocket {

namespace details_cow_string {
	void throw_invalid_argument(const char *fmt, ...){
		::std::array<char, 1024> str;
		VSNPRINTF(str, fmt);
		throw ::std::invalid_argument(str.data());
	}
	void throw_out_of_range(const char *fmt, ...){
		::std::array<char, 1024> str;
		VSNPRINTF(str, fmt);
		throw ::std::out_of_range(str.data());
	}
	void throw_length_error(const char *fmt, ...){
		::std::array<char, 1024> str;
		VSNPRINTF(str, fmt);
		throw ::std::length_error(str.data());
	}

	template void handle_io_exception(::std::ios  &);
	template void handle_io_exception(::std::wios &);

	template class storage_handle<char>;
	template class storage_handle<wchar_t>;
	template class storage_handle<char16_t>;
	template class storage_handle<char32_t>;

	template class shallow_base<char>;
	template class shallow_base<wchar_t>;
	template class shallow_base<char16_t>;
	template class shallow_base<char32_t>;
}

template class basic_cow_string<char>;
template class basic_cow_string<wchar_t>;
template class basic_cow_string<char16_t>;
template class basic_cow_string<char32_t>;

template ::std::istream  & operator>>(::std::istream  &is, cow_string  &str);
template ::std::wistream & operator>>(::std::wistream &is, cow_wstring &str);

template ::std::ostream  & operator<<(::std::ostream  &os, const cow_string  &str);
template ::std::wostream & operator<<(::std::wostream &os, const cow_wstring &str);

}
