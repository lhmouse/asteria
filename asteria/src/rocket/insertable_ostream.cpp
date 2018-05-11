// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "insertable_ostream.hpp"
#include <climits>

namespace rocket {

insertable_streambuf::~insertable_streambuf() = default;

streamsize insertable_streambuf::xsputn(const streambuf::char_type *s, streamsize n){
	const auto n_put = static_cast<size_t>(::std::min<streamsize>(n, PTRDIFF_MAX));
	m_str.insert(m_caret, s, n_put);
	m_caret += n_put;
	return static_cast<streamsize>(n_put);
}
streambuf::int_type insertable_streambuf::overflow(streambuf::int_type c){
	if(traits_type::eq_int_type(c, traits_type::eof())){
		return traits_type::not_eof(c);
	}
	m_str.insert(m_caret, 1, traits_type::to_char_type(c));
	m_caret += 1;
	return c;
}

insertable_ostream::~insertable_ostream() = default;

}
