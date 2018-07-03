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
using ::std::remove_const;
using ::std::is_same;
using ::std::remove_extent;

template<typename elementT>
class value_ptr;

template<typename elementT>
class value_ptr {
	static_assert(is_same<typename remove_const<elementT>::type, elementT>::value, "A top-level `const` qualifier on `elementT` is not allowed.");

public:
	using element_type  = typename remove_extent<elementT>::type;
	using shared_type   = shared_ptr<elementT>;
	using weak_type     = weak_ptr<elementT>;

private:
	shared_type m_ptr;

public:
	// 23.11.1.2.1, constructors
	constexpr value_ptr(nullptr_t = nullptr) noexcept
		: m_ptr(nullptr)
	{ }
	template<typename otherT, typename enable_if<is_convertible<otherT *, elementT *>::value>::type * = nullptr>
	explicit value_ptr(shared_ptr<otherT> &&other) noexcept
		: m_ptr(::std::move(other))
	{ }
	// 23.11.1.2.3, assignment
	value_ptr(value_ptr &&rhs) = default;
	value_ptr & operator=(value_ptr &&rhs) = default;

public:
	// 23.11.1.2.4, observers
	const elementT & operator*() const noexcept {
		ROCKET_ASSERT(this->m_ptr);
		return *(this->m_ptr);
	}
	elementT & operator*() noexcept {
		ROCKET_ASSERT(this->m_ptr);
		return *(this->m_ptr);
	}
	const elementT * operator->() const noexcept {
		ROCKET_ASSERT(this->m_ptr);
		return this->m_ptr.get();
	}
	elementT * operator->() noexcept {
		ROCKET_ASSERT(this->m_ptr);
		return this->m_ptr.get();
	}

	template<typename otherT = elementT>
	const otherT * get() const noexcept {
		return this->m_ptr.get();
	}
	template<typename otherT = elementT>
	otherT * get() noexcept {
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
	weak_ptr<const otherT> weaken() const noexcept {
		return this->m_ptr;
	}
	template<typename otherT = elementT>
	weak_ptr<otherT> weaken() noexcept {
		return this->m_ptr;
	}

	template<typename otherT = elementT>
	const otherT * cget() const noexcept {
		return this->get();
	}
	template<typename otherT = elementT>
	shared_ptr<const otherT> cshare() const noexcept {
		return this->share();
	}
	template<typename otherT = elementT>
	weak_ptr<const otherT> cweaken() const noexcept {
		return this->weaken();
	}

	explicit operator bool () const noexcept {
		return this->m_ptr.get() != nullptr;
	}
	operator shared_ptr<const elementT> () const noexcept {
		return this->m_ptr;
	}
	operator shared_ptr<elementT> & () noexcept {
		return this->m_ptr;
	}

	// 23.11.1.2.5, modifiers
	shared_type release() noexcept {
		shared_type ptr;
		this->m_ptr.swap(ptr);
		return ptr;
	}
	value_ptr & reset(nullptr_t = nullptr) noexcept {
		this->m_ptr.reset();
		return *this;
	}
	template<typename otherT>
	value_ptr & reset(shared_ptr<otherT> &&other) noexcept {
		this->m_ptr = ::std::move(other);
		return *this;
	}
	template<typename ...paramsT>
	value_ptr & emplace(paramsT &&...params){
		this->m_ptr = ::std::make_shared<elementT>(::std::forward<paramsT>(params)...);
		return *this;
	}

	void swap(value_ptr &rhs) noexcept {
		this->m_ptr.swap(rhs.m_ptr);
	}
};

template<typename elementT, typename otherT>
inline bool operator==(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator!=(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator<(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator>(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator<=(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator>=(const value_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() >= rhs.get();
}

template<typename elementT, typename otherT>
inline bool operator==(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator!=(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator<(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator>(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator<=(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator>=(const shared_ptr<elementT> &lhs, const value_ptr<otherT> &rhs) noexcept {
	return lhs.get() >= rhs.get();
}

template<typename elementT, typename otherT>
inline bool operator==(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator!=(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator<(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator>(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator<=(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename elementT, typename otherT>
inline bool operator>=(const value_ptr<elementT> &lhs, const shared_ptr<otherT> &rhs) noexcept {
	return lhs.get() >= rhs.get();
}

template<typename elementT>
inline bool operator==(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() == nullptr;
}
template<typename elementT>
inline bool operator!=(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() != nullptr;
}
template<typename elementT>
inline bool operator<(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() < nullptr;
}
template<typename elementT>
inline bool operator>(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() > nullptr;
}
template<typename elementT>
inline bool operator<=(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() <= nullptr;
}
template<typename elementT>
inline bool operator>=(const value_ptr<elementT> &lhs, nullptr_t) noexcept {
	return lhs.get() >= nullptr;
}

template<typename otherT>
inline bool operator==(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr == rhs.get();
}
template<typename otherT>
inline bool operator!=(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr != rhs.get();
}
template<typename otherT>
inline bool operator<(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr < rhs.get();
}
template<typename otherT>
inline bool operator>(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr > rhs.get();
}
template<typename otherT>
inline bool operator<=(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr <= rhs.get();
}
template<typename otherT>
inline bool operator>=(nullptr_t, const value_ptr<otherT> &rhs) noexcept {
	return nullptr >= rhs.get();
}

template<typename elementT>
inline void swap(value_ptr<elementT> &lhs, value_ptr<elementT> &rhs) noexcept {
	lhs.swap(rhs);
}

template<typename charT, typename traitsT, typename elementT>
inline basic_ostream<charT, traitsT> & operator<<(basic_ostream<charT, traitsT> &os, const value_ptr<elementT> &rhs){
	return os <<rhs.get();
}

}

#endif
