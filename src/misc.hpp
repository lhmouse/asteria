// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_MISC_HPP_
#define ASTERIA_MISC_HPP_

#include <string>
#include <ostream>
#include <cstddef>

namespace Asteria {

class Logger_streambuf : public std::streambuf {
private:
	std::string m_str;
	std::size_t m_ins = 0;

public:
	Logger_streambuf() noexcept;
	~Logger_streambuf();

protected:
	std::streamsize xsputn(const std::streambuf::char_type *s, std::streamsize n) override;
	std::streambuf::int_type overflow(std::streambuf::int_type c) override;

public:
	const std::string &get_str() const noexcept {
		return m_str;
	}
	std::size_t get_ins() const noexcept {
		return m_ins;
	}
	void set_string(std::string str) noexcept {
		m_str = std::move(str);
		m_ins = m_str.size();
	}
	void set_ins(std::size_t ins) noexcept {
		m_ins = ins;
	}
};

class Logger : public std::ostream {
private:
	const char *const m_file;
	const unsigned long m_line;
	const char *const m_func;

	Logger_streambuf m_buf;

public:
	Logger(const char *file, unsigned long line, const char *func) noexcept;
	~Logger();

	Logger(const Logger &) = delete;
	Logger &operator=(const Logger &) = delete;

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

	const Logger_streambuf &get_buf() const noexcept {
		return m_buf;
	}
	Logger_streambuf &get_buf() noexcept {
		return m_buf;
	}

	template<typename ValueT>
	Logger &operator,(const ValueT &value) noexcept
	try {
		*this <<value;
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
