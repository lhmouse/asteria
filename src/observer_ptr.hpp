// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_OBSERVER_PTR_HPP_
#define ASTERIA_OBSERVEr_PTR_HPP_

#include <memory> // std::shared_ptr, std::unique_ptr, std::addressof
#include <utility> // std::swap
#include <type_traits> // std::enable_if, std::is_convertible
#include <cassert> // assert()
#include "value_ptr.hpp"

namespace Asteria {

template<typename ElementT>
union __attribute__((__transparent_union__)) Observer_ptr {
private:
	ElementT *m_ptr;

public:
	constexpr Observer_ptr(std::nullptr_t = nullptr) noexcept
		: m_ptr(nullptr)
	{ }
	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	Observer_ptr(const OtherT *other) noexcept
		: m_ptr(other)
	{ }
	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	Observer_ptr(const std::shared_ptr<OtherT> &other) noexcept
		: m_ptr(other.get())
	{ }
	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	Observer_ptr(const std::unique_ptr<OtherT> &other) noexcept
		: m_ptr(other.get())
	{ }
	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	Observer_ptr(const Value_ptr<OtherT> &other) noexcept
		: m_ptr(other.get())
	{ }

	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	Observer_ptr(OtherT &other) noexcept
		: m_ptr(std::addressof(other))
	{ }
	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	Observer_ptr(OtherT &&) = delete;

public:
	const ElementT *get() const noexcept {
		return m_ptr;
	}
	ElementT *get() noexcept {
		return m_ptr;
	}

	void swap(Observer_ptr &rhs) noexcept {
		using std::swap;
		swap(m_ptr, rhs.m_ptr);
	}

	explicit operator bool() const noexcept {
		return bool(m_ptr);
	}

	const ElementT &operator*() const noexcept {
		assert(m_ptr);
		return *m_ptr;
	}
	ElementT &operator*() noexcept {
		assert(m_ptr);
		return *m_ptr;
	}

	const ElementT *operator->() const noexcept {
		assert(m_ptr);
		return m_ptr;
	}
	ElementT *operator->() noexcept {
		assert(m_ptr);
		return m_ptr;
	}
};

template<typename ElementT>
bool operator==(const Observer_ptr<ElementT> &lhs, const Observer_ptr<ElementT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename ElementT>
bool operator!=(const Observer_ptr<ElementT> &lhs, const Observer_ptr<ElementT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename ElementT>
bool operator<(const Observer_ptr<ElementT> &lhs, const Observer_ptr<ElementT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename ElementT>
bool operator>(const Observer_ptr<ElementT> &lhs, const Observer_ptr<ElementT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename ElementT>
bool operator<=(const Observer_ptr<ElementT> &lhs, const Observer_ptr<ElementT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename ElementT>
bool operator>=(const Observer_ptr<ElementT> &lhs, const Observer_ptr<ElementT> &rhs) noexcept {
	return lhs.get() >= rhs.get();
}

template<typename ElementT>
bool operator==(const Observer_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() == nullptr;
}
template<typename ElementT>
bool operator!=(const Observer_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() != nullptr;
}
template<typename ElementT>
bool operator<(const Observer_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() < nullptr;
}
template<typename ElementT>
bool operator>(const Observer_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() > nullptr;
}
template<typename ElementT>
bool operator<=(const Observer_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() <= nullptr;
}
template<typename ElementT>
bool operator>=(const Observer_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() >= nullptr;
}

template<typename ElementT>
bool operator==(std::nullptr_t, const Observer_ptr<ElementT> &rhs) noexcept {
	return nullptr == rhs.get();
}
template<typename ElementT>
bool operator!=(std::nullptr_t, const Observer_ptr<ElementT> &rhs) noexcept {
	return nullptr != rhs.get();
}
template<typename ElementT>
bool operator<(std::nullptr_t, const Observer_ptr<ElementT> &rhs) noexcept {
	return nullptr < rhs.get();
}
template<typename ElementT>
bool operator>(std::nullptr_t, const Observer_ptr<ElementT> &rhs) noexcept {
	return nullptr > rhs.get();
}
template<typename ElementT>
bool operator<=(std::nullptr_t, const Observer_ptr<ElementT> &rhs) noexcept {
	return nullptr <= rhs.get();
}
template<typename ElementT>
bool operator>=(std::nullptr_t, const Observer_ptr<ElementT> &rhs) noexcept {
	return nullptr >= rhs.get();
}

template<typename ElementT>
void swap(const Observer_ptr<ElementT> &lhs, const Observer_ptr<ElementT> &rhs) noexcept {
	lhs.swap(rhs);
}

}

#endif
