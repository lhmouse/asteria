// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "tinyfmt.hpp"
namespace rocket {

template class basic_tinyfmt<char>;

template tinyfmt& operator<<(tinyfmt&, char);
template tinyfmt& operator<<(tinyfmt&, const char*);
template tinyfmt& operator<<(tinyfmt&, const ascii_numput&);
template tinyfmt& operator<<(tinyfmt&, signed char);
template tinyfmt& operator<<(tinyfmt&, signed short);
template tinyfmt& operator<<(tinyfmt&, signed);
template tinyfmt& operator<<(tinyfmt&, signed long);
template tinyfmt& operator<<(tinyfmt&, signed long long);
template tinyfmt& operator<<(tinyfmt&, unsigned char);
template tinyfmt& operator<<(tinyfmt&, unsigned short);
template tinyfmt& operator<<(tinyfmt&, unsigned);
template tinyfmt& operator<<(tinyfmt&, unsigned long);
template tinyfmt& operator<<(tinyfmt&, unsigned long long);
template tinyfmt& operator<<(tinyfmt&, float);
template tinyfmt& operator<<(tinyfmt&, double);
template tinyfmt& operator<<(tinyfmt&, const void*);
template tinyfmt& operator<<(tinyfmt&, void*);
template tinyfmt& operator<<(tinyfmt&, const type_info&);
template tinyfmt& operator<<(tinyfmt&, const exception&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);
template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);

}  // namespace rocket
