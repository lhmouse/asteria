// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_PTR_HPP_
#define ASTERIA_VALUE_PTR_HPP_

#include <memory> // std::shared_ptr
#include <utility> // std::move, std::forward
#include <type_traits> // std::enable_if, std::is_convertible

namespace Asteria {

template<typename ElementT>
class Value_ptr {
private:
	std::shared_ptr<ElementT> m_ptr;

public:
	constexpr Value_ptr(std::nullptr_t = nullptr) noexcept
		: m_ptr()
	{ }
	template<typename OtherT, typename std::enable_if<std::is_convertible<OtherT *, ElementT *>::value>::type * = nullptr>
	Value_ptr(std::shared_ptr<OtherT> &&other) noexcept
		: m_ptr(std::move(other))
	{ }
	explicit Value_ptr(const Value_ptr &rhs) // An explicit copy constructor helps us find out typos.
		: m_ptr(rhs.m_ptr ? std::make_shared<ElementT>(*(rhs.m_ptr)) : nullptr)
	{ }
	Value_ptr(Value_ptr &&rhs) noexcept
		: m_ptr(std::move(rhs.m_ptr))
	{ }
	Value_ptr &operator=(const Value_ptr &rhs){
		if(this != &rhs){
			if(m_ptr && (m_ptr.use_count() == 1) && rhs.m_ptr){
				*m_ptr = *rhs.m_ptr;
			} else {
				m_ptr = rhs.m_ptr ? std::make_shared<ElementT>(*(rhs.m_ptr)) : nullptr;
			}
		}
		return *this;
	}
	Value_ptr &operator=(Value_ptr &&rhs) noexcept {
		m_ptr = std::move(rhs.m_ptr);
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
