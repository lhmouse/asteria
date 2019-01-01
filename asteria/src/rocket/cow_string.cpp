// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "cow_string.hpp"

namespace rocket {

template class basic_cow_string<char>;
template class basic_cow_string<wchar_t>;
template class basic_cow_string<char16_t>;
template class basic_cow_string<char32_t>;

template ::std::istream  & operator>>(::std::istream  &is, cow_string  &str);
template ::std::wistream & operator>>(::std::wistream &is, cow_wstring &str);

template ::std::ostream  & operator<<(::std::ostream  &os, const cow_string  &str);
template ::std::wostream & operator<<(::std::wostream &os, const cow_wstring &str);

template ::std::istream  & getline(::std::istream  &is, cow_string  &str, char    delim);
template ::std::wistream & getline(::std::wistream &is, cow_wstring &str, wchar_t delim);
template ::std::istream  & getline(::std::istream  &is, cow_string  &str);
template ::std::wistream & getline(::std::wistream &is, cow_wstring &str);

}
