// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "throw.hpp"
#include <array>
#include <stdexcept>
#include <cstdarg>
#include <cstdio>

namespace rocket {

#define VSNPRINTF(buf_, fmt_)	\
	do {	\
	  ::std::va_list ap_;	\
	  va_start(ap_, fmt_);	\
	  ::std::vsnprintf((buf_).data(), (buf_).size(), (fmt_), ap_);	\
	  va_end(ap_);	\
	} while(false)

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

void rethrow_current_exception(){
	throw;
}

}
