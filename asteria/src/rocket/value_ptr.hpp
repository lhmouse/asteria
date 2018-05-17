// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VALUE_PTR_HPP_
#define ROCKET_VALUE_PTR_HPP_

#include <cstddef> // std::nullptr_t
#include <memory> // std::shared_ptr<>, std::weak_ptr<>, std::make_shared()
#include <utility> // std::move(), std::swap(), std::forward()
#include <ostream> // std::basic_ostream<>
#include <type_traits> // std::enable_if<>, std::is_convertible<>, std::remove_cv<>, std::is_same<>
#include "assert.hpp"

namespace rocket {

using ::std::nullptr_t;
using ::std::shared_ptr;
using ::std::weak_ptr;
using ::std::basic_ostream;
using ::std::enable_if;
using ::std::is_convertible;
using ::std::remove_cv;
using ::std::is_same;

template<typename elementT>
class value_ptr {
	static_assert(is_same<typename remove_cv<elementT>::type, elementT>::value, "Top-level cv-qualifiers are not allowed.");

private:
	shared_ptr<elementT> m_ptr;

public:
	constexpr value_ptr(nullptr_t = nullptr) noexcept
		: m_ptr(nullptr)
	{ }
	template<typename otherT, typename enable_if<is_convertible<otherT *, elementT *>::value>::type * = nullptr>
	explicit value_ptr(shared_ptr<otherT> &&other) noexcept
		: m_ptr(::std::move(other))
	{ }
	value_ptr(value_ptr &&rhs) = default;
	value_ptr & operator=(value_ptr &&rhs) = default;

public:
	const elementT * get() const noexcept {
		return this->m_ptr.get();
	}
	elementT * get() noexcept {
		return this->m_ptr.get();
	}

	template<typename otherT = elementT>
	shared_ptr<const otherT> share() const noexcept {
		return this->m_ptr;
	}
	template<typename otherT = elementT>
	shared_ptr<otherT> share() noexcept {
		return this->m_ptr;
	}
	template<typename otherT = elementT>
	shared_ptr<const otherT> share_c() const noexcept {
		return this->share();
	}

	template<typename otherT = elementT>
	weak_ptr<const otherT> weaken() const noexcept {
		return this->m_ptr;
	}
	template<typename otherT = elementT>
	weak_ptr<otherT> weaken() noexcept {
		return this->m_ptr;
	}
	template<typename otherT = elementT>
	weak_ptr<const otherT> weaken_c() const noexcept {
		return this->weaken();
	}

	void reset(nullptr_t = nullptr) noexcept {
		this->m_ptr.reset();
	}
	template<typename otherT>
	void reset(shared_ptr<otherT> &&other) noexcept {
		this->m_ptr = ::std::move(other);
	}
	template<typename ...paramsT>
	void emplace(paramsT &&...params){
		this->m_ptr = ::std::make_shared<elementT>(::std::forward<paramsT>(params)...);
	}
	shared_ptr<elementT> release() noexcept {
		shared_ptr<elementT> ptr;
		ptr.swap(this->m_ptr);
		return ptr;
	}

	void swap(value_ptr &rhs) noexcept {
		this->m_ptr.swap(rhs.m_ptr);
	}

public:
	operator shared_ptr<const elementT>() const noexcept {
		return this->m_ptr;
	}
	operator shared_ptr<elementT>() noexcept {
		return this->m_ptr;
	}
	operator weak_ptr<const elementT>() const noexcept {
		return this->m_ptr;
	}
	operator weak_ptr<elementT>() noexcept {
		return this->m_ptr;
	}

	explicit operator bool() const noexcept {
		return this->m_ptr.operator bool();
	}
	const elementT & operator*() const {
		ROCKET_ASSERT(this->m_ptr != nullptr);
		return this->m_ptr.operator*();
	}
	elementT & operator*(){
		ROCKET_ASSERT(this->m_ptr != nullptr);
		return this->m_ptr.operator*();
	}
	const elementT * operator->() const {
		ROCKET_ASSERT(this->m_ptr != nullptr);
		return this->m_ptr.operator->();
	}
	elementT * operator->(){
		ROCKET_ASSERT(this->m_ptr != nullptr);
		return this->m_ptr.operator->();
	}
};

template<typename elementT, typename otherT>
bool operator==(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename elementT, typename otherT>
bool operator!=(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename elementT, typename otherT>
bool operator<(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename elementT, typename otherT>
bool operator>(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename elementT, typename otherT>
bool operator<=(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename elementT, typename otherT>
bool operator>=(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() >= rhs.get();
}

template<typename elementT, typename otherT>
bool operator==(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename elementT, typename otherT>
bool operator!=(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename elementT, typename otherT>
bool operator<(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename elementT, typename otherT>
bool operator>(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename elementT, typename otherT>
bool operator<=(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename elementT, typename otherT>
bool operator>=(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() >= rhs.get();
}

template<typename elementT, typename otherT>
bool operator==(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename elementT, typename otherT>
bool operator!=(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename elementT, typename otherT>
bool operator<(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename elementT, typename otherT>
bool operator>(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename elementT, typename otherT>
bool operator<=(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename elementT, typename otherT>
bool operator>=(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() >= rhs.get();
}

template<typename elementT>
bool operator==(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() == nullptr;
}
template<typename elementT>
bool operator!=(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() != nullptr;
}
template<typename elementT>
bool operator<(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() < nullptr;
}
template<typename elementT>
bool operator>(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() > nullptr;
}
template<typename elementT>
bool operator<=(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() <= nullptr;
}
template<typename elementT>
bool operator>=(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() >= nullptr;
}

template<typename otherT>
bool operator==(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr == rhs.get();
}
template<typename otherT>
bool operator!=(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr != rhs.get();
}
template<typename otherT>
bool operator<(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr < rhs.get();
}
template<typename otherT>
bool operator>(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr > rhs.get();
}
template<typename otherT>
bool operator<=(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr <= rhs.get();
}
template<typename otherT>
bool operator>=(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr >= rhs.get();
}

template<typename elementT>
void swap(value_ptr<elementT> &lhs, value_ptr<elementT> &rhs) noexcept {
	lhs.swap(rhs);
}

template<typename charT, typename traitsT, typename elementT>
basic_ostream<charT, traitsT> & operator<<(basic_ostream<charT, traitsT> &os, const value_ptr<elementT> &rhs){
	return os <<rhs.get();
}

}

#endif
