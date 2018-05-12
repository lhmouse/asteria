// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_HPP_
#define ROCKET_COW_STRING_HPP_

#include <string> // std::char_traits<>, std::allocator<>
#include <ostream> // std::basic_ostream<>, std::ostream, std::wostream

namespace rocket {

using ::std::char_traits;
using ::std::allocator;
using ::std::basic_ostream;
using ::std::ostream;
using ::std::wostream;

template<typename charT, typename traitsT = char_traits<charT>, typename allocatorT = allocator<charT>>
class basic_cow_string {
};

extern template class basic_cow_string<char>;
extern template class basic_cow_string<wchar_t>;
extern template class basic_cow_string<char16_t>;
extern template class basic_cow_string<char32_t>;

using cow_string     = basic_cow_string<char>;
using cow_wstring    = basic_cow_string<wchar_t>;
using cow_u16string  = basic_cow_string<char16_t>;
using cow_u32string  = basic_cow_string<char32_t>;

template<typename charT, typename traitsT, typename allocatorT>
basic_ostream<charT, traitsT> & operator<<(basic_ostream<charT, traitsT> &os, const basic_cow_string<charT, traitsT, allocatorT> &str){
	return os;
}

extern template ostream  & operator<<(ostream  &os, const cow_string  &str);
extern template wostream & operator<<(wostream &os, const cow_wstring &str);

}

#endif
