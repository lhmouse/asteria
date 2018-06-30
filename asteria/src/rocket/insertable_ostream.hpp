// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_INSERTABLE_OSTREAM_HPP_
#define ROCKET_INSERTABLE_OSTREAM_HPP_

#include "insertable_streambuf.hpp"
#include <ostream> // std::basic_ostream<>
#include <memory> // std::addressof()

namespace rocket {

using ::std::basic_ostream;

template<typename charT, typename traitsT = char_traits<charT>, typename allocatorT = allocator<charT>>
class basic_insertable_ostream;

template<typename charT, typename traitsT, typename allocatorT>
class basic_insertable_ostream : public basic_ostream<charT, traitsT> {
public:
	using string_type  = typename basic_insertable_streambuf<charT, traitsT>::string_type;
	using size_type    = typename basic_insertable_streambuf<charT, traitsT>::size_type;
	using traits_type  = typename basic_ostream<charT, traitsT>::traits_type;
	using char_type    = typename basic_ostream<charT, traitsT>::char_type;
	using int_type     = typename basic_ostream<charT, traitsT>::int_type;

private:
	basic_insertable_streambuf<charT, traitsT> m_sb;

public:
	explicit basic_insertable_ostream(ios_base::open_mode which = ios_base::out)
		: basic_ostream<charT, traitsT>(::std::addressof(this->m_sb))
		, m_sb(which | ios_base::out)
	{ }
	explicit basic_insertable_ostream(string_type str, ios_base::open_mode which = ios_base::out)
		: basic_ostream<charT, traitsT>(::std::addressof(this->m_sb))
		, m_sb(::std::move(str), which | ios_base::out)
	{ }
	~basic_insertable_ostream() override;

public:
	basic_insertable_streambuf<charT, traitsT> * rdbuf() const noexcept {
		return const_cast<basic_insertable_streambuf<charT, traitsT> *>(::std::addressof(this->m_sb));
	}

	const string_type & get_string() const noexcept {
		return this->rdbuf()->get_string();
	}
	size_type get_caret() const noexcept {
		return this->rdbuf()->get_caret();
	}
	void set_string(string_type str) noexcept {
		return this->rdbuf()->set_string(::std::move(str));
	}
	void set_caret(size_type caret) noexcept {
		return this->rdbuf()->set_caret(caret);
	}
	string_type extract_string() noexcept {
		return this->rdbuf()->extract_string();
	}
};

template<typename charT, typename traitsT, typename allocatorT>
basic_insertable_ostream<charT, traitsT, allocatorT>::~basic_insertable_ostream() = default;

extern template class basic_insertable_ostream<char>;
extern template class basic_insertable_ostream<wchar_t>;

using insertable_ostream  = basic_insertable_ostream<char>;
using insertable_wostream = basic_insertable_ostream<wchar_t>;

}

#endif
