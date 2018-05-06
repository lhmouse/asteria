// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INSERTABLE_OSTREAM_HPP_
#define ASTERIA_INSERTABLE_OSTREAM_HPP_

#include <string>
#include <ostream>
#include <cstddef>

namespace Asteria {

class Insertable_streambuf : public std::streambuf {
private:
	std::string m_string;
	std::size_t m_caret;

public:
	Insertable_streambuf();
	~Insertable_streambuf() override;

protected:
	std::streamsize xsputn(const std::streambuf::char_type *s, std::streamsize n) override;
	std::streambuf::int_type overflow(std::streambuf::int_type c) override;

public:
	const std::string & get_string() const noexcept {
		return m_string;
	}
	std::size_t get_caret() const noexcept {
		return m_caret;
	}
	void set_string(std::string string) noexcept {
		m_string = std::move(string);
		m_caret = m_string.size();
	}
	void set_caret(std::size_t caret) noexcept {
		m_caret = caret;
	}
	std::string extract_string() noexcept {
		std::string result;
		result.swap(m_string);
		m_caret = 0;
		return result;
	}
};

class Insertable_ostream : public std::ostream {
private:
	Insertable_streambuf m_sb;

public:
	Insertable_ostream();
	~Insertable_ostream() override;

public:
	Insertable_streambuf * rdbuf() const noexcept {
		return const_cast<Insertable_streambuf *>(&m_sb);
	}

	const std::string & get_string() const noexcept {
		return rdbuf()->get_string();
	}
	std::size_t get_caret() const noexcept {
		return rdbuf()->get_caret();
	}
	void set_string(std::string string) noexcept {
		return rdbuf()->set_string(std::move(string));
	}
	void set_caret(std::size_t caret) noexcept {
		return rdbuf()->set_caret(caret);
	}
};

}

#endif
