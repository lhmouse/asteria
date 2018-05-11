// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_INSERTABLE_OSTREAM_HPP_
#define ROCKET_INSERTABLE_OSTREAM_HPP_

#include <string> // std::string
#include <ostream> // std::streambuf, std::ostream, std::streamsize
#include <utility> // std::move()
#include <cstddef> // std::size_t

namespace rocket {

using ::std::string;
using ::std::streambuf;
using ::std::ostream;
using ::std::streamsize;
using ::std::size_t;

class insertable_streambuf : public streambuf {
private:
	string m_str;
	size_t m_caret;

public:
	insertable_streambuf() noexcept
		: streambuf()
		, m_str(), m_caret()
	{ }
	~insertable_streambuf() override;

protected:
	streamsize xsputn(const streambuf::char_type *s, streamsize n) override;
	streambuf::int_type overflow(streambuf::int_type c) override;

public:
	const string & get_string() const noexcept {
		return this->m_str;
	}
	size_t get_caret() const noexcept {
		return this->m_caret;
	}
	void set_string(string str) noexcept {
		this->m_str = ::std::move(str);
		this->m_caret = this->m_str.size();
	}
	void set_caret(size_t caret) noexcept {
		this->m_caret = caret;
	}
	string extract_string() noexcept {
		string result;
		result.swap(this->m_str);
		this->m_caret = 0;
		return result;
	}
};

class insertable_ostream : public ostream {
private:
	insertable_streambuf m_sb[1];

public:
	insertable_ostream() noexcept
		: ostream(this->m_sb)
	{ }
	~insertable_ostream() override;

public:
	insertable_streambuf * rdbuf() const noexcept {
		return const_cast<insertable_streambuf *>(this->m_sb);
	}

	const string & get_string() const noexcept {
		return this->rdbuf()->get_string();
	}
	size_t get_caret() const noexcept {
		return this->rdbuf()->get_caret();
	}
	void set_string(string str) noexcept {
		return this->rdbuf()->set_string(::std::move(str));
	}
	void set_caret(size_t caret) noexcept {
		return this->rdbuf()->set_caret(caret);
	}
	string extract_string() noexcept {
		return this->rdbuf()->extract_string();
	}
};

}

#endif
