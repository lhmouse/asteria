// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "insertable_ostream.hpp"

namespace Asteria {

Insertable_streambuf::Insertable_streambuf()
	: std::streambuf()
	, m_string(), m_caret(0)
{ }
Insertable_streambuf::~Insertable_streambuf() = default;

std::streamsize Insertable_streambuf::xsputn(const std::streambuf::char_type *s, std::streamsize n){
	const auto n_put = static_cast<std::size_t>(std::min<std::streamsize>(n, PTRDIFF_MAX));
	m_string.insert(m_caret, s, n_put);
	m_caret += n_put;
	return static_cast<std::streamsize>(n_put);
}
std::streambuf::int_type Insertable_streambuf::overflow(std::streambuf::int_type c){
	if(traits_type::eq_int_type(c, traits_type::eof())){
		return traits_type::not_eof(c);
	}
	m_string.insert(m_caret, 1, traits_type::to_char_type(c));
	m_caret += 1;
	return c;
}

Insertable_ostream::Insertable_ostream()
	: std::ostream(&m_sb)
{ }
Insertable_ostream::~Insertable_ostream() = default;

}
