// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STORED_VALUE_HPP_
#define ASTERIA_STORED_VALUE_HPP_

#include "fwd.hpp"
#include "value.hpp"

namespace Asteria {

class Stored_value {
	friend Value;

public:
	using Variant = rocket::variant<D_null, Value::Variant>;

private:
	Variant m_value_opt;

public:
	template<typename CandidateT, ASTERIA_UNLESS_IS_BASE_OF(Stored_value, CandidateT)>
	Stored_value(CandidateT &&value_opt)
		: m_value_opt(std::forward<CandidateT>(value_opt))
	{ }
	Stored_value(Stored_value &&) noexcept;
	Stored_value & operator=(Stored_value &&) noexcept;
	~Stored_value();

public:
	bool has_value() const noexcept {
		return m_value_opt.index() == 1;
	}
	const Value::Variant * get_opt() const noexcept {
		return m_value_opt.try_get<Value::Variant>();
	}
	Value::Variant * get_opt() noexcept {
		return m_value_opt.try_get<Value::Variant>();
	}
	const Value::Variant & get() const {
		return m_value_opt.get<Value::Variant>();
	}
	Value::Variant & get(){
		return m_value_opt.get<Value::Variant>();
	}
	template<typename CandidateT>
	void set(CandidateT &&cand){
		m_value_opt = std::forward<CandidateT>(cand);
	}
};

extern void set_value(Vp<Value> &value_out, Spr<Recycler> recycler, Stored_value &&value_opt);

}

#endif
