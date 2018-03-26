// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_PTR_HPP_
#define ASTERIA_VALUE_PTR_HPP_

#include <memory> // std::shared_ptr
#include <utility> // std::move, std::forward

namespace Asteria {

template<typename ElementT>
class Value_ptr {
private:
	std::shared_ptr<ElementT> m_ptr;

public:
	constexpr Value_ptr(std::nullptr_t = nullptr) noexcept
		: m_ptr()
	{ }
	Value_ptr(std::unique_ptr<ElementT> &&ptr)
		: m_ptr(std::move(ptr))
	{ }
	Value_ptr(std::shared_ptr<ElementT> &&ptr)
		: m_ptr(std::move(ptr))
	{ }

public:
	const std::shared_ptr<ElementT> &share() const {
		return m_ptr;
	}

	const ElementT *get() const noexcept {
		return m_ptr.get();
	}
	ElementT *get() noexcept {
		return m_ptr.get();
	}

	explicit operator bool() const noexcept {
		return bool(m_ptr);
	}

	const ElementT &operator*() const noexcept {
		return *m_ptr;
	}
	ElementT &operator*() noexcept {
		return *m_ptr;
	}

	const ElementT *operator->() const noexcept {
		return m_ptr.get();
	}
	ElementT *operator->() noexcept {
		return m_ptr.get();
	}
};

template<typename ElementT, typename ...ParamsT>
Value_ptr<ElementT> make_value(ParamsT &&...params){
	return std::make_shared<ElementT>(std::forward<ParamsT>(params)...);
}

}

#endif
