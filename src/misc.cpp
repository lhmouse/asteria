// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "misc.hpp"
#include <iostream>
#include <time.h> // time_t, time(), localtime_r()

namespace Asteria {

Logger::Logger(const char *file, unsigned long line, const char *func) noexcept
	: m_file(file), m_line(line), m_func(func)
	, m_stream(std::ios_base::ate)
{
	//
}
Logger::~Logger(){
	//
}

void write_log_to_stderr(Logger &&logger) noexcept {
 	// Prepend the timestamp and write the result to stderr.
	std::ostringstream oss;
 	::time_t now;
 	::time(&now);
 	struct ::tm result;
	oss <<"[" <<std::put_time(::localtime_r(&now, &result), "%c") <<"] "
	    <<logger.get_file() <<":" <<logger.get_line() <<" ### " <<logger.get_stream().str() <<'\n';
	std::cerr <<oss.str() <<std::flush;
}
void throw_runtime_error(Logger &&logger){
	// Prepend the function name and throw an `std::runtime_error`.
	std::ostringstream oss;
	oss <<logger.get_func() <<": " <<logger.get_stream().str();
	throw std::runtime_error(oss.str());
}

}
