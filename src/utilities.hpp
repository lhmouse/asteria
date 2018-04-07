// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#define ASTERIA_UTILITIES_HPP_

#include "insertable_ostream.hpp"

namespace Asteria {

class Logger {
private:
	const char *const m_file;
	const unsigned long m_line;
	const char *const m_func;

	Insertable_ostream m_stream;

public:
	Logger(const char *file, unsigned long line, const char *func) noexcept;
	~Logger();

private:
	template<typename ValueT>
	void do_put(const ValueT &value){
		m_stream <<value;
	}
	void do_put(bool value);
	void do_put(char value);
	void do_put(signed char value);
	void do_put(unsigned char value);
	void do_put(short value);
	void do_put(unsigned short value);
	void do_put(int value);
	void do_put(unsigned value);
	void do_put(long value);
	void do_put(unsigned long value);
	void do_put(long long value);
	void do_put(unsigned long long value);
	void do_put(const char *value);
	void do_put(const signed char *value);
	void do_put(const unsigned char *value);
	void do_put(const void *value);

public:
	const char *get_file() const noexcept {
		return m_file;
	}
	unsigned long get_line() const noexcept {
		return m_line;
	}
	const char *get_func() const noexcept {
		return m_func;
	}

	const Insertable_ostream &get_stream() const noexcept {
		return m_stream;
	}
	Insertable_ostream &get_stream() noexcept {
		return m_stream;
	}

public:
	template<typename ValueT>
	Logger &operator,(const ValueT &value) noexcept
	try {
		do_put(value);
		return *this;
	} catch(...){
		return *this;
	}
};

extern bool write_log_to_stderr(Logger &&logger) noexcept;
__attribute__((__noreturn__)) extern bool throw_runtime_error(Logger &&logger);

}

#ifdef ENABLE_DEBUG_LOGS
#  define ASTERIA_DEBUG_LOG(...)    (::Asteria::write_log_to_stderr(::std::move((::Asteria::Logger(__FILE__, __LINE__, __PRETTY_FUNCTION__), __VA_ARGS__))))
#else
#  define ASTERIA_DEBUG_LOG(...)    (static_cast<void>(0), true)
#endif

#define ASTERIA_THROW_RUNTIME_ERROR(...)      (::Asteria::throw_runtime_error(::std::move((::Asteria::Logger(__FILE__, __LINE__, __PRETTY_FUNCTION__), __VA_ARGS__))))
#define ASTERIA_VERIFY(cond_, ...)            ((cond_) ? true : ASTERIA_THROW_RUNTIME_ERROR(__VA_ARGS__))

#endif
