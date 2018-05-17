// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_NULLPTR_FILLER_HPP_
#define ROCKET_NULLPTR_FILLER_HPP_

#include <iterator> // std::random_access_iterator_tag
#include <cstddef> // std::ptrdiff_t, std::nullptr_t

namespace rocket {

using ::std::ptrdiff_t;
using ::std::nullptr_t;

class nullptr_filler {
public:
	using value_type         = const nullptr_t;
	using pointer            = value_type *;
	using reference          = value_type &;
	using difference_type    = ptrdiff_t;
	using iterator_category  = ::std::random_access_iterator_tag;

private:
	difference_type m_pos;

public:
	explicit constexpr nullptr_filler(difference_type pos) noexcept
		: m_pos(pos)
	{ }

public:
	difference_type tell() const noexcept {
		return this->m_pos;
	}
	nullptr_filler & seek(difference_type pos) noexcept {
		this->m_pos = pos;
		return *this;
	}
};

inline nullptr_filler::reference operator*(const nullptr_filler &) noexcept {
	static constexpr nullptr_filler::value_type s_null = nullptr;
	return s_null;
}

inline nullptr_filler & operator++(nullptr_filler &rhs) noexcept {
	return rhs.seek(rhs.tell() + 1);
}
inline nullptr_filler & operator--(nullptr_filler &rhs) noexcept {
	return rhs.seek(rhs.tell() - 1);
}

inline nullptr_filler operator++(nullptr_filler &lhs, int) noexcept {
	auto res = lhs;
	lhs.seek(lhs.tell() + 1);
	return res;
}
inline nullptr_filler operator--(nullptr_filler &lhs, int) noexcept {
	auto res = lhs;
	lhs.seek(lhs.tell() - 1);
	return res;
}

inline nullptr_filler & operator+=(nullptr_filler &lhs, nullptr_filler::difference_type rhs) noexcept {
	return lhs.seek(lhs.tell() + rhs);
}
inline nullptr_filler & operator-=(nullptr_filler &lhs, nullptr_filler::difference_type rhs) noexcept {
	return lhs.seek(lhs.tell() - rhs);
}

inline nullptr_filler operator+(const nullptr_filler &lhs, nullptr_filler::difference_type rhs) noexcept {
	auto res = lhs;
	res.seek(res.tell() + rhs);
	return res;
}
inline nullptr_filler operator-(const nullptr_filler &lhs, nullptr_filler::difference_type rhs) noexcept {
	auto res = lhs;
	res.seek(res.tell() - rhs);
	return res;
}

inline nullptr_filler operator+(nullptr_filler::difference_type lhs, const nullptr_filler &rhs) noexcept {
	return rhs + lhs;
}
inline nullptr_filler::difference_type operator-(const nullptr_filler &lhs, const nullptr_filler &rhs) noexcept {
	return lhs.tell() - rhs.tell();
}

inline bool operator==(const nullptr_filler &lhs, const nullptr_filler &rhs) noexcept {
	return lhs.tell() == rhs.tell();
}
inline bool operator!=(const nullptr_filler &lhs, const nullptr_filler &rhs) noexcept {
	return lhs.tell() != rhs.tell();
}
inline bool operator<(const nullptr_filler &lhs, const nullptr_filler &rhs) noexcept {
	return lhs.tell() < rhs.tell();
}
inline bool operator>(const nullptr_filler &lhs, const nullptr_filler &rhs) noexcept {
	return lhs.tell() > rhs.tell();
}
inline bool operator<=(const nullptr_filler &lhs, const nullptr_filler &rhs) noexcept {
	return lhs.tell() <= rhs.tell();
}
inline bool operator>=(const nullptr_filler &lhs, const nullptr_filler &rhs) noexcept {
	return lhs.tell() >= rhs.tell();
}

}

#endif
