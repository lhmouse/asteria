// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_INSERTABLE_STREAMBUF_HPP_
#define ROCKET_INSERTABLE_STREAMBUF_HPP_

#include <string> // std::basic_string<>, std::char_traits<>, std::allocator<>
#include <ostream> // std::basic_streambuf<>, std::basic_ostream<>, std::streamsize
#include <utility> // std::move()

namespace rocket {

using ::std::basic_string;
using ::std::char_traits;
using ::std::allocator;
using ::std::basic_streambuf;
using ::std::basic_ostream;
using ::std::streamsize;

template<typename charT, typename traitsT = char_traits<charT>, typename allocatorT = allocator<charT>>
class basic_insertable_streambuf : public basic_streambuf<charT, traitsT> {
public:
	using string_type  = basic_string<charT, traitsT, allocatorT>;
	using size_type    = typename string_type::size_type;
	using value_type   = typename string_type::value_type;
	using traits_type  = typename string_type::traits_type;
	using char_type    = typename traits_type::char_type;
	using int_type     = typename traits_type::int_type;

private:
	basic_string<charT, traitsT, allocatorT> m_str;
	size_type m_caret;

public:
	basic_insertable_streambuf() noexcept
		: m_str(), m_caret(string_type::npos)
	{ }
	~basic_insertable_streambuf() override;

protected:
	streamsize xsputn(const char_type *s, streamsize n) override;
	int_type overflow(int_type c) override;

public:
	const string_type & get_string() const noexcept {
		return this->m_str;
	}
	size_type get_caret() const noexcept {
		return this->m_caret;
	}
	void set_string(string_type str, size_type caret = string_type::npos) noexcept(noexcept(m_str = ::std::move(m_str))) {
		this->m_str = ::std::move(str);
		this->m_caret = caret;
	}
	void set_caret(size_type caret) noexcept {
		this->m_caret = caret;
	}
	string_type extract_string() noexcept(noexcept(m_str.swap(m_str))) {
		string_type str;
		this->m_str.swap(str);
		this->m_caret = string_type::npos;
		return str;
	}
};

template<typename charT, typename traitsT, typename allocatorT>
basic_insertable_streambuf<charT, traitsT, allocatorT>::~basic_insertable_streambuf() = default;

template<typename charT, typename traitsT, typename allocatorT>
streamsize basic_insertable_streambuf<charT, traitsT, allocatorT>::xsputn(const char_type *s, streamsize n){
	const auto n_put = (static_cast<unsigned long long>(n) <= this->m_str.max_size()) ? static_cast<size_type>(n) : this->m_str.max_size();
	if(this->m_caret == string_type::npos){
		this->m_str.insert(this->m_str.size(), s, n_put);
	} else {
		this->m_str.insert(this->m_caret, s, n_put);
		this->m_caret += n_put;
	}
	return static_cast<streamsize>(n_put);
}
template<typename charT, typename traitsT, typename allocatorT>
typename basic_insertable_streambuf<charT, traitsT, allocatorT>::int_type basic_insertable_streambuf<charT, traitsT, allocatorT>::overflow(int_type c){
	if(traits_type::eq_int_type(c, traits_type::eof())){
		return traits_type::not_eof(c);
	}
	if(this->m_caret == string_type::npos){
		this->m_str.push_back(traits_type::to_char_type(c));
	} else {
		this->m_str.insert(this->m_caret, static_cast<size_type>(1), traits_type::to_char_type(c));
		this->m_caret += 1;
	}
	return c;
}

extern template class basic_insertable_streambuf<char>;
extern template class basic_insertable_streambuf<wchar_t>;

using insertable_streambuf  = basic_insertable_streambuf<char>;
using insertable_wstreambuf = basic_insertable_streambuf<wchar_t>;

}

#endif
