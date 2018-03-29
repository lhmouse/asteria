// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_PTR_HPP_
#define ASTERIA_VALUE_PTR_HPP_

#include <memory> // std::shared_ptr
#include <utility> // std::move, std::forward, std::swap
#include <type_traits> // std::enable_if, std::is_convertible, std::remove_cv, std::is_same

namespace Asteria {

template<typename ElementT>
class Value_ptr {
	static_assert(std::is_same<typename std::remove_cv<ElementT>::type, ElementT>::value, "Please remove top-level cv-qualifiers from `ElementT`");

private:
	std::shared_ptr<ElementT> m_ptr;

public:
	constexpr Value_ptr(std::nullptr_t = nullptr) noexcept
		: m_ptr(nullptr)
	{ }
	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	explicit Value_ptr(std::shared_ptr<OtherT> &&other) noexcept
		: m_ptr(std::move(other))
	{ }

	explicit Value_ptr(const Value_ptr &rhs) // An explicit copy constructor prevents unintentional copies.
		: m_ptr(rhs.m_ptr ? std::make_shared<ElementT>(*(rhs.m_ptr)) : nullptr)
	{ }
	Value_ptr &operator=(const Value_ptr &rhs) noexcept {
		Value_ptr(rhs).swap(*this);
		return *this;
	}

	Value_ptr(Value_ptr &&rhs) = default;
	Value_ptr &operator=(Value_ptr &&rhs) = default;

public:
	const ElementT *get() const noexcept {
		return m_ptr.get();
	}
	ElementT *get() noexcept {
		return m_ptr.get();
	}

	void swap(Value_ptr &rhs) noexcept {
		using std::swap;
		swap(m_ptr, rhs.m_ptr);
	}

	explicit operator bool() const noexcept {
		return bool(m_ptr);
	}
	operator std::shared_ptr<const ElementT>() const noexcept {
		return m_ptr;
	}
	operator std::shared_ptr<ElementT>() noexcept {
		return m_ptr;
	}

	const ElementT &operator*() const noexcept {
		return m_ptr.operator*();
	}
	ElementT &operator*() noexcept {
		return m_ptr.operator*();
	}

	const ElementT *operator->() const noexcept {
		return m_ptr.operator->();
	}
	ElementT *operator->() noexcept {
		return m_ptr.operator->();
	}
};

template<typename ElementT>
bool operator==(const Value_ptr<ElementT> &lhs, const Value_ptr<ElementT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename ElementT>
bool operator!=(const Value_ptr<ElementT> &lhs, const Value_ptr<ElementT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename ElementT>
bool operator<(const Value_ptr<ElementT> &lhs, const Value_ptr<ElementT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename ElementT>
bool operator>(const Value_ptr<ElementT> &lhs, const Value_ptr<ElementT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename ElementT>
bool operator<=(const Value_ptr<ElementT> &lhs, const Value_ptr<ElementT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename ElementT>
bool operator>=(const Value_ptr<ElementT> &lhs, const Value_ptr<ElementT> &rhs) noexcept {
	return lhs.get() >= rhs.get();
}

template<typename ElementT>
bool operator==(const Value_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() == nullptr;
}
template<typename ElementT>
bool operator!=(const Value_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() != nullptr;
}
template<typename ElementT>
bool operator<(const Value_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() < nullptr;
}
template<typename ElementT>
bool operator>(const Value_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() > nullptr;
}
template<typename ElementT>
bool operator<=(const Value_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() <= nullptr;
}
template<typename ElementT>
bool operator>=(const Value_ptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() >= nullptr;
}

template<typename ElementT>
bool operator==(std::nullptr_t, const Value_ptr<ElementT> &rhs) noexcept {
	return nullptr == rhs.get();
}
template<typename ElementT>
bool operator!=(std::nullptr_t, const Value_ptr<ElementT> &rhs) noexcept {
	return nullptr != rhs.get();
}
template<typename ElementT>
bool operator<(std::nullptr_t, const Value_ptr<ElementT> &rhs) noexcept {
	return nullptr < rhs.get();
}
template<typename ElementT>
bool operator>(std::nullptr_t, const Value_ptr<ElementT> &rhs) noexcept {
	return nullptr > rhs.get();
}
template<typename ElementT>
bool operator<=(std::nullptr_t, const Value_ptr<ElementT> &rhs) noexcept {
	return nullptr <= rhs.get();
}
template<typename ElementT>
bool operator>=(std::nullptr_t, const Value_ptr<ElementT> &rhs) noexcept {
	return nullptr >= rhs.get();
}

template<typename ElementT>
void swap(const Value_ptr<ElementT> &lhs, const Value_ptr<ElementT> &rhs) noexcept {
	lhs.swap(rhs);
}

}

#endif
