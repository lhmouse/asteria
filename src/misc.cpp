// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "misc.hpp"
#include <iostream> // std::cerr
#include <time.h> // time_t, time(), asctime_r()

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

void write_log_to_stderr(Logger &&logger) noexcept
try {
	// Prepend the timestamp and write the result to stderr.
	std::ostringstream oss;
	::time_t now;
	::time(&now);
	char time_str[64];
	::ctime_r(&now, time_str);
	auto pos = std::strchr(time_str, '\n');
	if(pos){
		*pos = 0;
	}
	oss <<"[" <<time_str <<"] " <<logger.get_file() <<":" <<logger.get_line() <<" ### " <<logger.get_stream().str() <<'\n';
	std::cerr <<oss.str() <<std::flush;
} catch(...){
	// Swallow any exceptions thrown.
}
void throw_runtime_error(Logger &&logger){
	// Prepend the function name and throw an `std::runtime_error`.
	std::ostringstream oss;
	oss <<logger.get_func() <<": " <<logger.get_stream().str();
	throw std::runtime_error(oss.str());
}

}
