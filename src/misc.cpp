// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "misc.hpp"
#include <iostream> // std::cerr
#include <time.h> // time_t, time(), asctime_r()

namespace Asteria {

Logger_streambuf::Logger_streambuf() noexcept
	: std::streambuf()
{
	try {
		m_str.reserve(1023);
	} catch(...){
		// Allocation failure doesn't matter here.
	}
}
Logger_streambuf::~Logger_streambuf(){
	//
}

std::streamsize Logger_streambuf::xsputn(const std::streambuf::char_type *s, std::streamsize n){
	const auto n_put = static_cast<std::size_t>(std::min<std::streamsize>(n, PTRDIFF_MAX));
	m_str.insert(m_ins, s, n_put);
	m_ins += n_put;
	return static_cast<std::streamsize>(n_put);
}
std::streambuf::int_type Logger_streambuf::overflow(std::streambuf::int_type c){
	if(traits_type::eq_int_type(c, traits_type::eof())){
		return traits_type::not_eof(c);
	}
	m_str.insert(m_ins, 1, traits_type::to_char_type(c));
	m_ins += 1;
	return c;
}

Logger::Logger(const char *file, unsigned long line, const char *func) noexcept
	: std::ostream(&m_buf)
	, m_file(file), m_line(line), m_func(func)
{
	//
}
Logger::~Logger(){
	//
}

void write_log_to_stderr(Logger &&logger) noexcept
try {
	// Prepend the timestamp and write the result to stderr.
	::time_t now;
	::time(&now);
	char time_str[26];
	::ctime_r(&now, time_str);
	time_str[24] = 0;
	auto &buf = logger.get_buf();
	logger <<'\n';
	buf.set_ins(0);
	logger <<"[" <<time_str <<"] " <<logger.get_file() <<":" <<logger.get_line() <<" ##: ";
	std::cerr <<buf.get_str() <<std::flush;
} catch(...){
	// Swallow any exceptions thrown.
}
void throw_runtime_error(Logger &&logger){
	// Prepend the function name and throw an `std::runtime_error`.
	auto &buf = logger.get_buf();
	buf.set_ins(0);
	logger <<logger.get_func() <<": ";
	throw std::runtime_error(buf.get_str());
}

}
