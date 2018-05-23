// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STORED_VALUE_HPP_
#define ASTERIA_STORED_VALUE_HPP_

#include "fwd.hpp"
#include "variable.hpp"

namespace Asteria {

class Stored_value {
	friend Variable;

public:
	using Variant = rocket::variant<D_null, Variable::Variant>;

private:
	Variant m_value_opt;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Stored_value, ValueT)>
	Stored_value(ValueT &&value_opt)
		: m_value_opt(std::forward<ValueT>(value_opt))
	{ }
	Stored_value(Stored_value &&) noexcept;
	Stored_value & operator=(Stored_value &&) noexcept;
	~Stored_value();

public:
	bool has_value() const noexcept {
		return m_value_opt.index() == 1;
	}
	const Variable::Variant * get_opt() const noexcept {
		return m_value_opt.try_get<Variable::Variant>();
	}
	Variable::Variant * get_opt() noexcept {
		return m_value_opt.try_get<Variable::Variant>();
	}
	const Variable::Variant & get() const {
		return m_value_opt.get<Variable::Variant>();
	}
	Variable::Variant & get(){
		return m_value_opt.get<Variable::Variant>();
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_value_opt = std::forward<ValueT>(value);
	}
};

extern void set_variable(Xptr<Variable> &variable_out, Spparam<Recycler> recycler, Stored_value &&value_opt);

}

#endif
