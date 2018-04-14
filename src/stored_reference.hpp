// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STORED_REFERENCE_HPP_
#define ASTERIA_STORED_REFERENCE_HPP_

#include "fwd.hpp"
#include "reference.hpp"

namespace Asteria {

class Stored_reference {
	friend Reference;

public:
	using Variant = Reference::Types::rebind_as_variant;
	using Types   = Type_tuple<D_null, Variant>;

private:
	Types::rebind_as_variant m_value_opt;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Reference, ValueT), ASTERIA_UNLESS_IS_BASE_OF(Stored_reference, ValueT)>
	Stored_reference(ValueT &&value_opt)
		: m_value_opt(std::forward<ValueT>(value_opt))
	{ }
	Stored_reference(Reference &&reference)
		: m_value_opt(std::move(reference.m_variant))
	{ }
	Stored_reference(Stored_reference &&);
	Stored_reference &operator=(Stored_reference &&);
	~Stored_reference();

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
