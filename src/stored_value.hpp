// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STORED_VALUE_HPP_
#define ASTERIA_STORED_VALUE_HPP_

#include "fwd.hpp"
#include "variable.hpp"

namespace Asteria {

class Stored_value {
public:
	using Variant = Variable::Types::rebind_as_variant;
	using Types   = Type_tuple<D_null, Variant>;

private:
	Types::rebind_as_variant m_value_opt;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Variable, ValueT), ASTERIA_UNLESS_IS_BASE_OF(Stored_value, ValueT)>
	Stored_value(ValueT &&value_opt)
		: m_value_opt(std::forward<ValueT>(value_opt))
	{ }
	Stored_value(Variable &&variable)
		: m_value_opt(std::move(variable.m_variant))
	{ }
	Stored_value(Stored_value &&);
	Stored_value &operator=(Stored_value &&);
	~Stored_value();

public:
	bool has_value() const noexcept {
		return m_value_opt.which() == 1;
	}
	const Variant *get_opt() const noexcept {
		return boost::get<Variant>(&m_value_opt);
	}
	Variant *get_opt() noexcept {
		return boost::get<Variant>(&m_value_opt);
	}
	const Variant &get() const {
		return boost::get<Variant>(m_value_opt);
	}
	Variant &get(){
		return boost::get<Variant>(m_value_opt);
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_value_opt = std::forward<ValueT>(value);
	}
};

}

#endif
