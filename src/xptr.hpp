// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_XPTR_HPP_
#define ASTERIA_XPTR_HPP_

#include <memory> // std::shared_ptr
#include <utility> // std::move, std::forward, std::swap
#include <type_traits> // std::enable_if, std::is_convertible, std::remove_cv, std::is_same

namespace Asteria {

template<typename ElementT>
class Xptr {
	static_assert(std::is_same<typename std::remove_cv<ElementT>::type, ElementT>::value, "Please remove top-level cv-qualifiers from `ElementT`");

private:
	std::shared_ptr<ElementT> m_ptr;

public:
	constexpr Xptr(std::nullptr_t = nullptr) noexcept
		: m_ptr(nullptr)
	{ }
	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	explicit Xptr(std::shared_ptr<OtherT> &&other) noexcept
		: m_ptr(std::move(other))
	{ }

	Xptr(const Xptr &rhs)
		: m_ptr(rhs.m_ptr ? std::make_shared<ElementT>(*(rhs.m_ptr)) : nullptr)
	{ }
	Xptr &operator=(const Xptr &rhs) noexcept {
		Xptr(rhs).swap(*this);
		return *this;
	}

	Xptr(Xptr &&rhs) = default;
	Xptr &operator=(Xptr &&rhs) = default;

public:
	const ElementT *get() const noexcept {
		return m_ptr.get();
	}
	ElementT *get() noexcept {
		return m_ptr.get();
	}

	void swap(Xptr &rhs) noexcept {
		using std::swap;
		swap(m_ptr, rhs.m_ptr);
	}

	explicit operator bool() const noexcept {
		return m_ptr != nullptr;
	}

	operator std::shared_ptr<const ElementT>() const noexcept {
		return m_ptr;
	}
	operator std::shared_ptr<ElementT>() noexcept {
		return m_ptr;
	}

	operator std::weak_ptr<const ElementT>() const noexcept {
		return m_ptr;
	}
	operator std::weak_ptr<ElementT>() noexcept {
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
bool operator==(const Xptr<ElementT> &lhs, const Xptr<ElementT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename ElementT>
bool operator!=(const Xptr<ElementT> &lhs, const Xptr<ElementT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename ElementT>
bool operator<(const Xptr<ElementT> &lhs, const Xptr<ElementT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename ElementT>
bool operator>(const Xptr<ElementT> &lhs, const Xptr<ElementT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename ElementT>
bool operator<=(const Xptr<ElementT> &lhs, const Xptr<ElementT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename ElementT>
bool operator>=(const Xptr<ElementT> &lhs, const Xptr<ElementT> &rhs) noexcept {
	return lhs.get() >= rhs.get();
}

template<typename ElementT>
bool operator==(const Xptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() == nullptr;
}
template<typename ElementT>
bool operator!=(const Xptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() != nullptr;
}
template<typename ElementT>
bool operator<(const Xptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() < nullptr;
}
template<typename ElementT>
bool operator>(const Xptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() > nullptr;
}
template<typename ElementT>
bool operator<=(const Xptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() <= nullptr;
}
template<typename ElementT>
bool operator>=(const Xptr<ElementT> &lhs, std::nullptr_t) noexcept {
	return lhs.get() >= nullptr;
}

template<typename ElementT>
bool operator==(std::nullptr_t, const Xptr<ElementT> &rhs) noexcept {
	return nullptr == rhs.get();
}
template<typename ElementT>
bool operator!=(std::nullptr_t, const Xptr<ElementT> &rhs) noexcept {
	return nullptr != rhs.get();
}
template<typename ElementT>
bool operator<(std::nullptr_t, const Xptr<ElementT> &rhs) noexcept {
	return nullptr < rhs.get();
}
template<typename ElementT>
bool operator>(std::nullptr_t, const Xptr<ElementT> &rhs) noexcept {
	return nullptr > rhs.get();
}
template<typename ElementT>
bool operator<=(std::nullptr_t, const Xptr<ElementT> &rhs) noexcept {
	return nullptr <= rhs.get();
}
template<typename ElementT>
bool operator>=(std::nullptr_t, const Xptr<ElementT> &rhs) noexcept {
	return nullptr >= rhs.get();
}

template<typename ElementT>
void swap(const Xptr<ElementT> &lhs, const Xptr<ElementT> &rhs) noexcept {
	lhs.swap(rhs);
}

}

#endif
