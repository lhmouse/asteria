// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_PTR_HPP_
#define ASTERIA_VALUE_PTR_HPP_

#include <memory> // std::shared_ptr
#include <utility> // std::move, std::forward
#include <type_traits> // std::enable_if, std::is_convertible, std::remove_cv

namespace Asteria {

template<typename ElementT>
class Value_ptr_nocv {
private:
	std::shared_ptr<ElementT> m_ptr;

public:
	constexpr Value_ptr_nocv(std::nullptr_t = nullptr) noexcept
		: m_ptr()
	{ }
	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	explicit Value_ptr_nocv(std::shared_ptr<OtherT> &&other) noexcept
		: m_ptr(std::move(other))
	{ }
	explicit Value_ptr_nocv(const Value_ptr_nocv &rhs) // An explicit copy constructor prevents unintentional copies.
		: m_ptr(rhs.m_ptr ? std::make_shared<ElementT>(*(rhs.m_ptr)) : nullptr)
	{ }
	Value_ptr_nocv(Value_ptr_nocv &&rhs) noexcept
		: m_ptr(std::move(rhs.m_ptr))
	{ }
	Value_ptr_nocv &operator=(const Value_ptr_nocv &rhs){
		Value_ptr_nocv(rhs).swap(*this);
		return *this;
	}
	Value_ptr_nocv &operator=(Value_ptr_nocv &&rhs) noexcept {
		Value_ptr_nocv(std::move(rhs)).swap(*this);
		return *this;
	}

public:
	std::shared_ptr<const ElementT> share() const noexcept {
		return m_ptr;
	}
	std::shared_ptr<ElementT> share() noexcept {
		return m_ptr;
	}

	const ElementT *get() const noexcept {
		return m_ptr.get();
	}
	ElementT *get() noexcept {
		return m_ptr.get();
	}

	void swap(Value_ptr_nocv &rhs) noexcept {
		using std::swap;
		swap(m_ptr, rhs.m_ptr);
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

template<typename ElementT>
using Value_ptr = Value_ptr_nocv<typename std::remove_cv<ElementT>::type>;

template<typename ElementT, typename ...ParamsT>
Value_ptr<ElementT> make_value(ParamsT &&...params){
	return Value_ptr<ElementT>(std::make_shared<ElementT>(std::forward<ParamsT>(params)...));
}

}

#endif
