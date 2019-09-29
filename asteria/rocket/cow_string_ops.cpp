// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "cow_string_ops.hpp"

namespace rocket {

template ::std::istream& operator>>(::std::istream& is, cow_string& str);
template ::std::wistream& operator>>(::std::wistream& is, cow_wstring& str);

template ::std::ostream& operator<<(::std::ostream& os, const cow_string& str);
template ::std::wostream& operator<<(::std::wostream& os, const cow_wstring& str);

template ::std::istream& getline(::std::istream& is, cow_string& str, char delim);
template ::std::wistream& getline(::std::wistream& is, cow_wstring& str, wchar_t delim);
template ::std::istream& getline(::std::istream& is, cow_string& str);
template ::std::wistream& getline(::std::wistream& is, cow_wstring& str);

}  // namespace rocket
