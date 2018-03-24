// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_MISC_HPP_
#define ASTERIA_MISC_HPP_

#include <sstream>
#include <cstddef>

namespace Asteria {

class Logger {
private:
	const char *const m_file;
	const unsigned long m_line;
	const char *const m_func;

	std::ostringstream m_stream;

public:
	Logger(const char *file, unsigned long line, const char *func) noexcept;
	~Logger();

	Logger(const Logger &) = delete;
	Logger &operator=(const Logger &) = delete;

public:
	const char *get_file() const {
		return m_file;
	}
	unsigned long get_line() const {
		return m_line;
	}
	const char *get_func() const {
		return m_func;
	}

	const std::ostringstream &get_stream() const {
		return m_stream;
	}
	std::ostringstream &get_stream(){
		return m_stream;
	}

	template<typename ValueT>
	Logger &operator,(const ValueT &value) noexcept
	try {
		m_stream <<value;
		return *this;
	} catch(...){
		return *this;
	}
};

extern void write_log_to_stderr(Logger &&logger) noexcept;
__attribute__((__noreturn__)) extern void throw_runtime_error(Logger &&logger);

}

#ifdef ENABLE_DEBUG_LOGS
#  define ASTERIA_DEBUG_LOG(...)    (::Asteria::write_log_to_stderr(::std::move((::Asteria::Logger(__FILE__, __LINE__, __PRETTY_FUNCTION__), __VA_ARGS__))))
#else
#  define ASTERIA_DEBUG_LOG(...)    ((void)0)
#endif

#define ASTERIA_THROW(...)          (::Asteria::throw_runtime_error(::std::move((::Asteria::Logger(__FILE__, __LINE__, __PRETTY_FUNCTION__), __VA_ARGS__))))

#endif
