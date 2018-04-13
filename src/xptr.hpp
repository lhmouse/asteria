// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_XPTR_HPP_
#define ASTERIA_XPTR_HPP_

#include <memory> // std::shared_ptr
#include <utility> // std::move, std::swap
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
	Xptr(Xptr &&rhs) noexcept
		: m_ptr(std::move(rhs.m_ptr))
	{ }
	Xptr &operator=(Xptr &&rhs) noexcept {
		m_ptr = std::move(rhs.m_ptr);
		return *this;
	}

public:
	const ElementT *get() const noexcept {
		return m_ptr.get();
	}
	ElementT *get() noexcept {
		return m_ptr.get();
	}

	std::shared_ptr<const ElementT> share() const noexcept {
		return m_ptr;
	}
	std::shared_ptr<ElementT> share() noexcept {
		return m_ptr;
	}
	std::weak_ptr<const ElementT> weaken() const noexcept {
		return m_ptr;
	}
	std::weak_ptr<ElementT> weaken() noexcept {
		return m_ptr;
	}

	void reset(std::nullptr_t = nullptr) noexcept {
		m_ptr.reset();
	}
	template<typename OtherT>
	void reset(std::shared_ptr<OtherT> &&other) noexcept {
		m_ptr = std::move(other);
	}
	std::shared_ptr<ElementT> release() noexcept {
		std::shared_ptr<ElementT> ptr;
		ptr.swap(m_ptr);
		return ptr;
	}

	void swap(Xptr &rhs) noexcept {
		using std::swap;
		swap(m_ptr, rhs.m_ptr);
	}

public:
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

	explicit operator bool() const noexcept {
		return m_ptr.operator bool();
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

template<typename ElementT, typename OtherT>
bool operator==(const Xptr<ElementT> &lhs, const Xptr<OtherT> &rhs) noexcept {
	return lhs.get() == rhs.get();
}
template<typename ElementT, typename OtherT>
bool operator!=(const Xptr<ElementT> &lhs, const Xptr<OtherT> &rhs) noexcept {
	return lhs.get() != rhs.get();
}
template<typename ElementT, typename OtherT>
bool operator<(const Xptr<ElementT> &lhs, const Xptr<OtherT> &rhs) noexcept {
	return lhs.get() < rhs.get();
}
template<typename ElementT, typename OtherT>
bool operator>(const Xptr<ElementT> &lhs, const Xptr<OtherT> &rhs) noexcept {
	return lhs.get() > rhs.get();
}
template<typename ElementT, typename OtherT>
bool operator<=(const Xptr<ElementT> &lhs, const Xptr<OtherT> &rhs) noexcept {
	return lhs.get() <= rhs.get();
}
template<typename ElementT, typename OtherT>
bool operator>=(const Xptr<ElementT> &lhs, const Xptr<OtherT> &rhs) noexcept {
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

template<typename OtherT>
bool operator==(std::nullptr_t, const Xptr<OtherT> &rhs) noexcept {
	return nullptr == rhs.get();
}
template<typename OtherT>
bool operator!=(std::nullptr_t, const Xptr<OtherT> &rhs) noexcept {
	return nullptr != rhs.get();
}
template<typename OtherT>
bool operator<(std::nullptr_t, const Xptr<OtherT> &rhs) noexcept {
	return nullptr < rhs.get();
}
template<typename OtherT>
bool operator>(std::nullptr_t, const Xptr<OtherT> &rhs) noexcept {
	return nullptr > rhs.get();
}
template<typename OtherT>
bool operator<=(std::nullptr_t, const Xptr<OtherT> &rhs) noexcept {
	return nullptr <= rhs.get();
}
template<typename OtherT>
bool operator>=(std::nullptr_t, const Xptr<OtherT> &rhs) noexcept {
	return nullptr >= rhs.get();
}

template<typename ElementT>
void swap(Xptr<ElementT> &lhs, Xptr<ElementT> &rhs) noexcept {
	lhs.swap(rhs);
}

}

#endif
