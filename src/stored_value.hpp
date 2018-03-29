// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STORED_VALUE_HPP_
#define ASTERIA_STORED_VALUE_HPP_

#include "variable.hpp"
#include <boost/optional.hpp>

namespace Asteria {

class Stored_value {
private:
	boost::optional<Variable> m_value_opt;

public:
	Stored_value(Null = nullptr)
		: m_value_opt()
	{ }
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Stored_value, ValueT)>
	Stored_value(ValueT &&value_opt)
		: m_value_opt(std::forward<ValueT>(value_opt))
	{ }

	Stored_value(Stored_value &&);
	Stored_value &operator=(Stored_value &&);
	~Stored_value();

	Stored_value(const Stored_value &) = delete;
	Stored_value &operator=(const Stored_value &) = delete;

public:
	bool is_set() const noexcept {
		return m_value_opt.get_ptr() != nullptr;
	}
	const Variable &get() const {
		return m_value_opt.get();
	}
	Variable &get(){
		return m_value_opt.get();
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_value_opt = std::forward<ValueT>(value);
	}
	void clear() noexcept {
		m_value_opt = boost::none;
	}

public:
	explicit operator bool() const noexcept {
		return is_set();
	}
};

}

#endif
