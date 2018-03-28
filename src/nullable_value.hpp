// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_NULLABLE_BALUE_HPP_
#define ASTERIA_NULLABLE_BALUE_HPP_

#include "variable.hpp"

namespace Asteria {

class Nullable_value {
public:
	using Type = Variable::Type;
	using Types = typename Variable::Types::prepend< Variable  // -2
	                                               , Null      // -1
		>::type;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Nullable_value, ValueT)>
	Nullable_value(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	~Nullable_value();

	ASTERIA_FORBID_COPY(Nullable_value)
	ASTERIA_ALLOW_MOVE(Nullable_value)

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which() - 2);
	}

	template<typename ExpectT>
	const ExpectT &get() const {
		return boost::get<ExpectT>(m_variant);
	}
	template<typename ExpectT>
	ExpectT &get(){
		return boost::get<ExpectT>(m_variant);
	}
};

}

#endif
